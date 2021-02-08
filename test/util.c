#include <stdlib.h>
#include <stdio.h>
#include <eos_types_pub.h>
#include <eos_memory.h>

#include "util.h"

/* For memory leak detection. */
U32 current_memory_state(){
    return lifo_stack_entries();
}

U32 memory_leak(U32 prev_state){
    return (prev_state != current_memory_state());
}

FILE *open_resource(CuTest *ct, char* filename, char* mode) {
    char* testenv = getenv("EOSTESTDATA");
    CuAssertPtrNotNull(ct, testenv);
    char buffer[4096];
    sprintf(buffer, "%s/%s", testenv, filename);
    return fopen(buffer, mode);
}

void read_resource(CuTest *ct, char* filename, void **data_ptr, unsigned int *size) {
    FILE *fp = NULL;
    fp = open_resource(ct, filename, "r");
    CuAssertPtrNotNull(ct, fp);
    if (fp == NULL) { return; }
    fseek(fp, 0L, SEEK_END);
    *size = ftell(fp);
    *data_ptr = malloc(*size);
    fseek(fp, 0L, SEEK_SET);
    fread(*data_ptr, sizeof(unsigned char), *size, fp);
    fclose(fp);
}

EosInitParams default_init_params() {
    EosInitParams init;
    init.mise_max_bands = EOS_MISE_N_BANDS;
    return init;
}

/*
 * Initialize a default simple observation
 */
void InitEthemisObsValue(EosEthemisObservation* obs,
             const int nrows, const int ncols,
             const uint16_t default_value) {
    obs->observation_id = 1;
    obs->timestamp = 0;

    EosEthemisBand b;
    for (b=EOS_ETHEMIS_BAND_1; b < EOS_ETHEMIS_N_BANDS; b++) {
        obs->band_shape[b].rows = nrows;
        obs->band_shape[b].cols = ncols;
        obs->band_data[b] = (uint16_t*) malloc(sizeof(uint16_t) *
                               (nrows * ncols));
        int r, c;
        for (r=0; r < nrows; r++) {
            for (c=0; c < ncols; c++) {
                obs->band_data[b][r*ncols + c] = default_value;
            }
        }
    }
}

/*
 * Initialize a default simple observation with all zeros
 */
void InitEthemisObs(EosEthemisObservation* obs,
        const int nrows, const int ncols) {
    InitEthemisObsValue(obs, nrows, ncols, 0);
}

/*
 * Free memory associated with a simulated observation
 */
void FreeEthemisObs(EosEthemisObservation* obs) {
    EosEthemisBand b;
    for (b = EOS_ETHEMIS_BAND_1; b < EOS_ETHEMIS_N_BANDS; b++) {
        if (obs->band_data[b]) {
            free(obs->band_data[b]);
        }
    }
}

/*
 * Initialize a default simple MISE observation
 */
void InitMiseObsValue(EosMiseObservation* obs,
             const int nrows, const int ncols, const int nbands,
             const uint16_t default_value) {
    int i;
    const int nentries = (nrows * ncols * nbands);
    obs->observation_id = 0;
    obs->timestamp = 0;
    obs->shape.rows = nrows;
    obs->shape.cols = ncols;
    obs->shape.bands = nbands;
    obs->data = (uint16_t*) malloc(sizeof(uint16_t) * nentries);
    for (i = 0; i < nentries; i++) {
        obs->data[i] = default_value;
    }
}

/*
 * Initialize a default simple MISE observation with all zeros
 */
void InitMiseObs(EosMiseObservation* obs,
        const int nrows, const int ncols, const int nbands) {
    InitMiseObsValue(obs, nrows, ncols, nbands, 0);
}

/*
 * Free memory associated with a simulated MISE observation
 */
void FreeMiseObs(EosMiseObservation* obs) {
    free(obs->data);
}
