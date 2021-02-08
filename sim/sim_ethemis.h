#ifndef JPL_EOS_SIM_ETHEMIS
#define JPL_EOS_SIM_ETHEMIS

#include <libconfig.h>

#include <eos.h>

EosStatus run_ethemis_sim(char *inputfile, char *outputfile, config_t *config);

#endif
