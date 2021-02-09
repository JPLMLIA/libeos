#include <stdlib.h>
#include <stdint.h>
#include <float.h>

#include <eos_util.h>
#include "CuTest.h"
#include "util.h"

void TestByteSwapU32(CuTest *ct) {
    uint32_t i = 0x12345678;
    byte_swap_U32(&i);
    CuAssertIntEquals(ct, 0x78563412, i);
}

void TestByteSwapU16(CuTest *ct) {
    uint16_t i = 0x1234;
    byte_swap_U16(&i);
    CuAssertIntEquals(ct, 0x3412, i);
}

void TestByteOrderCorrection(CuTest *ct) {
    uint32_t i = 0x12345678;
    correct_endianness_U32(EOS_BIG_ENDIAN, EOS_BIG_ENDIAN, &i);
    CuAssertIntEquals(ct, 0x12345678, i);
    correct_endianness_U32(EOS_LITTLE_ENDIAN, EOS_LITTLE_ENDIAN, &i);
    CuAssertIntEquals(ct, 0x12345678, i);
    correct_endianness_U32(EOS_LITTLE_ENDIAN, EOS_BIG_ENDIAN, &i);
    CuAssertIntEquals(ct, 0x78563412, i);
    correct_endianness_U32(EOS_BIG_ENDIAN, EOS_LITTLE_ENDIAN, &i);
    CuAssertIntEquals(ct, 0x12345678, i);

    uint16_t j = 0x1234;
    correct_endianness_U16(EOS_BIG_ENDIAN, EOS_BIG_ENDIAN, &j);
    CuAssertIntEquals(ct, 0x1234, j);
    correct_endianness_U16(EOS_LITTLE_ENDIAN, EOS_LITTLE_ENDIAN, &j);
    CuAssertIntEquals(ct, 0x1234, j);
    correct_endianness_U16(EOS_LITTLE_ENDIAN, EOS_BIG_ENDIAN, &j);
    CuAssertIntEquals(ct, 0x3412, j);
    correct_endianness_U16(EOS_BIG_ENDIAN, EOS_LITTLE_ENDIAN, &j);
    CuAssertIntEquals(ct, 0x1234, j);
}

void TestIMAX(CuTest *ct) {
    long int a = 0, b = 1;
    CuAssertIntEquals(ct, eos_imax(a, b), 1);
    CuAssertIntEquals(ct, eos_imax(b, a), 1);
    CuAssertIntEquals(ct, eos_imax(a, a), 0);
}

void TestIMIN(CuTest *ct) {
    long int a = 1, b = 0;
    CuAssertIntEquals(ct, eos_imin(a, b), 0);
    CuAssertIntEquals(ct, eos_imin(b, a), 0);
    CuAssertIntEquals(ct, eos_imin(a, a), 1);
}

void TestUMIN(CuTest *ct) {
    unsigned int a = 1, b = 0;
    CuAssertIntEquals(ct, eos_umin(a, b), 0);
    CuAssertIntEquals(ct, eos_umin(b, a), 0);
    CuAssertIntEquals(ct, eos_umin(a, a), 1);
}

void TestUMAX(CuTest *ct) {
    unsigned int a = 10, b = 1;
    CuAssertIntEquals(ct, eos_umin(a, b), 1);
    CuAssertIntEquals(ct, eos_umin(b, a), 1);
    CuAssertIntEquals(ct, eos_umin(a, a), 10);
}

void TestLMAX(CuTest *ct) {
    long int a = 0, b = 1;
    CuAssertIntEquals(ct, eos_lmax(a, b), 1);
    CuAssertIntEquals(ct, eos_lmax(b, a), 1);
    CuAssertIntEquals(ct, eos_lmax(a, a), 0);
}

void TestLMIN(CuTest *ct) {
    long int a = 1, b = 0;
    CuAssertIntEquals(ct, eos_lmin(a, b), 0);
    CuAssertIntEquals(ct, eos_lmin(b, a), 0);
    CuAssertIntEquals(ct, eos_lmin(a, a), 1);
}

void TestRound(CuTest *ct) {
    CuAssertIntEquals(ct, eos_round(1.0), 1);
    CuAssertIntEquals(ct, eos_round(1.234), 1);
    CuAssertIntEquals(ct, eos_round(1.5), 2);
}

void TestCeil(CuTest *ct) {
    CuAssertIntEquals(ct, eos_ceil(1.0), 1);
    CuAssertIntEquals(ct, eos_ceil(1.234), 2);
    CuAssertIntEquals(ct, eos_ceil(1.5), 2);
}

void TestFloor(CuTest *ct) {
    CuAssertIntEquals(ct, eos_floor(1.0), 1);
    CuAssertIntEquals(ct, eos_floor(1.234), 1);
    CuAssertIntEquals(ct, eos_floor(1.5), 1);
}

void TestNorms(CuTest *ct) {
    double x[5] = {-1, 2, -3, 4, -5};
    double result;
    result = eos_norm_sqeuclidean(5, x);
    CuAssertDblEquals(ct, 55, result, 1e-3);

    result = eos_norm_infinity(5, x);
    CuAssertDblEquals(ct, 5, result, 1e-3);

    result = eos_dsum(5, x);
    CuAssertDblEquals(ct, -3, result, 1e-3);
}

CuSuite* CuUtilGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestByteSwapU16);
    SUITE_ADD_TEST(suite, TestByteSwapU32);
    SUITE_ADD_TEST(suite, TestLMIN);
    SUITE_ADD_TEST(suite, TestLMAX);
    SUITE_ADD_TEST(suite, TestIMIN);
    SUITE_ADD_TEST(suite, TestUMIN);
    SUITE_ADD_TEST(suite, TestUMAX);
    SUITE_ADD_TEST(suite, TestIMAX);
    SUITE_ADD_TEST(suite, TestRound);
    SUITE_ADD_TEST(suite, TestFloor);
    SUITE_ADD_TEST(suite, TestCeil);
    SUITE_ADD_TEST(suite, TestNorms);
    SUITE_ADD_TEST(suite, TestByteOrderCorrection);

    return suite;
}
