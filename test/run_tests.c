#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"

#define MAX_TEST_SUITES 100

CuSuite *CuUtilGetSuite();
CuSuite *CuMemoryGetSuite();
CuSuite *CuLogGetSuite();
CuSuite *CuParamGetSuite();
CuSuite *CuEthemisGetSuite();
CuSuite *CuDataGetSuite();
CuSuite *CuEosGetSuite();
CuSuite *CuMiseGetSuite();
CuSuite *CuPimsGetSuite();
CuSuite *CuHeapGetSuite();

unsigned int run_all(void)
{
    CuString *output = CuStringNew();
    CuSuite *suite = CuSuiteNew();

    CuSuite* suites[MAX_TEST_SUITES];
    int n_suites = 0;
    suites[n_suites++] = CuUtilGetSuite();
    suites[n_suites++] = CuMemoryGetSuite();
    suites[n_suites++] = CuLogGetSuite();
    suites[n_suites++] = CuParamGetSuite();
    suites[n_suites++] = CuEthemisGetSuite();
    suites[n_suites++] = CuDataGetSuite();
    suites[n_suites++] = CuEosGetSuite();
    suites[n_suites++] = CuMiseGetSuite();
    suites[n_suites++] = CuPimsGetSuite();
    suites[n_suites++] = CuHeapGetSuite();

    int i;
    for (i = 0; i < n_suites; i++) {
        CuSuiteAddSuite(suite, suites[i]);
    }

    CuSuiteRun(suite);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    CuStringDelete(output);
    for (i = 0; i < n_suites; i++) {
        CuSuiteDelete(suites[i]);
    }

    unsigned int failCount = suite->failCount;
    free(suite);
    return failCount;
}

int main(void) {
    unsigned int failCount = run_all();
    if (failCount == 0)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}
