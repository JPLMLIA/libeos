#ifndef JPL_EOS_MISE
#define JPL_EOS_MISE

#include "eos_types.h"

EosStatus eos_mise_detect_anomaly_rx(const EosObsShape shape,
    const U16* data, U32* n_results, EosPixelDetection* results);

U64 eos_mise_detect_anomaly_rx_mreq(const EosInitParams* params);

EosStatus compute_mean_pixel(const U16* data, const EosObsShape* shape,
    F64 mp[]);
EosStatus compute_covariance(const U16* data, const EosObsShape* shape,
    F64 mean_pixel[], F64* cov);

EosStatus get_eigen_symm(U32 n, F64* A, F64* w, F64* V, U32* buf);
EosStatus invert_sym_matrix(U32 n, F64* A, F64* A_inv);

#endif
