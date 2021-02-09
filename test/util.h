#ifndef EOS_TEST_UTIL
#define EOS_TEST_UTIL
#include <eos.h>
#include <eos_types.h>
#include "CuTest.h"

U32 current_memory_state();
U32 memory_leak(U32 prev_state);

void default_init_params_test(EosInitParams *init);
EosStatus default_init_params_pims(EosInitParams* init_params);

void read_resource(CuTest *ct, char* filename, void **data_ptr, unsigned int *size);

void InitEthemisObs(EosEthemisObservation* obs,
        const int nrows, const int ncols);

void InitEthemisObsValue(EosEthemisObservation* obs,
             const int nrows, const int ncols,
             const uint16_t default_value);

void FreeEthemisObs(EosEthemisObservation* obs);

void InitMiseObsValue(EosMiseObservation* obs,
             const int nrows, const int ncols, const int nbands,
             const uint16_t default_value);

void InitMiseObs(EosMiseObservation* obs,
        const int nrows, const int ncols, const int nbands);

void FreeMiseObs(EosMiseObservation* obs);

EosPimsObservationsFile InitPimsObsFile(U32 num_modes, U32 max_bins, U32 num_obs);
EosPimsObservation InitPimsObs(U32 num_bins, U32 id);
void FreePimsObs(EosPimsObservation obs);
void FreePimsObsFile(EosPimsObservationsFile obs_file);

#endif
