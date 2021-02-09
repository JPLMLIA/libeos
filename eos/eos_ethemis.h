#ifndef JPL_EOS_ETHEMIS
#define JPL_EOS_ETHEMIS

#include "eos_types.h"

EosStatus eos_ethemis_detect_anomaly_band(const EosObsShape shape,
    const U16* data, const U16 threshold,
    U32* n_results, EosPixelDetection* results);

#endif
