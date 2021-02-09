#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "eos.h"
#include "eos_log.h"
#include "eos_types.h"
#include "eos_util.h"
#include "eos_memory.h"
#include "eos_params.h"
#include "eos_ethemis.h"  /* Thermal anomaly detection for E-THEMIS */
#include "eos_mise.h"     /* Spectral anomaly detection for MISE */
#include "eos_pims.h"     /* Time-series anomaly detection for PIMS */
#include "eos_data.h"

static I32 EOS_IS_INITIALIZED = EOS_FALSE;
static EosInitParams init_params;

static U64 _eos_ethemis_detect_anomaly_mreq(const EosInitParams* params);
static U64 _eos_mise_detect_anomaly_mreq(const EosInitParams* params);
static U64 _eos_pims_detect_anomaly_mreq(const EosInitParams* params);

/*
 * This function should compute the maximum memory required by any public
 * function that can be called in the EOS library with the EosInitParams.
 */
uint64_t eos_memory_requirement(const EosInitParams* params) {
    U64 call_size = 0;
    U64 padding;
    if (eos_assert(params != NULL)) { return 0; }

    call_size = eos_lmax(call_size, _eos_ethemis_detect_anomaly_mreq(params));
    call_size = eos_lmax(call_size, _eos_mise_detect_anomaly_mreq(params));
    call_size = eos_lmax(call_size, _eos_pims_detect_anomaly_mreq(params));

    // Potential extra memory required by alignment bytes for each allocation
    padding = EOS_MEMORY_STACK_MAX_DEPTH * ALIGN_SIZE;

    return call_size + padding;
}

EosStatus eos_init_default_params(EosParams* params) {
    EosStatus status;

    if (eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }

    status = params_init_default(params);
    if (status != EOS_SUCCESS) { return status; }

    return EOS_SUCCESS;
}

/*
 * Initialize the library:

 * params: indicate the extreme parameter values with which library
 * functions might be called.

 * initial_memory_ptr: a pointer to pre-allocated memory or NULL (in which case
 * malloc will be called once during initialization)
 *
 * initial_memory_size: the size of the pre-allocated memory, or 0 if
 * `initial_memory_ptr` is NULL
 *
 * log_function: pointer to a custom logging function
 */
EosStatus eos_init(const EosInitParams* params,
                   void *initial_memory_ptr, uint64_t initial_memory_size,
                   void (*log_function)(EosLogType, const char*)) {

    EosStatus status;
    U64 required_nbytes;

    if (eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }

    if (EOS_IS_INITIALIZED) {
        eos_log(EOS_LOG_INFO, "Tearing down prior EOS initialization.");
        status = eos_teardown();
        if (status != EOS_SUCCESS) { return status; }
    }

    log_init(log_function);

    required_nbytes = eos_memory_requirement(params);
    status = memory_init(initial_memory_ptr, initial_memory_size, required_nbytes);
    if (status != EOS_SUCCESS) {
        eos_log(EOS_LOG_ERROR, "Memory initialization failed.");
        return status;
    } else {
        eos_log(EOS_LOG_INFO, "Memory initialization successful.");
    }
    init_params = *params;
    EOS_IS_INITIALIZED = EOS_TRUE;
    return EOS_SUCCESS;
}

/*
 * Teardown (un-initialize) the library
 */
EosStatus eos_teardown() {
    EOS_IS_INITIALIZED = EOS_FALSE;
    memory_teardown();
    log_teardown();
    return EOS_SUCCESS;
}

/*
 * Helper function to be called before any library function that requires
 * initialization.
 */
EosStatus _eos_before() {
    if (EOS_IS_INITIALIZED == EOS_FALSE) {
        eos_log(EOS_LOG_ERROR, "EOS is not initialized.");
        return EOS_NOT_INITIALIZED;
    }
    lifo_stack_clear();
    return EOS_SUCCESS;
}

/*
 * Helper function to be called after any library function that requires
 * initialization.
 */
void _eos_after() {}

static U64 _eos_ethemis_detect_anomaly_mreq(const EosInitParams* params) {
    // E-THEMIS detection algorithm does not currently require any additional
    // memory to be allocated

    (void) params; // Avoid "unused argument" error
    return 0;
}

EosStatus eos_ethemis_detect_anomaly(const EosEthemisParams* params,
                                     const EosEthemisObservation* observation,
                                     EosEthemisDetectionResult* result) {
    EosStatus status;
    EosEthemisBand band;
    status = _eos_before();
    if (status != EOS_SUCCESS) { return status; }

    // Assert that parameters are not NULL
    if (eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(observation != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(result != NULL)) { return EOS_ASSERT_ERROR; }

    status = ethemis_params_check(params);
    if (status != EOS_SUCCESS) { return status; }

    for (band = EOS_ETHEMIS_BAND_1; band < EOS_ETHEMIS_N_BANDS; band++) {
        status = eos_ethemis_detect_anomaly_band(
            observation->band_shape[band], observation->band_data[band],
            params->band_threshold[band], &(result->n_results[band]),
            result->band_results[band]
        );
        if (status != EOS_SUCCESS) {
            return status;
        }
    }

    _eos_after();
    return status;
}

static U64 _eos_mise_detect_anomaly_mreq(const EosInitParams* params) {
    U64 base_size = 0;
    U64 call_size = 0;
    if (eos_assert(params != NULL)) { return 0; }

    // No local variables allocated

    // Call to `eos_mise_detect_anomaly_rx`
    call_size = eos_lmax(call_size, eos_mise_detect_anomaly_rx_mreq(params));

    return base_size + call_size;
}

static U64 _eos_pims_detect_anomaly_mreq(const EosInitParams* params) {
    U64 base_size = 0;
    U64 call_size = 0;
    if (eos_assert(params != NULL)) { return 0; }

    call_size = eos_umax(call_size, eos_pims_alg_init_mreq(params -> pims_params.alg, &(params -> pims_params.params)));
    call_size = eos_umax(call_size, eos_pims_alg_on_recv_mreq(params -> pims_params.alg, &(params -> pims_params.params)));
    return base_size + call_size;
}

EosStatus eos_mise_detect_anomaly(const EosMiseParams* params,
                                  const EosMiseObservation* observation,
                                  EosMiseDetectionResult* result) {
    EosStatus status;
    status = _eos_before();
    if (status != EOS_SUCCESS) { return status; }

    // Assert that parameters are not NULL
    if (eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(observation != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(result != NULL)) { return EOS_ASSERT_ERROR; }

    status = mise_params_check(params);
    if (status != EOS_SUCCESS) { return status; }

    if (params->alg == EOS_MISE_RX) {
        status = eos_mise_detect_anomaly_rx(
                    observation->shape,   observation->data,
                    &(result->n_results), result->results);
        if (status != EOS_SUCCESS) {
            return status;
        }
    } else {
        /* algorithm not yet implemented */
        eos_logf(EOS_LOG_ERROR, 
                 "MISE algorithm %d not yet implemented", params->alg);
        return EOS_PARAM_ERROR;
    }

    _eos_after();
    return status;
}

EosStatus eos_load_etm(const void* data, const U64 size,
                       EosEthemisObservation* obs) {
    EosStatus status;
    status = _eos_before();
    if (status != EOS_SUCCESS) { return status; }

    status = load_etm(data, size, obs);
    if (status != EOS_SUCCESS) { return status; }

    _eos_after();
    return status;
}

EosStatus eos_load_mise(const void* data, const U64 size,
                        EosMiseObservation* obs) {
    EosStatus status;
    status = _eos_before();
    if (status != EOS_SUCCESS) { return status; }

    status = load_mise(data, size, obs);
    if (status != EOS_SUCCESS) { return status; }

    _eos_after();
    return status;
}

EosStatus eos_load_pims(const void* data, const U64 size,
                       EosPimsObservationsFile* obs_file) {
    EosStatus status;
    status = _eos_before();
    if (status != EOS_SUCCESS) { return status; }

    status = load_pims(data, size, obs_file);
    if (status != EOS_SUCCESS) { return status; }

    _eos_after();
    return status;
}

EosStatus eos_pims_observation_attributes(const void* data, const uint64_t size, uint32_t* num_modes, uint32_t* max_bins, uint32_t* num_obs){
    EosStatus status;
    status = _eos_before();
    if (status != EOS_SUCCESS) { return status; }

    status = read_pims_observation_attributes(data, size, num_modes, max_bins, num_obs);
    if (status != EOS_SUCCESS) { return status; }

    _eos_after();
    return status;
}
