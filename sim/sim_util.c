/*
 * Utility functions for simulator setup/teardown; these functions assume that
 * EOS and simulation logging have been initialized prior to being called.
 */
#include <stdlib.h>

#include <eos.h>
#include <eos_log.h>

#include "sim_log.h"
#include "sim_util.h"

void default_init_params(EosInitParams *init_params) {
    if (init_params == NULL) { return; }
    init_params->mise_max_bands = EOS_MISE_N_BANDS;
}

/* Private function prototypes. */
EosStatus _handle_baseline_state_request(EosPimsBaselineStateRequest* req, EosPimsBaselineState* state);
EosStatus _handle_baseline_state_teardown(EosPimsBaselineState* state);

/**** E-THEMIS ****/

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

/**** MISE ****/

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
EosStatus teardown_mise_sim(void **data, void **stack,
        EosMiseObservation *obs, EosMiseDetectionResult *result) {

    EosStatus status;

    free_mise_result(result);
    free_mise_obs(obs);
    if (*data != NULL) {
        free(*data);
        *data = NULL;
    }
    if (*stack != NULL) {
        free(*stack);
        *stack = NULL;
    }

    status = eos_teardown();
    sim_log_teardown();
    return status;
}


/**** PIMS ****/

/* Allocate memory for structs and arrays required for an EosPimsObservationsFile. */
EosStatus init_pims_obs_file(EosPimsObservationsFile* obs_file, U32 num_modes, U32 max_bins, U32 num_obs){

    /* Memory for the array of EosPimsModeInfo. */
    obs_file -> modes_info = malloc(sizeof(EosPimsModeInfo) * num_modes);
    if(obs_file -> modes_info == NULL){
        return EOS_INSUFFICIENT_MEMORY;
    }

    /* Memory for each EosPimsModeInfo. */
    for(U32 mode = 0; mode < num_modes; ++mode){
        obs_file -> modes_info[mode].bin_log_energies = malloc(sizeof(F32) * max_bins);
        if(obs_file -> modes_info[mode].bin_log_energies == NULL){
            return EOS_INSUFFICIENT_MEMORY;
        }
    }

    /* Memory for the array of observations. */
    obs_file -> observations = malloc(sizeof(EosPimsObservation) * num_obs);
    if(obs_file -> observations == NULL){
        return EOS_INSUFFICIENT_MEMORY;
    }

    /* Memory for all observation's counts, allocated as a contiguous block. */
    pims_count_t* counts_base_ptr = malloc(sizeof(pims_count_t) * max_bins * num_obs);
    if(counts_base_ptr == NULL){
        return EOS_INSUFFICIENT_MEMORY;
    }

    /* Individual observation's counts point to regions within the block. */
    pims_count_t* counts_ptr = counts_base_ptr;
    for(U32 obs = 0; obs < num_obs; ++obs){
        obs_file -> observations[obs].bin_counts = counts_ptr;
        counts_ptr += max_bins;
    }

    return EOS_SUCCESS;
}

/* 'handle_state_teardown()' router for different algorithms. */
EosStatus sim_pims_handle_state_teardown(EosPimsAlgorithm algorithm, EosPimsAlgorithmState* state){
    switch(algorithm){
        case EOS_PIMS_BASELINE:
            return _handle_baseline_state_teardown(&(state -> baseline_state));

        default:
            return EOS_VALUE_ERROR;
    }
}

/* 'handle_state_request()' router for different algorithms. */
EosStatus sim_pims_handle_state_request(EosPimsAlgorithm algorithm, EosPimsAlgorithmStateRequest* req, EosPimsAlgorithmState* state){
    switch(algorithm){
        case EOS_PIMS_BASELINE:
            return _handle_baseline_state_request(&(req -> baseline_req), &(state -> baseline_state));

        default:
            return EOS_VALUE_ERROR;
    }
}

/* Allocates memory for 'baseline'. */
EosStatus _handle_baseline_state_request(EosPimsBaselineStateRequest* req, EosPimsBaselineState* state){
    if(req -> queue_size == 0){
        return EOS_VALUE_ERROR;
    }

    /* 
     * We need space for (n + 1) observations, if we need to store n observations.
     * This is because of the semantics of our circular queue.
     */ 
    state -> queue.max_size = req -> queue_size;
    state -> queue.observations = malloc(sizeof(EosPimsObservation) * (req -> queue_size + 1));

    if(state -> queue.observations == NULL){
        return EOS_INSUFFICIENT_MEMORY;
    }

    /* Allocate space for holding observations for smoothing. */
    state -> last_smoothed_observation.bin_counts = malloc(sizeof(pims_count_t) * req -> max_bins);
    if(state -> last_smoothed_observation.bin_counts == NULL){
        return EOS_INSUFFICIENT_MEMORY;
    }

    return EOS_SUCCESS;
}

/* Frees memory for 'baseline'. */
EosStatus _handle_baseline_state_teardown(EosPimsBaselineState* state){
    if(state == NULL){
        return EOS_SUCCESS;
    }

    if(state -> queue.observations != NULL){
        free(state -> queue.observations);
    }

    /* We free only the counts, because the log-energies
     * just point to arrays not managed by us. */
    if(state -> last_smoothed_observation.bin_counts != NULL){
        free(state -> last_smoothed_observation.bin_counts);
    }

    return EOS_SUCCESS;
}

/* Frees the memory allocated for an EosPimsObservationsFile. */
void free_pims_obs_file(EosPimsObservationsFile* obs_file){
    for(U32 mode = 0; mode < obs_file -> num_modes; ++mode){
        free(obs_file -> modes_info[mode].bin_log_energies);
    }
    free(obs_file -> modes_info);

    /* We only have to free the first observation's bin counts, since they
       are all contiguous. */
    free(obs_file -> observations[0].bin_counts);
    free(obs_file -> observations);
}

/*
 * Completely teardown the PIMS sim including freeing data structures and
 * tearing down the sim logging and EOS library.
 */
EosStatus teardown_pims_sim(void **data, EosPimsAlgorithm algorithm, EosPimsObservationsFile *obs_file,
        EosPimsAlgorithmState* state) {

    /* Free state. */
    EosStatus status = sim_pims_handle_state_teardown(algorithm, state);
    if(status != EOS_SUCCESS){
        return status;
    }
    
    /* Free observations. */
    free_pims_obs_file(obs_file);

    /* Free memory allocated to read observations. */
    if (*data != NULL) {
        free(*data);
        *data = NULL;
    }

    /* Teardown 'eos'. */
    status = eos_teardown();

    /* Teardown logging structure. */
    sim_log_teardown();

    return status;
}