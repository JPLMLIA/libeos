#ifndef JPL_EOS_PARAMS
#define JPL_EOS_PARAMS

#include <stdlib.h>

#include "eos_types.h"
#include "float.h"

// Default E-THEMIS Params
#define EOS_DEFAULT_ETHEMIS_BAND_1_THRESHOLD 0
#define EOS_DEFAULT_ETHEMIS_BAND_2_THRESHOLD 0
#define EOS_DEFAULT_ETHEMIS_BAND_3_THRESHOLD 0

// Default MISE Params
#define EOS_DEFAULT_MISE_ALG EOS_MISE_RX

EosStatus params_init_default(EosParams* params);
EosStatus ethemis_params_check(const EosEthemisParams* params);
EosStatus mise_params_check(const EosMiseParams* params);
EosStatus params_check(const EosParams* params);
EosStatus _param_check(U32 check, CHAR* check_str);

/* #c is converted to the literal text in c */
#define param_check(c) (_param_check(c, #c))
#define param_gt_zero(p) (param_check(p > 0))
#define param_gte_zero(p) (param_check(p >= 0))
#define param_lt_one(p) (param_check(p < 1))
#define param_gt_one(p) (param_check(p > 1))
#define param_gte_one(p) (param_check(p >= 1))
#define param_in_range(p, l, u) (param_check(l <= p && p <= u))

#endif
