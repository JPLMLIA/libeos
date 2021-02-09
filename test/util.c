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
    size_t result = fread(*data_ptr, sizeof(unsigned char), *size, fp);
    CuAssertIntEquals(ct, result, sizeof(unsigned char) * (*size));
    fclose(fp);
}

void default_init_params_test(EosInitParams *init) {
    init->mise_max_bands = EOS_MISE_N_BANDS;
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


/*
 * Allocates memory for a file of PIMS observations.
 */
EosPimsObservationsFile InitPimsObsFile(U32 num_modes, U32 max_bins, U32 num_obs){
    /* Memory for the array of EosPimsModeInfo. */
    EosPimsModeInfo* modes_info = malloc(sizeof(EosPimsModeInfo) * num_modes);

    /* Memory for each EosPimsModeInfo. */
    for(U32 mode = 0; mode < num_modes; ++mode){
        modes_info[mode].bin_log_energies = malloc(sizeof(F32) * max_bins);
    }

    /* Memory for the array of observations. */
    EosPimsObservation* observations = malloc(sizeof(EosPimsObservation) * num_obs);

    /* Memory for each observation's counts. */
    for(U32 obs = 0; obs < num_obs; ++obs){
        observations[obs].bin_counts = malloc(sizeof(U32) * max_bins);
    }

    EosPimsObservationsFile obs_file = {
        .file_id = 0,
        .num_modes = num_modes,
        .max_bins = max_bins,
        .num_observations = num_obs,
        .modes_info = modes_info,
        .observations = observations,
    };
    return obs_file;
}

/* Free arrays pointed to by current observation. */
void FreePimsObsFile(EosPimsObservationsFile obs_file){

    for(U32 mode = 0; mode < obs_file.num_modes; ++mode){
        free(obs_file.modes_info[mode].bin_log_energies);
    }
    free(obs_file.modes_info);

    for(U32 obs = 0; obs < obs_file.num_observations; ++obs){
        free(obs_file.observations[obs].bin_counts);
    }
    free(obs_file.observations);
}

/*
 * Creates a 'fake' PIMS observation, with the given bins.
 * Counts for each bin are given by the value 'id'.
 * Bin definitions are the same for all values of 'id'.
 */
EosPimsObservation InitPimsObs(U32 num_bins, U32 id){
    pims_count_t* bin_counts = malloc(sizeof(pims_count_t) * num_bins);
    F32* bin_log_energies = malloc(sizeof(F32) * num_bins);

    for(U32 i = 0; i < num_bins; ++i){
        bin_counts[i] = id;
        bin_log_energies[i] = i * 2.5;
    }

    EosPimsObservation obs = {
        .observation_id = id,
        .timestamp = id,
        .num_bins = num_bins,
        .mode = 0,
        .bin_counts = bin_counts,
        .bin_log_energies = bin_log_energies,
    };
    return obs;
}

/* Free arrays pointed to by current observation. */
void FreePimsObs(EosPimsObservation obs){
    free(obs.bin_counts);
    free(obs.bin_log_energies);
}

/* Defaults for 'init_params' for PIMS tests. */
EosStatus default_init_params_pims(EosInitParams* init_params){
    EosParams params;
    EosStatus status = eos_init_default_params(&params);
    if(status != EOS_SUCCESS){
        return status;
    }
    init_params -> pims_params = params.pims;
    init_params -> mise_max_bands = 0;
    return EOS_SUCCESS;
}
