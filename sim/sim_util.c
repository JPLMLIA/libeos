/*
 * Utility functions for simulator setup/teardown; these functions assume that
 * EOS and simulation logging have been initialized prior to being called.
 */
#include <stdlib.h>

#include <eos.h>
#include <eos_log.h>

#include "sim_log.h"
#include "sim_util.h"

EosStatus init_ethemis_obs(EosEthemisObservation *obs, const int size) {
    obs->observation_id = 0;
    obs->timestamp = 0;

    EosEthemisBand band;
    for (band = EOS_ETHEMIS_BAND_1; band < EOS_ETHEMIS_N_BANDS; band++) {
        obs->band_shape[band].rows = 1;
        obs->band_shape[band].cols = size;
        obs->band_shape[band].bands = 1; /* one band at a time */
        obs->band_data[band] = (uint16_t*) malloc(sizeof(uint16_t) * size);
        if (obs->band_data[band] == NULL) {
            eos_logf(EOS_LOG_ERROR,
                "Error allocating data for band %d\n", band);
            return EOS_ERROR;
        }
    }
    return EOS_SUCCESS;
}

void free_ethemis_obs(EosEthemisObservation *obs) {
    EosEthemisBand band;
    for (band = EOS_ETHEMIS_BAND_1; band < EOS_ETHEMIS_N_BANDS; band++) {
        if (obs->band_data[band]) {
            free(obs->band_data[band]);
            obs->band_data[band] = NULL;
        }
    }
}

/*
 * This function initializes the data structure for holding E-THEMIS deteciton
 * results. It assumes the `n_results` fields have been populated with the
 * desired sizes for each band.
 */
EosStatus init_ethemis_result(EosEthemisDetectionResult *result) {
    EosEthemisBand band;
    if (eos_assert(result != NULL)) { return EOS_ASSERT_ERROR; }
    for (band = EOS_ETHEMIS_BAND_1; band < EOS_ETHEMIS_N_BANDS; band++) {
        result->band_results[band] = (EosPixelDetection*) malloc(
            sizeof(EosPixelDetection) * result->n_results[band]
        );
        if (result->band_results[band] == NULL) {
            eos_logf(
                EOS_LOG_ERROR,
                "Error allocating data for band %d\n", band
            );
            return EOS_ERROR;
        }
    }
    return EOS_SUCCESS;
}

void free_ethemis_result(EosEthemisDetectionResult *result) {
    EosEthemisBand band;
    for (band = EOS_ETHEMIS_BAND_1; band < EOS_ETHEMIS_N_BANDS; band++) {
        if (result->band_results[band]) {
            free(result->band_results[band]);
            result->band_results[band] = NULL;
        }
    }
}

/*
 * Completely teardown the E-THEMIS sim including freeing data structures and
 * tearing down the sim logging and EOS library.
 */
EosStatus teardown_ethemis_sim(void **data, EosEthemisObservation *obs,
        EosEthemisDetectionResult *result) {

    EosStatus status;

    free_ethemis_result(result);
    free_ethemis_obs(obs);
    if (*data != NULL) {
        free(*data);
        *data = NULL;
    }

    status = eos_teardown();
    sim_log_teardown();
    return status;
}

EosStatus init_mise_obs(EosMiseObservation *obs, const int size) {
    obs->observation_id = 0;
    obs->timestamp = 0;

    /* Allocate memory for data equal to the file size */
    obs->data = (uint16_t*) malloc(sizeof(uint16_t) * size);
    if (obs->data == NULL) {
        eos_logf(EOS_LOG_ERROR,
            "Error allocating data for MISE observation.");
        return EOS_ERROR;
    }

    /* We don't yet know the dimensions, just the total size,
     * so these are placeholder values */ 
    obs->shape.cols = 1;
    obs->shape.rows = 1;
    obs->shape.bands = 1;

    return EOS_SUCCESS;
}

void free_mise_obs(EosMiseObservation *obs) {
    if (obs->data) {
        free(obs->data);
        obs->data = NULL;
    }
}

/*
 * Result n_results should already be populated with the desired sizes
 */
EosStatus init_mise_result(EosMiseDetectionResult *result) {
    if (eos_assert(result != NULL)) { return EOS_ASSERT_ERROR; }
    result->results = (EosPixelDetection*) malloc(
            sizeof(EosPixelDetection) * result->n_results);
    if (result->results == NULL) {
        eos_logf(EOS_LOG_ERROR,
            "Error allocating data for MISE results");
        return EOS_ERROR;
    }
    return EOS_SUCCESS;
}

void free_mise_result(EosMiseDetectionResult *result) {
    if (result->results) {
        free(result->results);
        result->results = NULL;
    }
}

/*
 * Completely teardown the MISE sim including freeing data structures and
 * tearing down the sim logging and EOS library.
 */
EosStatus teardown_mise_sim(void **data, EosMiseObservation *obs,
        EosMiseDetectionResult *result) {

    EosStatus status;

    free_mise_result(result);
    free_mise_obs(obs);
    if (*data != NULL) {
        free(*data);
        *data = NULL;
    }

    status = eos_teardown();
    sim_log_teardown();
    return status;
}
