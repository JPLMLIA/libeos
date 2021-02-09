#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <eos_log.h>
#include <eos_params.h>
#include <eos.h>
#include "CuTest.h"

CuString *output = NULL;

void save_log(EosLogType type, const char* message) {
    (void)type;
    if (output != NULL) {
        CuStringAppend(output, message);
    }
}

void TestMessageOverflow(CuTest *ct) {
    output = CuStringNew();
    log_init(save_log);
    char monster_string[2*MAX_LOG_MSG_SIZE];
    memset(monster_string, 0, 2*MAX_LOG_MSG_SIZE);
    memset(monster_string, 'x', 2*MAX_LOG_MSG_SIZE - 1);
    eos_logf(EOS_LOG_DEBUG, monster_string);
    CuAssertStrEquals(ct, "Log message too large.", output->buffer);
    CuStringDelete(output);
    log_teardown();
}

void TestAssert(CuTest *ct) {
    output = CuStringNew();
    log_init(save_log);
    eos_assert(1 == 2);
    CuAssertStrEquals(ct, "log_test.c, 34: assertion '1 == 2' failed", output->buffer);
    CuStringDelete(output);
    log_teardown();
}

void TestParamLog(CuTest *ct) {
    EosStatus status;
    output = CuStringNew();
    log_init(save_log);
    status = param_check(0.0 > 1.0);
    CuAssertIntEquals(ct, EOS_PARAM_ERROR, status);
    CuAssertStrEquals(ct,
        "Parameter check \"0.0 > 1.0\" failed",
        output->buffer);
    CuStringDelete(output);
    log_teardown();
}

void TestLogNoOp(CuTest *ct) {
    (void)ct;
    // Nothing to test; just call to
    // make sure there is no error
    _eos_log_noop(EOS_LOG_DEBUG);
}

CuSuite* CuLogGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TestLogNoOp);

#if EOS_LOGGING_ON
    SUITE_ADD_TEST(suite, TestMessageOverflow);
    SUITE_ADD_TEST(suite, TestAssert);
    SUITE_ADD_TEST(suite, TestParamLog);
#endif

    return suite;
}
