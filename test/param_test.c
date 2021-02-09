#include <stdlib.h>

#include <eos_params.h>
#include "CuTest.h"

void TestParamMacro(CuTest *ct) {
    EosStatus status;

    status = param_check(1.0 > 0.0);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_check(0.0 > 1.0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);
}

void TestParamGtzMacro(CuTest *ct) {
    EosStatus status;

    status = param_gt_zero(1.0);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gt_zero(1);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gt_zero(0.0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    status = param_gt_zero(0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    status = param_gt_zero(-1.0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    status = param_gt_zero(-1);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);
}

void TestParamGtezMacro(CuTest *ct) {
    EosStatus status;

    status = param_gte_zero(1.0);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gte_zero(1);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gte_zero(0.0);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gte_zero(0);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gte_zero(-1.0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    status = param_gte_zero(-1);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);
}

void TestParamLtoMacro(CuTest *ct) {
    EosStatus status;

    status = param_lt_one(0.0);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_lt_one(0);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_lt_one(1.0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    status = param_lt_one(1);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    status = param_lt_one(2.0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    status = param_lt_one(2);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);
}

void TestParamGtoMacro(CuTest *ct) {
    EosStatus status;

    status = param_gt_one(2.0);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gt_one(2);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gt_one(1.0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    status = param_gt_one(1);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    status = param_gt_one(0.0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    status = param_gt_one(0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);
}

void TestParamGteoMacro(CuTest *ct) {
    EosStatus status;

    status = param_gte_one(2.0);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gte_one(2);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gte_one(1.0);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gte_one(1);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_gte_one(0.0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    status = param_gte_one(0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);
}

void TestParamInRangeMacro(CuTest *ct) {
    EosStatus status;

    status = param_in_range(3, 0, 4);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = param_in_range(3, 3, 3);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    /* too high */
    status = param_in_range(3, 0, 2);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    /* too low */
    status = param_in_range(-1, 0, 2);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    /* range not in order */
    status = param_in_range(3, 4, 2);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);
}

void TestMiseParamCheck(CuTest *ct) {
    EosStatus status;
    EosMiseParams params;

    params.alg = EOS_MISE_RX;
    status = mise_params_check(&params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    params.alg = EOS_MISE_N_ALGS;
    status = mise_params_check(&params);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);
}

void TestPimsParamCheck(CuTest *ct) {
    EosStatus status;
    EosPimsParams pims_params;

    pims_params.params.common_params.filter = EOS_PIMS_NO_FILTER;
    pims_params.params.common_params.max_observations = 10;
    pims_params.params.common_params.max_bins = 100;
    pims_params.alg = EOS_PIMS_BASELINE;
    status = pims_params_check(&pims_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    pims_params.params.common_params.max_observations = 0;
    pims_params.alg = EOS_PIMS_NO_ALGORITHM;
    status = pims_params_check(&pims_params);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    pims_params.params.common_params.max_observations = 1;
    status = pims_params_check(&pims_params);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    pims_params.alg = EOS_PIMS_BASELINE;
    status = pims_params_check(&pims_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    pims_params.params.common_params.filter = EOS_PIMS_MIN_FILTER;
    status = pims_params_check(&pims_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    pims_params.params.common_params.filter = EOS_PIMS_MEAN_FILTER;
    status = pims_params_check(&pims_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    pims_params.params.common_params.filter = EOS_PIMS_MAX_FILTER;
    status = pims_params_check(&pims_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    pims_params.params.common_params.max_bins = 0;
    status = pims_params_check(&pims_params);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);
}

void TestCombinedParamCheck(CuTest *ct) {
    EosStatus status;
    EosParams params;

    /* Invalid values for PIMS and MISE: should be rejected independently. */
    params.pims.params.common_params.max_observations = 0;
    params.mise.alg = EOS_MISE_N_ALGS;

    status = params_check(&params);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);
}

CuSuite* CuParamGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestParamMacro);
    SUITE_ADD_TEST(suite, TestParamGtzMacro);
    SUITE_ADD_TEST(suite, TestParamGtezMacro);
    SUITE_ADD_TEST(suite, TestParamLtoMacro);
    SUITE_ADD_TEST(suite, TestParamGtoMacro);
    SUITE_ADD_TEST(suite, TestParamGteoMacro);
    SUITE_ADD_TEST(suite, TestParamInRangeMacro);
    SUITE_ADD_TEST(suite, TestMiseParamCheck);
    SUITE_ADD_TEST(suite, TestPimsParamCheck);
    SUITE_ADD_TEST(suite, TestCombinedParamCheck);

    return suite;
}
