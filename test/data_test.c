#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <eos_data.h>
#include <eos_log.h>
#include "CuTest.h"
#include "util.h"


void TestPublicLoadEtm(CuTest* ct) {
    void* data;
    uint32_t size;
    uint32_t r, c;
    EosEthemisBand b;
    EosEthemisObservation obs;
    EosStatus status;
    EosInitParams init_params = default_init_params();

    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    InitEthemisObs(&obs, 3, 3);
    read_resource(ct, "test_ethemis.etm", &data, &size);

    status = eos_load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 0xAB, obs.observation_id);
    CuAssertIntEquals(ct, 0xCD, obs.timestamp);
    for (b = EOS_ETHEMIS_BAND_1; b < EOS_ETHEMIS_N_BANDS; b++) {
        uint32_t exp = b + 1;
        uint32_t cols = obs.band_shape[b].cols;
        uint32_t rows = obs.band_shape[b].rows;
        CuAssertIntEquals(ct, exp, rows);
        CuAssertIntEquals(ct, exp, cols);
        for (r = 0; r < rows; r++) {
            for (c = 0; c < cols; c++) {
                CuAssertIntEquals(ct, exp, obs.band_data[b][r*cols + c]);
            }
        }
    }

    // Clean up
    free(data);
    FreeEthemisObs(&obs);

    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    
}

void TestPublicLoadEtmWithoutInit(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisObservation obs;
    EosStatus status;

    /* Omit call to eos_init() */
    //status = eos_init(&init_params, NULL, 0, NULL);
    //CuAssertIntEquals(ct, EOS_SUCCESS, status);

    InitEthemisObs(&obs, 3, 3);

    read_resource(ct, "test_ethemis.etm", &data, &size);

    status = eos_load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_NOT_INITIALIZED, status);

    // Clean up
    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadEtm(CuTest* ct) {
    void* data;
    uint32_t size;
    uint32_t r, c;
    EosEthemisBand b;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 3, 3);

    read_resource(ct, "test_ethemis.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 0xAB, obs.observation_id);
    CuAssertIntEquals(ct, 0xCD, obs.timestamp);
    for (b = EOS_ETHEMIS_BAND_1; b < EOS_ETHEMIS_N_BANDS; b++) {
        uint32_t exp = b + 1;
        uint32_t cols = obs.band_shape[b].cols;
        uint32_t rows = obs.band_shape[b].rows;
        CuAssertIntEquals(ct, exp, rows);
        CuAssertIntEquals(ct, exp, cols);
        for (r = 0; r < rows; r++) {
            for (c = 0; c < cols; c++) {
                CuAssertIntEquals(ct, exp, obs.band_data[b][r*cols + c]);
            }
        }
    }

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadEtmEmptyBand2(CuTest* ct) {
    void* data;
    uint32_t size;
    uint32_t r, c;
    EosEthemisBand b;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 10, 10);

    read_resource(ct, "test_ethemis_band2_empty.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 0, obs.observation_id);
    CuAssertIntEquals(ct, 0, obs.timestamp);
    for (b = EOS_ETHEMIS_BAND_1; b < EOS_ETHEMIS_N_BANDS; b++) {
        uint32_t exp = (b == 1) ? 0 : 10;
        uint32_t cols = obs.band_shape[b].cols;
        uint32_t rows = obs.band_shape[b].rows;
        CuAssertIntEquals(ct, exp, rows);
        CuAssertIntEquals(ct, exp, cols);
        for (r = 0; r < rows; r++) {
            for (c = 0; c < cols; c++) {
                CuAssertIntEquals(ct, 1, obs.band_data[b][r*cols + c]);
            }
        }
    }

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadEtmEmptyAllBands(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisBand b;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 10, 10);

    read_resource(ct, "test_ethemis_allbands_empty.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 0, obs.observation_id);
    CuAssertIntEquals(ct, 0, obs.timestamp);
    for (b = EOS_ETHEMIS_BAND_1; b < EOS_ETHEMIS_N_BANDS; b++) {
        uint32_t exp = 0;
        uint32_t cols = obs.band_shape[b].cols;
        uint32_t rows = obs.band_shape[b].rows;
        CuAssertIntEquals(ct, exp, rows);
        CuAssertIntEquals(ct, exp, cols);
    }

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadEmptyFile(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 3, 3);

    read_resource(ct, "test_ethemis_empty.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_ETM_LOAD_ERROR, status);

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadWrongInstrument(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 10, 10);

    read_resource(ct, "test_ethemis_wrong_inst.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_ETM_LOAD_ERROR, status);

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadWrongVersion(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 10, 10);

    read_resource(ct, "test_ethemis_wrong_version.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_ETM_VERSION_ERROR, status);

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadMissingVersion(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 10, 10);

    read_resource(ct, "test_ethemis_missing_version.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_ETM_VERSION_ERROR, status);

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadMissingHeader(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 10, 10);

    read_resource(ct, "test_ethemis_missing_header.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_ETM_LOAD_ERROR, status);

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadDataTruncated(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 10, 10);

    read_resource(ct, "test_ethemis_truncated_data.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_ETM_LOAD_ERROR, status);

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadHeaderTruncated(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 10, 10);

    read_resource(ct, "test_ethemis_truncated_header.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_ETM_LOAD_ERROR, status);

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadExtraData(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 10, 10);

    read_resource(ct, "test_ethemis_extra_data.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_SUCCESS, status); // Counts as success

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadDataTooBig(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisObservation obs;
    EosStatus status;

    InitEthemisObs(&obs, 2, 2);  // too small

    read_resource(ct, "test_ethemis.etm", &data, &size);

    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_ETM_LOAD_ERROR, status);

    free(data);
    FreeEthemisObs(&obs);
}

void TestLoadIntoNullObs(CuTest* ct) {
    void* data;
    uint32_t size;
    EosEthemisObservation obs = {0};
    EosStatus status;

    read_resource(ct, "test_ethemis.etm", &data, &size);

    /* No space allocated for band_data in obs */
    status = load_etm(data, size, &obs);
    CuAssertIntEquals(ct, EOS_ETM_LOAD_ERROR, status);

    free(data);
    FreeEthemisObs(&obs);
}

void TestPublicLoadMise(CuTest* ct) {
    void* data;
    uint32_t size;
    EosStatus status;
    EosMiseObservation obs;
    EosInitParams init_params = default_init_params();

    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    InitMiseObs(&obs, 10, 10, 5);
    read_resource(ct, "test_mise.mis", &data, &size);

    status = eos_load_mise(data, size, &obs);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    free(data);
    FreeMiseObs(&obs);

    status = eos_teardown();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

void TestPublicLoadMiseWithoutInit(CuTest* ct) {
    void* data;
    uint32_t size;
    EosStatus status;
    EosMiseObservation obs;

    InitMiseObs(&obs, 10, 10, 5);
    read_resource(ct, "test_mise.mis", &data, &size);

    status = eos_load_mise(data, size, &obs);
    CuAssertIntEquals(ct, EOS_NOT_INITIALIZED, status);

    free(data);
    FreeMiseObs(&obs);
}

void TestLoadMise(CuTest* ct) {
    void* data;
    uint32_t size;
    EosStatus status;
    EosMiseObservation obs;

    InitMiseObs(&obs, 10, 10, 5);

    read_resource(ct, "test_mise.mis", &data, &size);

    status = load_mise(data, size, &obs);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    free(data);
    FreeMiseObs(&obs);
}

void TestLoadMiseTruncated(CuTest* ct) {
    void* data;
    uint32_t size;
    EosStatus status;
    EosMiseObservation obs;

    InitMiseObs(&obs, 10, 10, 5);

    read_resource(ct, "test_mise.mis", &data, &size);

    // Report that the data is only 1 byte
    status = load_mise(data, 1, &obs);
    CuAssertIntEquals(ct, EOS_MISE_LOAD_ERROR, status);

    // Report that the data is only 15 bytes
    status = load_mise(data, 15, &obs);
    CuAssertIntEquals(ct, EOS_MISE_LOAD_ERROR, status);

    // Report that the data 10 bytes smaller than expected
    status = load_mise(data, size - 10, &obs);
    CuAssertIntEquals(ct, EOS_MISE_LOAD_ERROR, status);

    free(data);
    FreeMiseObs(&obs);
}

void TestLoadMiseWrongHeader(CuTest* ct) {
    void* data;
    uint32_t size;
    EosStatus status;
    EosMiseObservation obs;

    InitMiseObs(&obs, 10, 10, 5);

    read_resource(ct, "test_mise.mis", &data, &size);

    // Corrupt the file header
    ((char*) data)[0] = ' ';

    status = load_mise(data, size, &obs);
    CuAssertIntEquals(ct, EOS_MISE_LOAD_ERROR, status);

    free(data);
    FreeMiseObs(&obs);
}

void TestLoadMiseWrongVersion(CuTest* ct) {
    void* data;
    uint32_t size;
    EosStatus status;
    EosMiseObservation obs;

    InitMiseObs(&obs, 10, 10, 5);

    read_resource(ct, "test_mise.mis", &data, &size);

    // Corrupt the file version
    ((uint8_t*) data)[11] = 0;

    status = load_mise(data, size, &obs);
    CuAssertIntEquals(ct, EOS_MISE_VERSION_ERROR, status);

    free(data);
    FreeMiseObs(&obs);
}

CuSuite* CuDataGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestPublicLoadEtm);
    SUITE_ADD_TEST(suite, TestPublicLoadEtmWithoutInit);
    SUITE_ADD_TEST(suite, TestLoadEtm);
    SUITE_ADD_TEST(suite, TestLoadEtmEmptyBand2);
    SUITE_ADD_TEST(suite, TestLoadEtmEmptyAllBands);
    SUITE_ADD_TEST(suite, TestLoadEmptyFile);
    SUITE_ADD_TEST(suite, TestLoadWrongInstrument);
    SUITE_ADD_TEST(suite, TestLoadWrongVersion);
    SUITE_ADD_TEST(suite, TestLoadMissingVersion);
    SUITE_ADD_TEST(suite, TestLoadMissingHeader);
    SUITE_ADD_TEST(suite, TestLoadDataTruncated);
    SUITE_ADD_TEST(suite, TestLoadHeaderTruncated);
    SUITE_ADD_TEST(suite, TestLoadExtraData);
    SUITE_ADD_TEST(suite, TestLoadDataTooBig);
    SUITE_ADD_TEST(suite, TestLoadIntoNullObs);

    SUITE_ADD_TEST(suite, TestPublicLoadMise);
    SUITE_ADD_TEST(suite, TestPublicLoadMiseWithoutInit);
    SUITE_ADD_TEST(suite, TestLoadMise);
    SUITE_ADD_TEST(suite, TestLoadMiseTruncated);
    SUITE_ADD_TEST(suite, TestLoadMiseWrongHeader);
    SUITE_ADD_TEST(suite, TestLoadMiseWrongVersion);

    return suite;
}
