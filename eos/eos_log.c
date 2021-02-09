#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "eos_log.h"

#ifdef EOS_UNSAFE_LOG
#define GEN_LOG(buffer, max_size, fmt, args) (vsprintf(buffer, fmt, args))
#else
#define GEN_LOG(buffer, max_size, fmt, args) (vsnprintf(buffer, max_size, fmt, args))
#endif

static void (*eos_log_function)(EosLogType, const CHAR*) = NULL;

void _eos_log_noop(EosLogType type, ...) { (void)type; }

void default_log_function(EosLogType type, const CHAR* message) {
#if LOG_TO_STDOUT
    CHAR *prefix;
    switch (type) {
        case EOS_LOG_DEBUG:
            prefix = "DEBUG";
            break;
        case EOS_LOG_INFO:
            prefix = "INFO";
            break;
        case EOS_LOG_WARN:
            prefix = "WARNING";
            break;
        case EOS_LOG_ERROR:
            prefix = "ERROR";
            break;
        case EOS_LOG_KEY_VALUE:
            return;
        default:
            prefix = "LOG";
    }
    printf("%s: %s\n", prefix, message);
#else
    (void)type;
    (void)(message);
#endif
}

void log_init(void (*log_function)(EosLogType, const CHAR*)) {
    eos_log_function = log_function;
}

void log_teardown() {
    eos_log_function = NULL;
}

void _eos_logf(EosLogType type, const CHAR* message_fmt, ...) {
    CHAR buffer[MAX_LOG_MSG_SIZE];
    I32 retval;
    va_list args;
    va_start(args, message_fmt);
    retval = GEN_LOG(buffer, MAX_LOG_MSG_SIZE, message_fmt, args);
    if (retval >= MAX_LOG_MSG_SIZE) {
        eos_log(EOS_LOG_ERROR, "Log message too large.");
    } else {
        eos_log(type, buffer);
    }
    va_end(args);
}

void _eos_log(EosLogType type, const CHAR* message) {
    if (eos_log_function == NULL) {
        default_log_function(type, message);
    } else {
        eos_log_function(type, message);
    }
}

U32 _eos_assert(U32 condition, const CHAR* filename, U32 line,
                const CHAR* condition_str) {
    if (condition) {
        return EOS_FALSE;
    } else {
        eos_logf(EOS_LOG_ERROR, "%s, %d: assertion '%s' failed",
                 filename, line, condition_str);
        return EOS_TRUE;
    }
}
