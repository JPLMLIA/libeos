#ifndef JPL_EOS_SIM_IO
#define JPL_EOS_SIM_IO

#include <eos.h>

EosStatus read_observation(const char* filename, const char* mode,
    void **data_ptr, uint64_t *size);

#endif
