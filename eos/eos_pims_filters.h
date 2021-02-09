#ifndef JPL_EOS_PIMS_FILTERS
#define JPL_EOS_PIMS_FILTERS

#include "eos_types.h"

U64 eos_pims_filter_mreq(const EosPimsCommonParams* c_params);
EosStatus eos_pims_filter(EosPimsFilter filter, EosPimsObservation obs, EosPimsObservationQueue q);

#endif
