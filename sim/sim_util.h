#ifndef JPL_EOS_SIM_UTIL
#define JPL_EOS_SIM_UTIL

#include <eos.h>

/*
 * Generic Utility Functions
 */
void default_init_params(EosInitParams *init_params);

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
EosStatus teardown_mise_sim(void **data, void **stack,
    EosMiseObservation *obs, EosMiseDetectionResult *result);


/*
 * PIMS Utility Functions
 */

EosStatus init_pims_obs_file(EosPimsObservationsFile* obs_file, U32 num_modes, U32 max_bins, U32 num_obs);
EosStatus sim_pims_handle_state_teardown(EosPimsAlgorithm algorithm, EosPimsAlgorithmState* state);
EosStatus sim_pims_handle_state_request(EosPimsAlgorithm algorithm, EosPimsAlgorithmStateRequest* req, EosPimsAlgorithmState* state);
void free_pims_obs(EosPimsObservation *obs);
void free_pims_obs_file(EosPimsObservationsFile *obs);
EosStatus teardown_pims_sim(void **data, EosPimsAlgorithm algorithm, EosPimsObservationsFile *obs_file, EosPimsAlgorithmState* state);

#endif
