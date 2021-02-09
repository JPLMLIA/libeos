#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <eos.h>
#include <eos_log.h>

#include "sim_io.h"
#include "sim_log.h"
#include "sim_util.h"

EosStatus run_mise_sim(const char* inputfile, const char* outputfile,
        const int k) {

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

    eos_logf(EOS_LOG_INFO,
        "Loading MISE observation from file contents (size %d)", size);
    status = init_mise_obs(&obs, size / sizeof(uint16_t));
    CHECK_STATUS(status);
    status = eos_load_mise(data, size, &obs);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO,
        "Successfully loaded observation %d (time stamp: %d)",
        obs.observation_id, obs.timestamp
    );
    eos_logf(EOS_LOG_INFO, "Loaded MISE data: %d x %d, %d bands",
             obs.shape.rows, obs.shape.cols, obs.shape.bands);

    //eos_logf(EOS_LOG_INFO, "Loading configuration");
    //status = eos_init_default_params(&params);
    //CHECK_STATUS(status);
    eos_logf(EOS_LOG_INFO, "Loading configuration...");
    status = eos_init_default_params(&params);
    CHECK_STATUS(status);
    result.n_results = k;

    eos_logf(EOS_LOG_INFO, "Checking parameters");
    status = params_check(&params);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Initialize result data structure");
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
    return teardown_mise_sim(&data, &stack, &obs, &result);
}
