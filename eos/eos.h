#ifndef JPL_EOS
#define JPL_EOS

#include "eos_types_pub.h"

/**
 * Calculates library-level memory requirement
 *
 * This function computes how much memory the library will need if it is
 * initialized with the given `params`. The initialization parameters put
 * limits on the size of various inputs with which library functions will be
 * called.
 *
 * :param params: parameters used to initialize the module
 *
 * :return: required memory in bytes
 */
uint64_t eos_memory_requirement(const EosInitParams* params);

EosStatus eos_init_default_params(EosParams* params);

EosStatus eos_init(const EosInitParams* params, void* initial_memory_ptr,
                   uint64_t initial_memory_size,
                   void (*log_function)(EosLogType, const char*));
EosStatus eos_teardown();

EosStatus eos_ethemis_detect_anomaly(const EosEthemisParams* params,
                                     const EosEthemisObservation* observation,
                                     EosEthemisDetectionResult* result);

EosStatus eos_mise_detect_anomaly(const EosMiseParams* params,
                                  const EosMiseObservation* observation,
                                  EosMiseDetectionResult* result);

EosStatus eos_load_etm(const void* data, const uint64_t size,
                       EosEthemisObservation* obs);

EosStatus eos_load_mise(const void* data, const uint64_t size,
                        EosMiseObservation* obs);

#endif
