#ifndef JPL_EOS_PIMS_BASELINE
#define JPL_EOS_PIMS_BASELINE

#include "eos_types.h"

EosStatus eos_pims_baseline_state_request(EosPimsCommonParams* c_params, EosPimsBaselineParams* params, EosPimsBaselineStateRequest* req);
U64 eos_pims_baseline_init_mreq(const EosPimsCommonParams* c_params, const EosPimsBaselineParams* params);
EosStatus eos_pims_baseline_init(EosPimsCommonParams* c_params, EosPimsBaselineParams* params, EosPimsBaselineState* state);
U64 eos_pims_baseline_on_recv_mreq(const EosPimsCommonParams* c_params, const EosPimsBaselineParams* params);
EosStatus eos_pims_baseline_on_recv(EosPimsObservation obs, EosPimsCommonParams* c_params, EosPimsBaselineParams* params, EosPimsBaselineState* state, EosPimsDetection* result);

#endif
