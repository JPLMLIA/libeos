#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <eos.h>
#include "CuTest.h"
#include "util.h"

void TestDoubleInit(CuTest* ct) {
    EosStatus status;
    EosInitParams init_params = default_init_params();

    /* Call eos_init() twice - should be okay and re-init */
    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_init(&init_params, NULL, 0, NULL);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

}

void TestInsufficientMemoryInit(CuTest* ct) {
    EosStatus status;
    EosInitParams init_params = default_init_params();

    uint8_t data[8];

    /* Call eos_init() twice - should be okay and re-init */
    status = eos_init(&init_params, &data, 8, NULL);
    CuAssertIntEquals(ct, EOS_INSUFFICIENT_MEMORY, status);
}

CuSuite* CuEosGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestDoubleInit);
    SUITE_ADD_TEST(suite, TestInsufficientMemoryInit);

    return suite;
}
