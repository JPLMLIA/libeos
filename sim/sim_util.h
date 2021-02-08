#ifndef JPL_EOS_SIM_UTIL
#define JPL_EOS_SIM_UTIL

#include <eos.h>

/*
 * E-THEMIS Utility Functions
 */

EosStatus init_ethemis_obs(EosEthemisObservation *obs, const int size);
void free_ethemis_obs(EosEthemisObservation *obs);
EosStatus init_ethemis_result(EosEthemisDetectionResult *result);
void free_ethemis_result(EosEthemisDetectionResult *result);
EosStatus teardown_ethemis_sim(void **data, EosEthemisObservation *obs,
    EosEthemisDetectionResult *result);

/*
 * MISE Utility Functions
 */

EosStatus init_mise_obs(EosMiseObservation *obs, const int size);
void free_mise_obs(EosMiseObservation *obs);
EosStatus init_mise_result(EosMiseDetectionResult *result);
void free_mise_result(EosMiseDetectionResult *result);
EosStatus teardown_mise_sim(void **data, EosMiseObservation *obs,
    EosMiseDetectionResult *result);

#endif
