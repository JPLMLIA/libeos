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

#define N_RESULTS_DEFAULT 0

static EosStatus _extract_int(config_setting_t *group, const char *name,
        int32_t *value, const int64_t min_value, const int64_t max_value) {

    if (eos_assert(group != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(name != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(value != NULL)) { return EOS_ASSERT_ERROR; }

    config_setting_t *setting;
    int32_t v;

    setting = config_setting_get_member(group, name);
    if (setting == NULL) {
        eos_logf(EOS_LOG_WARN, "No \"%s\" specified; using default", name);
        return EOS_SUCCESS;
    }

    // Check that the value is a number
    if (!config_setting_is_number(setting)) {
        eos_logf(EOS_LOG_ERROR,
            "The \"%s\" setting should be a number", name
        );
        return EOS_ERROR;
    }

    // Switch depending on entry type
    switch(config_setting_type(setting)) {
        case CONFIG_TYPE_INT64:
            v = (int32_t) config_setting_get_int64(setting);
            break;
        case CONFIG_TYPE_INT:
            v = config_setting_get_int(setting);
            break;
        default:
            eos_logf(EOS_LOG_ERROR,
                "Unexpected type for \"%s\" array entries (expected int)",
                name
            );
            return EOS_ERROR;
    }

    // Value range check
    if ((v < min_value) || (v > max_value)) {
        eos_logf(EOS_LOG_ERROR,
            "Value %s = %d not in range [%ld, %ld]",
            name, v, min_value, max_value
        );
        return EOS_ERROR;
    }

    value[0] = v;

    // Finally, remove the entry from the group
    if (config_setting_remove(group, name) == CONFIG_TRUE) {
        return EOS_SUCCESS;
    } else {
        return EOS_ERROR;
    }
}

static EosStatus load_config(config_t *config, EosParams *params,
        EosMiseDetectionResult *result) {

    EosStatus status;
    int32_t value;
    uint32_t v, count;
    config_setting_t *root_setting;

    // These parameters must be non-NULL
    if(eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }
    if(eos_assert(result != NULL)) { return EOS_ASSERT_ERROR; }

    // Start by initializing all parameters to defaults
    status = eos_init_default_params(params);
    if (status != EOS_SUCCESS) { return status; }

    result->n_results = N_RESULTS_DEFAULT;

    // If no config, return early with defaults
    if (config == NULL) {
        eos_logf(EOS_LOG_INFO, "No config file provided; using defaults.");
        return EOS_SUCCESS;
    }

    // Get the root setting (should never be null, but could be empty)
    root_setting = config_root_setting(config);
    if(eos_assert(root_setting != NULL)) { return EOS_ASSERT_ERROR; }

    value = result->n_results; // Store default
    status = _extract_int(
        root_setting, "n_results",
        &(value), 0, UINT32_MAX
    );
    eos_logf(EOS_LOG_INFO, "Result: %d", value);
    if (status != EOS_SUCCESS) { return status; }
    result->n_results = value; // Re-assign (potentially modified) value

    count = config_setting_length(root_setting);
    for (v = 0; v < count; v++) {
        config_setting_t *member = config_setting_get_elem(root_setting, v);
        if (eos_assert(member != NULL)) { return EOS_ASSERT_ERROR; }

        const char *name = config_setting_name(member);
        if (eos_assert(name != NULL)) { return EOS_ASSERT_ERROR; }

        eos_logf(EOS_LOG_WARN, "Ignoring extraneous config setting \"%s\"", name);
    }

    return EOS_SUCCESS;
}

EosStatus run_mise_sim(char *inputfile, char *outputfile, config_t *config) {

    EosStatus status;
    void* data = NULL;
    void* stack = NULL;
    uint64_t size;
    EosParams params;
    EosMiseObservation obs = {0, 0, {0}, 0};
    EosMiseDetectionResult result = {0, 0};

    status = sim_log_init(outputfile);
    if (status != EOS_SUCCESS) { return status; }

    #define CHECK_STATUS(status) do { \
        if (status != EOS_SUCCESS) { \
            eos_logf(EOS_LOG_ERROR, "Error code %d", status);\
            teardown_mise_sim(&data, &stack, &obs, &result);\
            return status;\
        } \
    } while(0)

    log_function(EOS_LOG_INFO, "Initializing library...");
    EosInitParams init_params;
    default_init_params(&init_params);

    // Allocate memory for EOS stack
    uint64_t required_mem = eos_memory_requirement(&init_params);
    stack = malloc((size_t) required_mem);

    status = eos_init(&init_params, stack, required_mem, log_function);
    CHECK_STATUS(status);
    eos_logf(EOS_LOG_INFO, "Initialized library with %ld bytes", required_mem);

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

    eos_logf(EOS_LOG_INFO, "Loaded MISE data: %d x %d, %d bands",
             obs.shape.rows, obs.shape.cols, obs.shape.bands);

    eos_logf(EOS_LOG_INFO, "Loading configuration...");
    status = load_config(config, &params, &result);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Checking parameters");
    status = params_check(&params);
    CHECK_STATUS(status);

    status = init_mise_result(&result);
    CHECK_STATUS(status);

#ifdef EOS_BF_TEST
    // Treat observation as a 2-D array
    VALGRIND_BITFLIPS_MEM_ON(
        obs.data,
        obs.shape.rows,
        obs.shape.cols * obs.shape.bands,
        BITFLIPS_USHORT, BITFLIPS_ROW_MAJOR
    );

    // Treat stack as a byte array
    VALGRIND_BITFLIPS_MEM_ON(
        stack, 1, required_mem,
        BITFLIPS_UCHAR, BITFLIPS_ROW_MAJOR
    );

    // Treat results as a byte array
    VALGRIND_BITFLIPS_MEM_ON(
        result.results, 1,
        sizeof(EosPixelDetection) * result.n_results,
        BITFLIPS_UCHAR, BITFLIPS_ROW_MAJOR
    );
#endif

    eos_logf(EOS_LOG_INFO, "Running detection...");
    status = eos_mise_detect_anomaly(&params.mise, &obs, &result);
    eos_logf(EOS_LOG_INFO, "Detection completed with status %d", status);
    CHECK_STATUS(status);

#ifdef EOS_BF_TEST
    // Disable BITFLIPS for data, stack, and results
    VALGRIND_BITFLIPS_MEM_OFF(obs.data);
    VALGRIND_BITFLIPS_MEM_OFF(stack);
    VALGRIND_BITFLIPS_MEM_OFF(result.results);
#endif

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
    return teardown_mise_sim(&data, &stack, &obs, &result);
}

