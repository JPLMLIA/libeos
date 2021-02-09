#ifndef JPL_EOS_PIMS_ALGORITHMS
#define JPL_EOS_PIMS_ALGORITHMS

#include "eos_types.h"

EosStatus eos_pims_alg_state_request(EosPimsAlgorithm algorithm, EosPimsAlgorithmParams* params, EosPimsAlgorithmStateRequest* req);
U64 eos_pims_alg_init_mreq(EosPimsAlgorithm algorithm, const EosPimsAlgorithmParams* params);
EosStatus eos_pims_alg_init(EosPimsAlgorithm algorithm, EosPimsAlgorithmParams* params, EosPimsAlgorithmState* state);
U64 eos_pims_alg_on_recv_mreq(EosPimsAlgorithm algorithm, const EosPimsAlgorithmParams* params);
EosStatus eos_pims_alg_on_recv(EosPimsObservation obs, EosPimsAlgorithmParams* params, EosPimsAlgorithmState* state, EosPimsDetection* result);
EosStatus eos_pims_teardown();
EosStatus eos_pims_verify_initialization();

#endif
