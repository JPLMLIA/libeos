#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <eos.h>
#include <eos_log.h>
#include <eos_params.h>

#ifdef EOS_BF_TEST
#include <valgrind/bitflips.h>
#endif

#include "sim_io.h"
#include "sim_log.h"
#include "sim_util.h"
#include "sim_mise.h"

//#define N_RESULTS_DEFAULT 0
#define N_RESULTS_DEFAULT 10

/* TODO: add function for loading a MISE config file */

EosStatus run_mise_sim(char *inputfile, char *outputfile) {
    //        config_t *config) {

    EosStatus status;
    void* data = NULL;
    uint64_t size;
    EosMiseObservation obs = {0, 0, {0}, 0};
    EosMiseDetectionResult result = {0, 0};

    status = sim_log_init(outputfile);
    if (status != EOS_SUCCESS) { return status; }

    #define CHECK_STATUS(status) do { \
        if (status != EOS_SUCCESS) { \
            eos_logf(EOS_LOG_ERROR, "Error code %d", status);\
            teardown_mise_sim(&data, &obs, &result);\
            return status;\
        } \
    } while(0)

    log_function(EOS_LOG_INFO, "Initializing library...");
    EosInitParams init_params;
    init_params.mise_max_bands = EOS_MISE_N_BANDS;
    status = eos_init(&init_params, NULL, 0, log_function);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Reading observation from \"%s\"", inputfile);
    status = read_observation(inputfile, "rb", &data, &size);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Loading MISE observation from file contents (size %d)", size);
    init_mise_obs(&obs, size / sizeof(uint16_t));
    status = eos_load_mise(data, size, &obs);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO,
        "Successfully loaded observation %d (time stamp: %d)",
        obs.observation_id, obs.timestamp
    );

    /* TODO: Load config and run detection */
    eos_logf(EOS_LOG_INFO, "Loaded MISE data: %d x %d, %d bands\n", 
             obs.shape.rows, obs.shape.cols, obs.shape.bands);

    /*
    uint16_t i, j, b;
    for (i = 0; i < obs.shape.rows; i++) {
        for (j = 0; j < obs.shape.cols; j++) {
            for (b = 0; b < obs.shape.bands; b++)
                printf("%d ", obs.data[i*obs.shape.cols*obs.shape.bands +
                                       j*obs.shape.bands + 
                                       b]);
            printf(", ");
        }
        printf("\n");
    }
    */

    EosParams params;
    /* todo: load config from a file */
    /*
    status = load_config(config, &params, &result);
    CHECK_STATUS(status);
    */
    /* for now just hard-code */
    // Start by initializing all parameters to defaults
    status = eos_init_default_params(&params);
    CHECK_STATUS(status);
    // Default mise.alg is RX
    status = params_check(&params);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Checking parameters");
    status = params_check(&params);
    CHECK_STATUS(status);

    result.n_results = N_RESULTS_DEFAULT;

    status = init_mise_result(&result);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Running detection...");
    status = eos_mise_detect_anomaly(&params.mise, &obs, &result);
    eos_logf(EOS_LOG_INFO, "Detection completed with status %d", status);
    CHECK_STATUS(status);

    int j, n_det; 
    eos_logf(EOS_LOG_INFO, "Detection results");
    n_det = result.n_results;
    eos_logf(EOS_LOG_INFO, "%d detections", n_det);
    for (j = 0; j < n_det; j++) {
        EosPixelDetection det = result.results[j];
        eos_logf(EOS_LOG_KEY_VALUE,
                 "{ \"rank\": %d, \"row\": %u, \"col\": %u, \"score\": %f }",
                 j, det.row, det.col, det.score);
    }

    eos_logf(EOS_LOG_INFO, "Simulation successful.");
    eos_logf(EOS_LOG_INFO, "Tear-down library...");
    return teardown_mise_sim(&data, &obs, &result);
}

