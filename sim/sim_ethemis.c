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
#include "sim_ethemis.h"

#define N_RESULTS_DEFAULT 0

static EosStatus _extract_int_array(config_setting_t *group, const char *name,
        int64_t *values, uint32_t *n_values,
        const int64_t min_value, const int64_t max_value) {

    if (eos_assert(group != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(name != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(values != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(n_values != NULL)) { return EOS_ASSERT_ERROR; }

    config_setting_t *setting, *entry;
    uint32_t count, i;
    uint32_t n_val = *n_values;

    setting = config_setting_get_member(group, name);
    if (setting == NULL) {
        eos_logf(EOS_LOG_WARN, "No \"%s\" specified; using default", name);
        *n_values = 0;
        return EOS_SUCCESS;
    }

    // Check that the value is an array
    if (!config_setting_is_array(setting)) {
        eos_logf(EOS_LOG_ERROR,
            "The \"%s\" setting should be an array of size %u",
            name, n_val
        );
        return EOS_ERROR;
    }

    // Check that there are an expected number of entries
    count = config_setting_length(setting);
    if (count != n_val) {
        eos_logf(EOS_LOG_ERROR,
            "Got %u settings for \"%s\" instead of %u",
            count, name, n_val
        );
        return EOS_ERROR;
    }

    // Parse values
    for (i = 0; i < n_val; i++) {
        entry = config_setting_get_elem(setting, i);
        if(eos_assert(entry != NULL)) { return EOS_ASSERT_ERROR; }
        int64_t value;

        // Switch depending on entry type
        switch(config_setting_type(entry)) {
            case CONFIG_TYPE_INT64:
                value = config_setting_get_int64(entry);
                break;
            case CONFIG_TYPE_INT:
                value = (int64_t) config_setting_get_int(entry);
                break;
            default:
                eos_logf(EOS_LOG_ERROR,
                    "Unexpected type for \"%s\" array entries (expected int)",
                    name
                );
                return EOS_ERROR;
        }

        // Value range check
        if ((value < min_value) || (value > max_value)) {
            eos_logf(EOS_LOG_ERROR,
                "Value %s[%u] = %ld not in range [%ld, %ld]",
                name, i, value, min_value, max_value
            );
            return EOS_ERROR;
        }

        values[i] = value;
    }

    // Finally, remove the entry from the group
    if (config_setting_remove(group, name) == CONFIG_TRUE) {
        return EOS_SUCCESS;
    } else {
        return EOS_ERROR;
    }
}

static EosStatus load_config(config_t *config, EosParams *params,
        EosEthemisDetectionResult *result) {

    EosStatus status;
    EosEthemisBand band;
    int64_t values[EOS_ETHEMIS_N_BANDS];
    uint32_t n_values, v, count;
    config_setting_t *root_setting;

    eos_logf(EOS_LOG_INFO, "Loading configuration...");

    // These parameters must be non-NULL
    if(eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }
    if(eos_assert(result != NULL)) { return EOS_ASSERT_ERROR; }

    // Start by initializing all parameters to defaults
    status = eos_init_default_params(params);
    if (status != EOS_SUCCESS) { return status; }

    for (band = EOS_ETHEMIS_BAND_1; band < EOS_ETHEMIS_N_BANDS; band++) {
        result->n_results[band] = N_RESULTS_DEFAULT;
    }

    // If no config, return early with defaults
    if (config == NULL) {
        eos_logf(EOS_LOG_INFO, "No config file provided; using defaults.");
        return EOS_SUCCESS;
    }

    // Get the root setting (should never be null, but could be empty)
    root_setting = config_root_setting(config);
    if(eos_assert(root_setting != NULL)) { return EOS_ASSERT_ERROR; }

    // Get band threshold settings
    n_values = EOS_ETHEMIS_N_BANDS;
    status = _extract_int_array(
        root_setting, "band_threshold",
        values, &n_values, 0, UINT16_MAX
    );
    if (status != EOS_SUCCESS) { return status; }
    for (v = EOS_ETHEMIS_BAND_1; v < EOS_ETHEMIS_N_BANDS; v++) {
        if (v < n_values) {
            params->ethemis.band_threshold[v] = values[v];
        }
        eos_logf(EOS_LOG_INFO, "band_threshold[%d] = %u",
            v, params->ethemis.band_threshold[v]
        );
    }

    // Get n_results settings
    n_values = EOS_ETHEMIS_N_BANDS;
    status = _extract_int_array(
        root_setting, "n_results",
        values, &n_values, 0, UINT32_MAX
    );
    if (status != EOS_SUCCESS) { return status; }
    for (v = EOS_ETHEMIS_BAND_1; v < EOS_ETHEMIS_N_BANDS; v++) {
        if (v < n_values) {
            result->n_results[v] = values[v];
        }
        eos_logf(EOS_LOG_INFO, "n_results[%d] = %u",
            v, result->n_results[v]
        );
    }

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

EosStatus run_ethemis_sim(char *inputfile, char *outputfile,
        config_t *config) {

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
    init_params.mise_max_bands = EOS_MISE_N_BANDS;
    status = eos_init(&init_params, NULL, 0, log_function);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Reading observation from \"%s\"", inputfile);
    status = read_observation(inputfile, "rb", &data, &size);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Loading observation from file contents");
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
    status = load_config(config, &params, &result);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Checking parameters");
    status = params_check(&params);
    CHECK_STATUS(status);

    eos_logf(EOS_LOG_INFO, "Initialize result data structure");
    status = init_ethemis_result(&result);
    CHECK_STATUS(status);

#ifdef EOS_BF_TEST
    // Enable BITFLIPS for band data and results
    for (band = EOS_ETHEMIS_BAND_1; band < EOS_ETHEMIS_N_BANDS; band++) {
        VALGRIND_BITFLIPS_MEM_ON(
            obs.band_data[band],
            obs.band_shape[band].rows,
            obs.band_shape[band].cols,
            BITFLIPS_USHORT, BITFLIPS_ROW_MAJOR
        );

        // Treat results as a byte array
        VALGRIND_BITFLIPS_MEM_ON(
            result.band_results[band],
            1, sizeof(EosPixelDetection) * result.n_results[band],
            BITFLIPS_UCHAR, BITFLIPS_ROW_MAJOR
        );
    }
#endif

    eos_logf(EOS_LOG_INFO, "Running detection...");
    status = eos_ethemis_detect_anomaly(&params.ethemis, &obs, &result);
    eos_logf(EOS_LOG_INFO, "Detection completed with status %d", status);
    CHECK_STATUS(status);

#ifdef EOS_BF_TEST
    // Disable BITFLIPS for band data and results
    for (band = EOS_ETHEMIS_BAND_1; band < EOS_ETHEMIS_N_BANDS; band++) {
        VALGRIND_BITFLIPS_MEM_OFF(obs.band_data[band]);
        VALGRIND_BITFLIPS_MEM_OFF(result.band_results[band]);
    }
#endif

    eos_logf(EOS_LOG_INFO, "Detection results");
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
