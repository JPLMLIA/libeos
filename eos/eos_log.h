#ifndef JPL_EOS_LOG
#define JPL_EOS_LOG

#include "eos_types.h"

#define EOS_LOGGING_ON   1    // @logset
#define LOG_TO_STDOUT    0    // @logstdout
#define MAX_LOG_MSG_SIZE 1000 // @logsize

void log_init(void (*log_function)(EosLogType, const CHAR*));
void log_teardown();
void _eos_log(EosLogType type, const CHAR* message);
void _eos_logf(EosLogType type, const CHAR* message_fmt, ...);
U32 _eos_assert(U32 condition, const CHAR* filename, U32 line, const CHAR* condition_str);
void _eos_log_noop(EosLogType type, ...);

// @eos-assert+1
#define eos_assert(e) (_eos_assert(e, __FILE__, __LINE__, #e))

#if EOS_LOGGING_ON
#define eos_log(type, ...) (_eos_log(type, __VA_ARGS__))
#define eos_logf(type, ...) (_eos_logf(type, __VA_ARGS__))
#else
#define eos_log(type, ...) (_eos_log_noop(type, __VA_ARGS__))
#define eos_logf(type, ...) (_eos_log_noop(type, __VA_ARGS__))
#endif

#endif
