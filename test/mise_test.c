#include <stdlib.h>
#include <string.h>
#include <float.h>

#include <eos.h>
#include <eos_mise.h>
#include "CuTest.h"
#include "util.h"

EosStatus _eigen_pivot(F64 p, F64 y, F64* c, F64* s, F64* t);
EosStatus _rx_score(F64* mean_sub, F64* cov_inv, const EosObsShape shape,
    F64* temp, F64* score);

void TestComputeMeanPixel(CuTest *ct) {

    // Nominal test case
    EosStatus status;
    const U16 data[6] = {1, 2, 3, 4, 5, 6};
    EosObsShape shape;
    shape.rows = 1;
    shape.cols = 2;
    shape.bands = 3;

    F64 mp[3];

    status = compute_mean_pixel(data, &shape, mp);

    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 2.5, mp[0], 1e-9);
    CuAssertDblEquals(ct, 3.5, mp[1], 1e-9);
    CuAssertDblEquals(ct, 4.5, mp[2], 1e-9);

    // Zero-size observation
    shape.rows = 0;
    status = compute_mean_pixel(data, &shape, mp);

    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0, mp[0], 1e-9);
    CuAssertDblEquals(ct, 0, mp[1], 1e-9);
    CuAssertDblEquals(ct, 0, mp[2], 1e-9);

    // Null Pointers
    shape.rows = 1;
    status = compute_mean_pixel(NULL, &shape, mp);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);
    status = compute_mean_pixel(data, NULL, mp);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);
    status = compute_mean_pixel(data, &shape, NULL);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);
}

void TestComputeCovariance(CuTest *ct) {
    // Nominal test case
    EosStatus status;
    const U16 data[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    EosObsShape shape;
    shape.rows = 1;
    shape.cols = 3;
    shape.bands = 3;
    U32 i, cov_size = shape.bands * shape.bands;

    F64 mean_pixel[shape.bands];
    status = compute_mean_pixel(data, &shape, mean_pixel);

    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 4, mean_pixel[0], 1e-9);
    CuAssertDblEquals(ct, 5, mean_pixel[1], 1e-9);
    CuAssertDblEquals(ct, 6, mean_pixel[2], 1e-9);

    F64 cov[shape.bands * shape.bands];
    status = compute_covariance(data, &shape, mean_pixel, cov);

    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    for (i = 0; i < cov_size; i++) {
        CuAssertDblEquals(ct, 9.0, cov[i], 1e-9);
    }

    // Test Small Sample Sizes
    shape.cols = 1;
    status = compute_covariance(data, &shape, mean_pixel, cov);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    shape.cols = 0;
    status = compute_covariance(data, &shape, mean_pixel, cov);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    // Test NULL pointers
    shape.cols = 3;
    status = compute_covariance(NULL, &shape, mean_pixel, cov);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);
    status = compute_covariance(data, NULL, mean_pixel, cov);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);
    status = compute_covariance(data, &shape, NULL, cov);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);
    status = compute_covariance(data, &shape, mean_pixel, NULL);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);
}

void TestPZeroInEigenPivot(CuTest *ct) {
    EosStatus status;
    F64 p = 0;
    F64 y = 1;
    F64 c;
    F64 s;
    F64 t;

    status = _eigen_pivot(p, y, &c, &s, &t);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);
}

void TestEigen(CuTest *ct) {
    double a[4] = {-5, 1, 1, 3};
    double v[9];
    double w[3];
    double ve[4] = {0.99250756, -0.12218326, 0.12218326, 0.99250756};
    double we[2] = {-5.12310563, 3.12310563};
    U32 buf[6];
    get_eigen_symm(2, a, w, v, buf);
    int i;
    for (i = 0; i < 4; i++) {
        CuAssertDblEquals(ct,  ve[i], v[i], 1e-6);
    }
    for (i = 0; i < 2; i++) {
        CuAssertDblEquals(ct,  we[i], w[i], 1e-6);
    }

    // Test degenerate case (zero matrix)
    double b[4] = {0, 0, 0, 0};
    double vbe[4] = {1.0, 0.0, 0.0, 1.0};
    double wbe[2] = {0.0, 0.0};
    get_eigen_symm(2, b, w, v, buf);
    for (i = 0; i < 4; i++) {
        CuAssertDblEquals(ct,  vbe[i], v[i], DBL_EPSILON);
    }
    for (i = 0; i < 2; i++) {
        CuAssertDblEquals(ct,  wbe[i], w[i], DBL_EPSILON);
    }

    double c[9] = {1, -3, 2, -3, 5, 6, 2, 6, 4};
    double vce[9] = {
         0.81049889, -0.31970025,  0.49079864,
        -0.0987837 ,  0.75130448,  0.65252078,
        -0.57735027, -0.57735027,  0.57735027
    };
    double wce[3] = {3.39444872, 10.60555128, -4.};
    get_eigen_symm(3, c, w, v, buf);
    for (i = 0; i < 9; i++) {
        CuAssertDblEquals(ct,  vce[i], v[i], 1e-6);
    }
    for (i = 0; i < 3; i++) {
        CuAssertDblEquals(ct,  wce[i], w[i], 1e-6);
    }

    // Test small matrix
    double d[1] = {2.0};
    get_eigen_symm(1, d, w, v, buf);
    CuAssertDblEquals(ct,  1.0, v[0], 1e-6);
    CuAssertDblEquals(ct,  2.0, w[0], 1e-6);
}

void TestInvert(CuTest *ct) {
    EosStatus status;
    double v[64];
    double x[64];
    memset(v, 0, 64*sizeof(double));
    memset(x, 0, 64*sizeof(double));
    v[0*8 + 0] = -5;
    v[0*8 + 1] = 1;
    v[1*8 + 0] = 1;
    v[1*8 + 1] = 3;
    v[2*8 + 2] = 1;
    v[3*8 + 3] = 1;
    v[4*8 + 4] = 1;
    v[5*8 + 5] = 1;
    v[6*8 + 6] = 1;
    v[7*8 + 7] = 1;
    status = invert_sym_matrix(8, v, x);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, -0.1875, x[0*8 + 0], DBL_EPSILON);
    CuAssertDblEquals(ct,  0.0625, x[0*8 + 1], DBL_EPSILON);
    CuAssertDblEquals(ct,  0.0625, x[1*8 + 0], DBL_EPSILON);
    CuAssertDblEquals(ct,  0.3125, x[1*8 + 1], DBL_EPSILON);
}

void TestRxScore(CuTest *ct) {
    EosStatus status;
    EosObsShape shape = {1, 1, 3};
    F64 mean_sub[3] = {1.0, 2.0, 3.0};
    F64 cov_inv[9] = {
        1.0, 0.0, 0.0,
        0.0, 2.0, 0.0,
        0.0, 0.0, 3.0
    };
    F64 temp[3];
    F64 score;

    status = _rx_score(mean_sub, cov_inv, shape, temp, &score);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 36.0, score, 1e-12);

    status = _rx_score(NULL, cov_inv, shape, temp, &score);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);

    status = _rx_score(mean_sub, NULL, shape, temp, &score);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);

    status = _rx_score(mean_sub, cov_inv, shape, NULL, &score);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);

    status = _rx_score(mean_sub, cov_inv, shape, temp, NULL);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);
}

void TestRxAnomalyDetection(CuTest *ct) {
    EosStatus status;
    EosObsShape shape1 = {1, 2, 3};
    uint16_t data1[6] = {1, 1, 1, 2, 2, 2};
    uint32_t n_results = 4;
    EosPixelDetection results[4];
    EosInitParams init_params;
    default_init_params_test(&init_params);

    // Initialize for memory allocation
    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_mise_detect_anomaly_rx(shape1, data1, &n_results, results);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 2, n_results);

    // Test n_results = 0
    n_results = 0;
    status = eos_mise_detect_anomaly_rx(shape1, data1, &n_results, results);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 0, n_results);

    // Test outlier correctly selected
    EosObsShape shape2 = {1, 3, 2};
    uint16_t data2[6] = {1, 1, 2, 2, 100, 100};
    n_results = 1;
    status = eos_mise_detect_anomaly_rx(shape2, data2, &n_results, results);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 1, n_results);
    CuAssertIntEquals(ct, 0, results[0].row);
    CuAssertIntEquals(ct, 2, results[0].col);

    // Test zero-size input
    EosObsShape shape3 = {0, 3, 2};
    uint16_t data3[1] = {0};
    n_results = 1;
    status = eos_mise_detect_anomaly_rx(shape3, data3, &n_results, results);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 0, n_results);

    // Test zero bands (pixel chosen arbitrarily but has score 0.0)
    EosObsShape shape4 = {1, 2, 0};
    uint16_t data4[1] = {0};
    n_results = 1;
    status = eos_mise_detect_anomaly_rx(shape4, data4, &n_results, results);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 1, n_results);
    CuAssertDblEquals(ct, 0, results[0].score, 1e-9);

    // Test NULL pointer behavior
    n_results = 4;
    status = eos_mise_detect_anomaly_rx(shape1, NULL, &n_results, results);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);

    status = eos_mise_detect_anomaly_rx(shape1, data1, NULL, results);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);

    status = eos_mise_detect_anomaly_rx(shape1, data1, &n_results, NULL);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);

    // Clean up
    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

void TestMiseInterface(CuTest *ct) {
    EosStatus status;

    // Set up library/algorithm params
    EosInitParams init_params;
    default_init_params_test(&init_params);
    EosMiseParams params;
    params.alg = EOS_MISE_RX;

    // Set up structure on stack for storing results
    EosPixelDetection detections[10];
    EosMiseDetectionResult result;
    result.n_results = 10;
    result.results = (EosPixelDetection*)(&detections);

    // Load data and initialize observation
    void* data;
    uint32_t size;
    EosMiseObservation obs;
    InitMiseObs(&obs, 10, 10, 5);
    read_resource(ct, "mise/test_mise.mis", &data, &size);

    // First test without initialization
    status = eos_mise_detect_anomaly(&params, &obs, &result);
    CuAssertIntEquals(ct, EOS_NOT_INITIALIZED, status);

    // Initialize library
    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    // Test with NULL arguments
    status = eos_mise_detect_anomaly(NULL, &obs, &result);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);

    status = eos_mise_detect_anomaly(&params, NULL, &result);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);

    status = eos_mise_detect_anomaly(&params, &obs, NULL);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);

    // Nominal case
    status = eos_mise_detect_anomaly(&params, &obs, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    // Bad Algorithm
    result.n_results = 10;
    params.alg = 0xBAD;
    status = eos_mise_detect_anomaly(&params, &obs, &result);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);

    // Error within algorithm
    params.alg = EOS_MISE_RX;
    result.results = NULL;
    status = eos_mise_detect_anomaly(&params, &obs, &result);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);

    // Clean up
    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    free(data);
    FreeMiseObs(&obs);
}

CuSuite* CuMiseGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();

    // Public interface
    SUITE_ADD_TEST(suite, TestMiseInterface);

    // Internal Function
    SUITE_ADD_TEST(suite, TestComputeMeanPixel);
    SUITE_ADD_TEST(suite, TestComputeCovariance);
    SUITE_ADD_TEST(suite, TestPZeroInEigenPivot);
    SUITE_ADD_TEST(suite, TestEigen);
    SUITE_ADD_TEST(suite, TestInvert);
    SUITE_ADD_TEST(suite, TestRxScore);
    SUITE_ADD_TEST(suite, TestRxAnomalyDetection);

    return suite;
}
