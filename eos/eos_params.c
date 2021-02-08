#include "eos_params.h"
#include "eos_log.h"

EosStatus _param_check(U32 check, CHAR* check_str) {
    if (!check) {
        eos_logf(EOS_LOG_ERROR, "Parameter check \"%s\" failed", check_str);
        return EOS_PARAM_ERROR;
    }
    return EOS_SUCCESS;
}

EosStatus ethemis_params_check(const EosEthemisParams* params) {
    EosStatus status = EOS_SUCCESS;
    if (eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }

    // No checks currently required

    if (status != EOS_SUCCESS) {
        status = EOS_PARAM_ERROR;
    }
    return status;
}

EosStatus mise_params_check(const EosMiseParams* params) {
    EosStatus status = EOS_SUCCESS;
    if (eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }

    /* Check for valid algorithm (in range) */
    status |= param_in_range(params->alg, 0, (EOS_MISE_N_ALGS - 1));

    if (status != EOS_SUCCESS) {
        status = EOS_PARAM_ERROR;
    }
    return status;
}

EosStatus params_check(const EosParams* params) {
    EosStatus status = EOS_SUCCESS;
    if (eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }

    status |= ethemis_params_check(&params->ethemis);
    status |= mise_params_check(&params->mise);

    if (status != EOS_SUCCESS) {
        status = EOS_PARAM_ERROR;
    }
    return status;
}

EosStatus params_init_default(EosParams* params) {
    EosStatus status = EOS_SUCCESS;
    if (eos_assert(params != NULL)) { return EOS_ASSERT_ERROR; }

    // Initialize E-THEMIS parameters
    params->ethemis.band_threshold[EOS_ETHEMIS_BAND_1] =
        EOS_DEFAULT_ETHEMIS_BAND_1_THRESHOLD;
    params->ethemis.band_threshold[EOS_ETHEMIS_BAND_2] =
        EOS_DEFAULT_ETHEMIS_BAND_2_THRESHOLD;
    params->ethemis.band_threshold[EOS_ETHEMIS_BAND_3] =
        EOS_DEFAULT_ETHEMIS_BAND_3_THRESHOLD;

    // Initialize MISE parameters
    params->mise.alg = EOS_DEFAULT_MISE_ALG;

    status = params_check(params);
    if (status != EOS_SUCCESS) { return status; }

    return status;
}
