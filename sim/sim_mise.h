#ifndef JPL_EOS_SIM_MISE
#define JPL_EOS_SIM_MISE

#include <libconfig.h>
#include <eos.h>

EosStatus run_mise_sim(char *inputfile, char *outputfile, config_t *config);

#endif
