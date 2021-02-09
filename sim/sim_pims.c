#include <eos.h>
#include <eos_pims.h>
#include <eos_log.h>
#include <eos_util.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef EOS_BF_TEST
    #include <valgrind/bitflips.h>
    #ifdef EOS_PIMS_U16_DATA
        #define BITFLIPS_DTYPE BITFLIPS_USHORT
    #else
        #define BITFLIPS_DTYPE BITFLIPS_UINT
    #endif
#endif

#include "sim_io.h"
#include "sim_log.h"
#include "sim_util.h"
#include "sim_pims.h"
#include "run_sim.h"

/* Defaults for 'init_params'.
 * We don't want contributions from other algorithms! */
EosStatus default_init_params_pims(EosInitParams* init_params){
    EosPimsParams pims_params = {
        .alg = EOS_PIMS_BASELINE,
        .params = {
            .common_params = {
                .filter = EOS_PIMS_MEDIAN_FILTER,
                .max_observations = 1000,
                .max_bins = 100,
            }
        }
    };
    init_params -> pims_params = pims_params;
    init_params -> mise_max_bands = 0;
    return EOS_SUCCESS;
}

/* Parses the algorithm name (a string) to convert to a EosPimsAlgorithm. */
EosPimsAlgorithm parse_algorithm(const char* alg){
    if(alg == NULL){
        return EOS_PIMS_NO_ALGORITHM;
    }

    if(strncmp(alg, "baseline", strlen("baseline")) == 0){
        return EOS_PIMS_BASELINE;
    }

    eos_logf(EOS_LOG_WARN, "Algorithm will be set to 'none' as no options matched.");
    return EOS_PIMS_NO_ALGORITHM;
}

/* Parses the filter name (a string) to convert to a EosPimsFilter. */
EosPimsFilter parse_filter(const char* filter){
    if(filter == NULL){
        return EOS_PIMS_NO_FILTER;
    }

    if(strncmp(filter, "min", strlen("min")) == 0){
        return EOS_PIMS_MIN_FILTER;
    }

    if(strncmp(filter, "mean", strlen("mean")) == 0){
        return EOS_PIMS_MEAN_FILTER;
    }

    if(strncmp(filter, "median", strlen("median")) == 0){
        return EOS_PIMS_MEDIAN_FILTER;
    }

    if(strncmp(filter, "max", strlen("max")) == 0){
        return EOS_PIMS_MAX_FILTER;
    }

    eos_logf(EOS_LOG_WARN, "Filter will be set to 'none' as no options matched.");
    return EOS_PIMS_NO_FILTER;
}

/* Loads the config for PIMS parameters, using our own code.
 * Only compiled if libconfig is not allowed. */
#ifdef EOS_NO_LIBCONFIG
EosStatus load_custom_config(config_ptr config_file, EosParams* params) {

    /* 'params' must be non-NULL. */
    if(eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }

    /* Start by initializing all parameters to defaults. */
    EosStatus status = eos_init_default_params(params);
    if (status != EOS_SUCCESS) { return status; }

    if (config_file == NULL) {
        eos_logf(EOS_LOG_INFO, "No config file provided; using defaults.");
        return EOS_SUCCESS;
    }

    /* Open the config file. */
    FILE* fp = fopen(config_file, "r");
    if(eos_assert(fp != NULL)){
        return EOS_ASSERT_ERROR;
    }

    /* Read config line-by-line. The format is fixed:
     *  - Line 0: Filter.
     *  - Line 1: Max Observations in Filter.
     *  - Line 2: Algorithm.
     */
    char line[100];
    uint32_t line_no = 0;
    uint32_t done = EOS_FALSE;

    while(fgets(line, 100, fp) && !done){
        /* Remove newline at the end. */
        strtok(line, "\n");

        /* Follow format above. */
        switch(line_no){
            case 0:
                params -> pims.params.common_params.filter = parse_filter(line);
                eos_logf(EOS_LOG_INFO, "- Filter: %s", line);
                break;

            case 1:
                params -> pims.params.common_params.max_observations = strtol(line, NULL, 10);
                eos_logf(EOS_LOG_INFO, "- Maximum observations for filter: %d", strtol(line, NULL, 10));
                break;

            case 2:
                params -> pims.alg = parse_algorithm(line);
                eos_logf(EOS_LOG_INFO, "- Algorithm chosen: %s", line);
                break;
            
            default:
                done = EOS_TRUE;
        }

        line_no += 1;
    }

    fclose(fp);
    return EOS_SUCCESS;
}
#endif

/* Loads the config for PIMS parameters, using libconfig.
 * Only compiled if libconfig is allowed. */
#ifndef EOS_NO_LIBCONFIG
EosStatus load_libconfig_config(config_ptr config, EosParams* params) {

    /* 'params' must be non-NULL. */
    if(eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }

    /* Start by initializing all parameters to defaults. */
    EosStatus status = eos_init_default_params(params);
    if (status != EOS_SUCCESS) { return status; }

    /* If no config, return early with defaults. */
    if (config == NULL) {
        eos_logf(EOS_LOG_INFO, "No config file provided; using defaults.");
        return EOS_SUCCESS;
    }

    /* Get the root setting (should never be null, but could be empty). */
    config_setting_t* root_setting = config_root_setting(config);
    if(eos_assert(root_setting != NULL)) { return EOS_ASSERT_ERROR; }

    /* Read common parameters. */
    double threshold;
    config_lookup_float(config, "pims.common_params.threshold", &threshold);
    params -> pims.params.common_params.threshold = threshold;
    eos_logf(EOS_LOG_INFO, "- Threshold: %0.2f", threshold);
    
    /* Read the filter. */
    const char* filter = NULL;
    config_lookup_string(config, "pims.common_params.filter", &filter);
    params -> pims.params.common_params.filter = parse_filter(filter);
    eos_logf(EOS_LOG_INFO, "- Filter: %s", filter);

    /* Read the maximum number of observations for the filter */
    int max_observations;
    config_lookup_int(config, "pims.common_params.max_observations", &max_observations);
    params -> pims.params.common_params.max_observations = max_observations;
    eos_logf(EOS_LOG_INFO, "- Maximum observations for filter: %d", max_observations);

    /* Read the algorithm. */
    const char* alg = NULL;
    config_lookup_string(config, "pims.algorithm", &alg);
    params -> pims.alg = parse_algorithm(alg);
    eos_logf(EOS_LOG_INFO, "- Algorithm chosen: %s", alg);

    /* Based on the chosen algorithm, read the other parameters. */
    switch(params -> pims.alg){
        case EOS_PIMS_BASELINE:
            break;

        default:
            eos_log(EOS_LOG_ERROR, "Invalid algorithm!");
            return EOS_VALUE_ERROR;
    }

    return EOS_SUCCESS;
}
#endif

/* Runs the PIMS simulator given an input file. */
EosStatus run_pims_sim(char* inputfile, char* outputfile, config_ptr config) {

    /* Initialize logging to stdout. */
    EosStatus status = EOS_SUCCESS;
    status = sim_log_init("-");
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Initialize 'eos'. */
    EosInitParams init_params;
    status |= default_init_params_pims(&init_params);
    status |= eos_init(&init_params, NULL, 0, log_function);
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Read data file containing observations. */
    uint64_t size;
    void* data = NULL;
    eos_logf(EOS_LOG_INFO, "Reading data file \"%s\"...", inputfile);
    status = read_observation(inputfile, "rb", &data, &size);
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Read parameters NUM_MODES, MAX_BINS, and NUM_OBS, in order to allocate space. */
    eos_logf(EOS_LOG_INFO, "Peeking at data file to see NUM_MODES, MAX_BINS, and NUM_OBS...");
    U32 num_modes, max_bins, num_obs;
    status = eos_pims_observation_attributes(data, size, &num_modes, &max_bins, &num_obs);
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Allocate memory for observations. */
    eos_logf(EOS_LOG_INFO, "Allocating memory according to NUM_MODES = %d, MAX_BINS = %d and NUM_OBS = %d", num_modes, max_bins, num_obs);
    EosPimsObservationsFile obs_file;
    status = init_pims_obs_file(&obs_file, num_modes, max_bins, num_obs);
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Copy over data from file.*/
    eos_logf(EOS_LOG_INFO, "Loading observation from file contents...");
    status = eos_load_pims(data, size, &obs_file);
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Read in parameters.
       If we can't use libconfig, like on VxWorks, then we will use our own
       config parsing logic. */
    EosParams allparams;
    #ifdef EOS_NO_LIBCONFIG
        eos_logf(EOS_LOG_INFO, "Loading parameters from config file using custom parser...");
        status = load_custom_config(config, &allparams);
        if(status != EOS_SUCCESS){
            return status;
        }
    #else
        eos_logf(EOS_LOG_INFO, "Loading parameters from config file using libconfig...");
        status = load_libconfig_config(config, &allparams);
        if(status != EOS_SUCCESS){
            return status;
        }
    #endif

    /* Unpack algorithm and params. */
    EosPimsAlgorithm algorithm = allparams.pims.alg;
    EosPimsAlgorithmParams params = allparams.pims.params;

    /* Fill in MAX_BINS, based on the data file read! */
    params.common_params.max_bins = max_bins;

    /* Print memory usage. */
    U64 memory_needed_bytes = 0;
    memory_needed_bytes = eos_umax(memory_needed_bytes, eos_pims_alg_init_mreq(algorithm, &params));
    memory_needed_bytes = eos_umax(memory_needed_bytes, eos_pims_alg_on_recv_mreq(algorithm, &params));
    eos_logf(EOS_LOG_KEY_VALUE, "Actual memory usage: %lu bytes.", (U32) memory_needed_bytes);

    /* Get request for memory needed for state. */
    EosPimsAlgorithmStateRequest req;
    status = eos_pims_alg_state_request(algorithm, &params, &req);
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Handle request to allocate space for state. */
    EosPimsAlgorithmState state;
    status = sim_pims_handle_state_request(algorithm, &req, &state);
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Expose state to radiation with BITFLIPS! */
    #ifdef EOS_BF_TEST

        /* Enable BITFLIPS for each 'valuable' element in the state. */
        switch(algorithm){
            case EOS_PIMS_BASELINE:

                /* For the last smoothed observation. */
                /* The unsmoothed observations in the queue are already accounted for above. */
                VALGRIND_BITFLIPS_MEM_ON(
                    state.baseline_state.last_smoothed_observation.bin_counts,
                    1, max_bins,
                    BITFLIPS_DTYPE, BITFLIPS_ROW_MAJOR
                );

                break;

            default:
                return EOS_VALUE_ERROR;
        }

    #endif

    /* Initialize algorithm. */
    eos_logf(EOS_LOG_INFO, "Initializing algorithm...");
    eos_pims_alg_init(algorithm, &params, &state);

    /* Start the clock here. */
    eos_logf(EOS_LOG_KEY_VALUE, "Starting the clock!");

    /* To be filled in at each timestep. */
    /* We allocate space on the heap since we might have a large number of observations. */
    F64* scores = malloc(num_obs * sizeof(F64));
    U32* timestamps = malloc(num_obs * sizeof(U32));
    EosPimsDetection result;
    EosPimsObservation curr_obs, prev_obs;

    #ifdef EOS_BF_TEST
        U32 old_obs_index;
        EosPimsObservation old_obs;
    #endif

    /* Loop through observations. */
    eos_logf(EOS_LOG_INFO, "Playing back observations...");
    for(U32 i = 0; i < num_obs; ++i){
        curr_obs = obs_file.observations[i];

        /* Expose observation to radiation with BITFLIPS! */
        #ifdef EOS_BF_TEST

            VALGRIND_BITFLIPS_MEM_ON(
                curr_obs.bin_counts,
                1, curr_obs.num_bins,
                BITFLIPS_DTYPE, BITFLIPS_ROW_MAJOR
            );

        #endif

        /* If we have a mismatch between bin definitions between the current
         * and the just previous observation, reset! */
        if(i > 0 && !check_bin_definitions(curr_obs, prev_obs)){
            eos_logf(EOS_LOG_INFO, "Bin definitions changed for observation with ID %d!", curr_obs.observation_id);
            eos_pims_alg_init(algorithm, &params, &state);
        }

        /* Process the new observation. */
        status = eos_pims_alg_on_recv(curr_obs, &params, &state, &result);
        if(status != EOS_SUCCESS){
            eos_logf(EOS_LOG_ERROR, "Error after observation with ID %d!", curr_obs.observation_id);
            return status;
        }

        /* Save scores and timestamps. */
        scores[i] = result.score;
        timestamps[i] = curr_obs.timestamp;
        prev_obs = curr_obs;

        /* Remove radiation exposure for old observations!
         * This helps speed up the simulation. */
        #ifdef EOS_BF_TEST
            if(i >= params.common_params.max_observations){
                old_obs_index = i - params.common_params.max_observations;
                old_obs = obs_file.observations[old_obs_index];
                VALGRIND_BITFLIPS_MEM_OFF(
                    old_obs.bin_counts
                );
            }
        #endif
    }

    /* Stop the clock now! */
    eos_logf(EOS_LOG_KEY_VALUE, "Stopping the clock!");

    /* Log scores and times to outputfile.
     * As elsewhere, "-" indicates standard output. */
    eos_logf(EOS_LOG_INFO, "Printing scores and times...");
    FILE* fp;
    if(strncmp("-", outputfile, 1) == 0){
        fp = stdout;
    } else {
        fp = fopen(outputfile, "w");
    }

    if(fp == NULL){
        eos_logf(EOS_LOG_DEBUG, "Could not open %s.", outputfile);
        return EOS_PIMS_LOAD_ERROR;
    }

    fprintf(fp, "Scores,Timestamps\n");
    for(U32 i = 0; i < num_obs; ++i){
        fprintf(fp, "%0.4f,%d\n", scores[i], timestamps[i]);
    }

    /* Don't close stdout, because the logging goes there! */
    if(fp != stdout){
        fclose(fp);
    }

    /* Disable BITFLIPS! Otherwise, SIGSEV awaits when we free(). */
    #ifdef EOS_BF_TEST
        for(U32 obs = 0; obs < obs_file.num_observations; ++obs) {
            EosPimsObservation observation = obs_file.observations[obs];
            VALGRIND_BITFLIPS_MEM_OFF(
                observation.bin_counts
            );
        }

        switch(algorithm){
            case EOS_PIMS_BASELINE:
                VALGRIND_BITFLIPS_MEM_OFF(
                    state.baseline_state.last_smoothed_observation.bin_counts
                );
                break;

            default:
                return EOS_VALUE_ERROR;
        }
    #endif

    /* Free memory. */
    eos_logf(EOS_LOG_INFO, "Simulation over! Tearing down...");
    free(scores);
    free(timestamps);
    status = teardown_pims_sim(&data, algorithm, &obs_file, &state);
    return status;
}
