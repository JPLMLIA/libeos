#include <stdlib.h>
#include <stdio.h>

#include <eos.h>
#include <eos_ethemis.h>
#include "util.h"
#include "CuTest.h"

void CuAssertDetEquals(CuTest* ct, EosPixelDetection exp, EosPixelDetection act) {
    CuAssertIntEquals(ct, exp.row, act.row);
    CuAssertIntEquals(ct, exp.col, act.col);
    CuAssertDblEquals(ct, exp.score, act.score, 1e-9);
}

/*
 * Free memory associated with a detection
 */
void FreeDet(EosEthemisDetectionResult* res) {
    EosEthemisBand b;
    for (b = EOS_ETHEMIS_BAND_1; b < EOS_ETHEMIS_N_BANDS; b++) {
        if (res->band_results[b]) {
            free(res->band_results[b]);
        }
    }
}

/*
 * Clean up (deallocate) test observation and result
 */
void CleanUpTest(EosEthemisObservation* obs,
         EosEthemisDetectionResult* res) {
    FreeEthemisObs(obs);
    FreeDet(res);
}

/*
 * Basic test to exercise the interface
 */
void TestEthemisInterface(CuTest *ct) {
    const int NROWS = 10;
    const int NCOLS = 5;
    const uint16_t THRESH = 8;
    const int N_EXPECT = 3;

    EosStatus status;
    EosInitParams init_params;
    default_init_params_test(&init_params);

    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    EosEthemisObservation obs;
    InitEthemisObs(&obs, NROWS, NCOLS);

    // Populate with expected detections
    EosPixelDetection expected[N_EXPECT];
    expected[0].row = 7;
    expected[0].col = 2;
    expected[0].score = THRESH + 1;
    expected[1].row = 7;
    expected[1].col = 4;
    expected[1].score = THRESH + 2;
    expected[2].row = 8;
    expected[2].col = 1;
    expected[2].score = THRESH + 3;
    EosPixelDetection e;
    int i;
    uint16_t* data = obs.band_data[EOS_ETHEMIS_BAND_1];
    for (i = 0; i < N_EXPECT; i++) {
        e = expected[i];
        data[e.row*NCOLS + e.col] = (uint16_t)e.score;
    }

    EosEthemisDetectionResult result;
    result.n_results[EOS_ETHEMIS_BAND_1] = 5;
    result.n_results[EOS_ETHEMIS_BAND_2] = 0;
    result.n_results[EOS_ETHEMIS_BAND_3] = 0;
    result.band_results[EOS_ETHEMIS_BAND_1] = calloc(
        sizeof(EosPixelDetection), result.n_results[EOS_ETHEMIS_BAND_1]);
    result.band_results[EOS_ETHEMIS_BAND_2] = NULL;
    result.band_results[EOS_ETHEMIS_BAND_3] = NULL;

    EosParams params;
    status = eos_init_default_params(&params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    params.ethemis.band_threshold[EOS_ETHEMIS_BAND_1] = THRESH;

    status = eos_ethemis_detect_anomaly(&(params.ethemis), &obs, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, N_EXPECT, result.n_results[EOS_ETHEMIS_BAND_1]);
    EosPixelDetection* det = result.band_results[EOS_ETHEMIS_BAND_1];
    CuAssertDetEquals(ct, expected[2], det[0]);
    CuAssertDetEquals(ct, expected[1], det[1]);
    CuAssertDetEquals(ct, expected[0], det[2]);

    // Clean up
    CleanUpTest(&obs, &result);

    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/*
 * Test behavior if the E-THEMIS observation is empty
 */
void TestEmptyEthemisObservation(CuTest *ct) {
    const uint16_t THRESH = 8;
    const int N_EXPECT = 0;

    EosStatus status;
    EosInitParams init_params;
    default_init_params_test(&init_params);

    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    EosEthemisObservation obs;
    obs.observation_id = 1;
    // Empty observation
    obs.band_shape[EOS_ETHEMIS_BAND_1].rows = 0;
    obs.band_shape[EOS_ETHEMIS_BAND_1].cols = 0;
    obs.band_shape[EOS_ETHEMIS_BAND_2].rows = 0;
    obs.band_shape[EOS_ETHEMIS_BAND_2].cols = 0;
    obs.band_shape[EOS_ETHEMIS_BAND_3].rows = 0;
    obs.band_shape[EOS_ETHEMIS_BAND_3].cols = 0;
    obs.band_data[EOS_ETHEMIS_BAND_1] = NULL;
    obs.band_data[EOS_ETHEMIS_BAND_2] = NULL;
    obs.band_data[EOS_ETHEMIS_BAND_3] = NULL;

    EosEthemisDetectionResult result;
    // Request top 5 detections despite no data
    result.n_results[EOS_ETHEMIS_BAND_1] = 5;
    result.n_results[EOS_ETHEMIS_BAND_2] = 0;
    result.n_results[EOS_ETHEMIS_BAND_3] = 0;
    result.band_results[EOS_ETHEMIS_BAND_1] = calloc(
        sizeof(EosPixelDetection), result.n_results[EOS_ETHEMIS_BAND_1]);
    result.band_results[EOS_ETHEMIS_BAND_2] = NULL;
    result.band_results[EOS_ETHEMIS_BAND_3] = NULL;

    EosParams params;
    status = eos_init_default_params(&params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    params.ethemis.band_threshold[EOS_ETHEMIS_BAND_1] = THRESH;

    // Perform detection
    status = eos_ethemis_detect_anomaly(&(params.ethemis), &obs, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, N_EXPECT, result.n_results[EOS_ETHEMIS_BAND_1]);

    // Clean up
    CleanUpTest(&obs, &result);

    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/*
 * Test behavior if data is non-NULL but band_results
 * (array to store detections) is NULL (e.g., memory allocation failed)
 */
void TestNullBandResults(CuTest *ct) {
    const int NROWS = 10;
    const int NCOLS = 5;
    const uint16_t THRESH = 8;

    EosStatus status;
    EosInitParams init_params;
    default_init_params_test(&init_params);

    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    EosEthemisObservation obs;
    InitEthemisObs(&obs, NROWS, NCOLS);

    EosEthemisDetectionResult result;
    result.n_results[EOS_ETHEMIS_BAND_1] = 5;
    result.n_results[EOS_ETHEMIS_BAND_2] = 0;
    result.n_results[EOS_ETHEMIS_BAND_3] = 0;
    // No memory allocated for any band results (or alloc failed)
    result.band_results[EOS_ETHEMIS_BAND_1] = NULL;
    result.band_results[EOS_ETHEMIS_BAND_2] = NULL;
    result.band_results[EOS_ETHEMIS_BAND_3] = NULL;

    EosParams params;
    status = eos_init_default_params(&params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    params.ethemis.band_threshold[EOS_ETHEMIS_BAND_1] = THRESH;

    // Perform detection
    status = eos_ethemis_detect_anomaly(&(params.ethemis), &obs, &result);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);

    // Clean up
    CleanUpTest(&obs, &result);

    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/*
 * Test behavior if data is all zeros and THRESH is 1.
 * Should get no detections.
 */
void TestZeroData(CuTest *ct) {
    const int NROWS = 10;
    const int NCOLS = 5;
    const uint16_t THRESH = 1;
    const int N_EXPECT = 0;

    EosStatus status;
    EosInitParams init_params;
    default_init_params_test(&init_params);

    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    EosEthemisObservation obs;
    InitEthemisObs(&obs, NROWS, NCOLS);

    EosEthemisDetectionResult result;
    // Request top 5 detections (but none should pass the threshold)
    result.n_results[EOS_ETHEMIS_BAND_1] = 5;
    result.n_results[EOS_ETHEMIS_BAND_2] = 0;
    result.n_results[EOS_ETHEMIS_BAND_3] = 0;
    result.band_results[EOS_ETHEMIS_BAND_1] = calloc(
        sizeof(EosPixelDetection), result.n_results[EOS_ETHEMIS_BAND_1]);
    result.band_results[EOS_ETHEMIS_BAND_2] = NULL;
    result.band_results[EOS_ETHEMIS_BAND_3] = NULL;

    EosParams params;
    status = eos_init_default_params(&params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    params.ethemis.band_threshold[EOS_ETHEMIS_BAND_1] = THRESH;

    // Perform detection
    status = eos_ethemis_detect_anomaly(&(params.ethemis), &obs, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, N_EXPECT, result.n_results[EOS_ETHEMIS_BAND_1]);

    // Clean up
    CleanUpTest(&obs, &result);

    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/*
 * Test behavior if data is all zeros and THRESH is 0.
 * Should get N_EXPECT detections.
 */
void TestZeroDataZeroThresh(CuTest *ct) {
    const int NROWS = 10;
    const int NCOLS = 5;
    const uint16_t THRESH = 0;
    const int N_EXPECT = 5;

    EosStatus status;
    EosInitParams init_params;
    default_init_params_test(&init_params);

    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    EosEthemisObservation obs;
    InitEthemisObs(&obs, NROWS, NCOLS);

    EosEthemisDetectionResult result;
    // Request top 5 detections (should get all 5)
    result.n_results[EOS_ETHEMIS_BAND_1] = N_EXPECT;
    result.n_results[EOS_ETHEMIS_BAND_2] = 0;
    result.n_results[EOS_ETHEMIS_BAND_3] = 0;
    result.band_results[EOS_ETHEMIS_BAND_1] = calloc(
        sizeof(EosPixelDetection), result.n_results[EOS_ETHEMIS_BAND_1]);
    result.band_results[EOS_ETHEMIS_BAND_2] = NULL;
    result.band_results[EOS_ETHEMIS_BAND_3] = NULL;

    EosParams params;
    status = eos_init_default_params(&params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    params.ethemis.band_threshold[EOS_ETHEMIS_BAND_1] = THRESH;

    // Perform detection
    status = eos_ethemis_detect_anomaly(&(params.ethemis), &obs, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, N_EXPECT, result.n_results[EOS_ETHEMIS_BAND_1]);

    // Clean up
    CleanUpTest(&obs, &result);

    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/*
 * Test behavior if data is all at max uint16 (65536).
 * Should get only N_EXPECT detections.
 */
void TestNExpectLimit(CuTest *ct) {
    const int NROWS = 10;
    const int NCOLS = 5;
    const uint16_t THRESH = 0;
    const int N_EXPECT = 5;

    EosStatus status;
    EosInitParams init_params;
    default_init_params_test(&init_params);

    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    EosEthemisObservation obs;
    InitEthemisObsValue(&obs, NROWS, NCOLS, 65535);

    EosEthemisDetectionResult result;
    // Request top 5 detections (should get 5)
    result.n_results[EOS_ETHEMIS_BAND_1] = N_EXPECT;
    result.n_results[EOS_ETHEMIS_BAND_2] = 0;
    result.n_results[EOS_ETHEMIS_BAND_3] = 0;
    result.band_results[EOS_ETHEMIS_BAND_1] = calloc(
        sizeof(EosPixelDetection), result.n_results[EOS_ETHEMIS_BAND_1]);
    result.band_results[EOS_ETHEMIS_BAND_2] = NULL;
    result.band_results[EOS_ETHEMIS_BAND_3] = NULL;

    EosParams params;
    status = eos_init_default_params(&params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    params.ethemis.band_threshold[EOS_ETHEMIS_BAND_1] = THRESH;

    // Perform detection
    status = eos_ethemis_detect_anomaly(&(params.ethemis), &obs, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, N_EXPECT, result.n_results[EOS_ETHEMIS_BAND_1]);

    // Clean up
    CleanUpTest(&obs, &result);

    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/*
 * Test behavior if zero detections are requested (but >= hot pixels).
 * Should get 0 detections.
 */
void TestZeroRequested(CuTest *ct) {
    const int NROWS = 10;
    const int NCOLS = 5;
    const uint16_t THRESH = 0;
    const int N_EXPECT = 0;

    EosStatus status;
    EosInitParams init_params;
    default_init_params_test(&init_params);

    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    EosEthemisObservation obs;
    InitEthemisObsValue(&obs, NROWS, NCOLS, 10);

    EosEthemisDetectionResult result;
    // Request zero detections (should get 0)
    result.n_results[EOS_ETHEMIS_BAND_1] = 0;
    result.n_results[EOS_ETHEMIS_BAND_2] = 0;
    result.n_results[EOS_ETHEMIS_BAND_3] = 0;
    // Allocate memory as if we wanted more results
    result.band_results[EOS_ETHEMIS_BAND_1] = calloc(
                     sizeof(EosPixelDetection), 5);
    result.band_results[EOS_ETHEMIS_BAND_2] = NULL;
    result.band_results[EOS_ETHEMIS_BAND_3] = NULL;

    EosParams params;
    status = eos_init_default_params(&params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    params.ethemis.band_threshold[EOS_ETHEMIS_BAND_1] = THRESH;

    // Perform detection
    status = eos_ethemis_detect_anomaly(&(params.ethemis), &obs, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, N_EXPECT, result.n_results[EOS_ETHEMIS_BAND_1]);

    // Clean up
    CleanUpTest(&obs, &result);

    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/*
 * Test behavior if we request more detections than are possible.
 * Should only get the limit (but no errors).
 */
void TestTooManyRequested(CuTest *ct) {
    const int NROWS = 10;
    const int NCOLS = 5;
    const uint16_t THRESH = 0;
    const int N_EXPECT = NROWS*NCOLS;

    EosStatus status;
    EosInitParams init_params;
    default_init_params_test(&init_params);

    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    EosEthemisObservation obs;
    InitEthemisObsValue(&obs, NROWS, NCOLS, 10);

    EosEthemisDetectionResult result;
    // Request too many detections
    result.n_results[EOS_ETHEMIS_BAND_1] = NROWS*NCOLS + 1;
    result.n_results[EOS_ETHEMIS_BAND_2] = 0;
    result.n_results[EOS_ETHEMIS_BAND_3] = 0;
    result.band_results[EOS_ETHEMIS_BAND_1] = calloc(
        sizeof(EosPixelDetection), result.n_results[EOS_ETHEMIS_BAND_1]);
    result.band_results[EOS_ETHEMIS_BAND_2] = NULL;
    result.band_results[EOS_ETHEMIS_BAND_3] = NULL;

    EosParams params;
    status = eos_init_default_params(&params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    params.ethemis.band_threshold[EOS_ETHEMIS_BAND_1] = THRESH;

    // Perform detection
    status = eos_ethemis_detect_anomaly(&(params.ethemis), &obs, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, N_EXPECT, result.n_results[EOS_ETHEMIS_BAND_1]);

    // Clean up
    CleanUpTest(&obs, &result);

    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

CuSuite* CuEthemisGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TestEthemisInterface);
    SUITE_ADD_TEST(suite, TestEmptyEthemisObservation);
    SUITE_ADD_TEST(suite, TestNullBandResults);
    SUITE_ADD_TEST(suite, TestZeroData);
    SUITE_ADD_TEST(suite, TestZeroDataZeroThresh);
    SUITE_ADD_TEST(suite, TestNExpectLimit);
    SUITE_ADD_TEST(suite, TestZeroRequested);
    SUITE_ADD_TEST(suite, TestTooManyRequested);

    return suite;
}
