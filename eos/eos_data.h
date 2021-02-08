#ifndef JPL_EOS_DATA
#define JPL_EOS_DATA

#include "eos_types.h"

#define ETM_ALIGNMENT 4
#define ETM_HEADER_STR "EOS_ETHEMIS"
#define ETM_HEADER_PAD_VALUE 0xFF
#define ETM_VERSION_BYTES 1
#define ETM_HEADER_ENTRIES 8  /* id, timestamp, c/r, c/r, c/r */

#define MISE_ALIGNMENT 4
#define MISE_HEADER_STR "EOS_MISE"
#define MISE_HEADER_PAD_VALUE 0xFF
#define MISE_VERSION_BYTES 1
#define MISE_HEADER_ENTRIES 5 /* id, timestamp, cols, rows, bands */

EosStatus load_etm(const void* data, const U64 size, EosEthemisObservation* obs);
EosStatus load_mise(const void* data, const U64 size, EosMiseObservation* obs);

#endif
