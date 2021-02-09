#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <eos.h>
#include <eos_log.h>

#include "sim_io.h"
#include "sim_log.h"
#include "sim_util.h"

const static uint16_t THRESHOLD = 1;

EosStatus run_ethemis_sim(const char* inputfile, const char* outputfile,
        const int k) {

    EosStatus status;
    uint64_t size;
    EosEthemisBand band;
    EosParams params;
    EosInitParams init_params;

    void* data = NULL;
    EosEthemisObservation obs = {0};
    EosEthemisDetectionResult result = {{0},{0}};

    status = sim_log_init(outputfile);
    if (status != EOS_SUCCESS) { return status; }

    #define CHECK_STATUS(status) do { \
        if (status != EOS_SUCCESS) { \
            eos_logf(EOS_LOG_ERROR, "Error code %d", status);\
            teardown_ethemis_sim(&data, &obs, &result);\
            return status;\
        } \
    } while(0)

    log_function(EOS_LOG_INFO, "Initializing library...");
    default_init_params(&init_params);
    status = eos_init(&init_params, NULL, 0, log_function);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Reading observation from \"%s\"", inputfile);
    status = read_observation(inputfile, "rb", &data, &size);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Initialize observation data structure");
    status = init_ethemis_obs(&obs, size / sizeof(uint16_t));
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Loading observation from file contents");
    status = eos_load_etm(data, size, &obs);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO,
        "Successfully loaded observation %d (time stamp: %d)",
        obs.observation_id, obs.timestamp
    );

    eos_logf(EOS_LOG_INFO, "Loading configuration");
    status = eos_init_default_params(&params);
    CHECK_STATUS(status);

    // How many results we want for each band
    for (band = EOS_ETHEMIS_BAND_1; band < EOS_ETHEMIS_N_BANDS; band++) {
        result.n_results[band] = k;
    }

    // Set thresholds
    for (band = EOS_ETHEMIS_BAND_1; band < EOS_ETHEMIS_N_BANDS; band++) {
        params.ethemis.band_threshold[band] = THRESHOLD;
        eos_logf(EOS_LOG_INFO, "band_threshold[%d] = %u",
            band, params.ethemis.band_threshold[band]
        );
    }

    eos_logf(EOS_LOG_INFO, "Initialize result data structure");
    status = init_ethemis_result(&result);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Running detection...");
    status = eos_ethemis_detect_anomaly(&params.ethemis, &obs, &result);
    eos_logf(EOS_LOG_INFO, "Detection completed with status %d", status);
    CHECK_STATUS(status);

    for (band = EOS_ETHEMIS_BAND_1; band < EOS_ETHEMIS_N_BANDS; band++) {
        int j, n_det = result.n_results[band];
        eos_logf(EOS_LOG_INFO, "Band %d (%d detections)", band + 1, n_det);
        for (j = 0; j < n_det; j++) {
            EosPixelDetection det = result.band_results[band][j];
            eos_logf(EOS_LOG_KEY_VALUE,
                "{ \"band\": %d, \"rank\": %d, \"row\": %u, "
                "\"col\": %u, \"dn\": %u }",
                band + 1, j, det.row, det.col, (uint32_t) det.score
            );
        }
    }

    eos_logf(EOS_LOG_INFO, "Simulation successful.");
    eos_logf(EOS_LOG_INFO, "Tear-down library...");
    return teardown_ethemis_sim(&data, &obs, &result);
}
