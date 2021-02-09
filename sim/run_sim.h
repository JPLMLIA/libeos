#ifndef JPL_EOS_RUN_SIM
#define JPL_EOS_RUN_SIM

#ifndef EOS_NO_LIBCONFIG
#include <libconfig.h>
#endif

#ifdef EOS_NO_LIBCONFIG
    typedef char* config_ptr;
#else
    typedef config_t* config_ptr;
#endif

#endif
