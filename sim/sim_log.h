#ifndef JPL_EOS_SIM_LOG
#define JPL_EOS_SIM_LOG

#include <eos.h>

EosStatus sim_log_init(const char *outputfile);
void sim_log_teardown(void);
void log_function(EosLogType type, const char* message);

#endif
