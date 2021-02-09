#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "eos_types_pub.h"
#include "eos_data.h"
#include "eos_log.h"
#include "eos_util.h"

#ifndef INFINITY
    #define INFINITY (1.0/0.0)
#endif

U32 _get_padding_bytes(U32 header_str_bytes, U32 alignment, U32 version_bytes) {
    U32 padding_bytes;
    padding_bytes = (
        alignment - ((header_str_bytes + version_bytes) % alignment)
    ) % alignment;
    if (padding_bytes == 0) {
        padding_bytes = alignment;
    }
    return padding_bytes;
}

/******** E-THEMIS ********/

EosStatus _load_etm_v1(const void* data, const U64 size,
                       EosEthemisObservation* obs, U32 header_bytes) {
    EosEndianness system;
    U32 header[ETM_HEADER_ENTRIES];
    U32* band_dims;
    U32 full_header_bytes;
    U32 band_data_offset;
    U32 i;

    if (eos_assert(data != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(obs != NULL)) { return EOS_ASSERT_ERROR; }

    full_header_bytes = header_bytes + (ETM_HEADER_ENTRIES * sizeof(U32));

    if (size < full_header_bytes) {
        eos_log(EOS_LOG_ERROR, "ETM file truncated before header.");
        return EOS_ETM_LOAD_ERROR;
    }

    system = system_endianness();

    memcpy(
        (void*)header, const_byte_offset(data, header_bytes),
        ETM_HEADER_ENTRIES * sizeof(U32)
    );
    for (i = 0; i < ETM_HEADER_ENTRIES; i++) {
        correct_endianness_U32(EOS_BIG_ENDIAN, system, &header[i]);
    }

    band_data_offset = full_header_bytes;

    // Band Dimensions start at header entry 2
    obs->observation_id = header[0];
    obs->timestamp = header[1];
    band_dims = &header[2];

    for (i = EOS_ETHEMIS_BAND_1; i < EOS_ETHEMIS_N_BANDS; i++) {
        EosObsShape shape;
        U32 band_size;
        U32 band_space;
        U32 band_data_bytes;
        U16* band_data;
        U32 j;

        shape.cols = band_dims[2*i];
        shape.rows = band_dims[2*i + 1];
        shape.bands = 1; /* One band at a time */
        band_size = shape.cols * shape.rows;
        band_space = obs->band_shape[i].rows * obs->band_shape[i].cols;
        if (band_size > band_space) {
            eos_logf(EOS_LOG_ERROR,
                "Insufficient space (%d) in destination to hold "
                "%d band %d data entries in ETM file.",
                band_space, band_size, i);
            return EOS_ETM_LOAD_ERROR;
        }
        obs->band_shape[i] = shape;

        band_data_bytes = band_size * sizeof(U16);
        if (band_data_bytes + band_data_offset > size) {
            eos_logf(EOS_LOG_ERROR,
                "ETM file truncated; expected at least %d bytes "
                "while reading band %d, but file size is %d",
                band_data_bytes + band_data_offset, i, size);
            return EOS_ETM_LOAD_ERROR;
        }

        if (band_size > 0) {
            band_data = obs->band_data[i];
            if (eos_assert(band_data != NULL)) { return EOS_ASSERT_ERROR; }
            memcpy(
                band_data,
                const_byte_offset(data, band_data_offset),
                band_data_bytes
            );
            for (j = 0; j < band_size; j++) {
                correct_endianness_U16(EOS_BIG_ENDIAN, system, &band_data[j]);
            }
        }

        band_data_offset += band_data_bytes;
    }

    if (size > band_data_offset) {
        eos_logf(EOS_LOG_WARN,
            "Expected %d bytes in ETM file but got %d.",
            band_data_offset, size);
    }
    return EOS_SUCCESS;
}

EosStatus load_etm(const void* data, const U64 size, EosEthemisObservation* obs) {
    U32 header_str_bytes;
    U32 padding_bytes;
    U32 header_start_bytes;
    U8 version;

    if (eos_assert(data != NULL)) { return EOS_ASSERT_ERROR; }

    header_str_bytes = strlen(ETM_HEADER_STR);
    padding_bytes = _get_padding_bytes(header_str_bytes, ETM_ALIGNMENT,
                                       ETM_VERSION_BYTES);
    header_start_bytes = header_str_bytes + padding_bytes
                                + ETM_VERSION_BYTES;

    if (size < header_start_bytes) {
        eos_log(EOS_LOG_ERROR, "ETM file too small for header.");
        return EOS_ETM_LOAD_ERROR;
    }

    if (strncmp((const CHAR*)data, ETM_HEADER_STR, header_str_bytes) != 0) {
        eos_log(EOS_LOG_ERROR, "Unexpected ETM header string.");
        return EOS_ETM_LOAD_ERROR;
    }

    version = ((const U8*)data)[header_str_bytes + padding_bytes];

    switch (version) {
        case 0x01:
            return _load_etm_v1(data, size, obs, header_start_bytes);
        default:
            eos_logf(EOS_LOG_ERROR, "Unknown ETM version %d", version);
            return EOS_ETM_VERSION_ERROR;
    }
}

/******** MISE ********/

EosStatus _load_mise_v1(const void* data, const U64 size,
                        EosMiseObservation* obs, U32 header_bytes) {
    EosEndianness system;
    U32 header[MISE_HEADER_ENTRIES];
    U32 full_header_bytes;
    U32 n_data_values, data_bytes;
    U32 i;

    if (eos_assert(data != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(obs != NULL)) { return EOS_ASSERT_ERROR; }

    full_header_bytes = header_bytes + (MISE_HEADER_ENTRIES * sizeof(U32));

    if (size < full_header_bytes) {
        eos_log(EOS_LOG_ERROR, "MISE file truncated before header.");
        return EOS_MISE_LOAD_ERROR;
    }

    system = system_endianness();

    memcpy((void*)header, 
           const_byte_offset(data, header_bytes),
           MISE_HEADER_ENTRIES * sizeof(U32));
    for (i = 0; i < MISE_HEADER_ENTRIES; i++) {
        correct_endianness_U32(EOS_BIG_ENDIAN, system, &header[i]);
    }

    /* Dimensions start at header entry 2 */
    obs->observation_id = header[0];
    obs->timestamp = header[1];
    obs->shape.cols = header[2];
    obs->shape.rows = header[3];
    obs->shape.bands = header[4];
    if (obs->shape.bands != EOS_MISE_N_BANDS) {
        eos_logf(EOS_LOG_INFO,
                 "Read %d MISE bands (expecting %d)",
                 obs->shape.bands, EOS_MISE_N_BANDS);
    }

    /* Copy the file data */
    n_data_values = obs->shape.cols * obs->shape.rows * obs->shape.bands;
    data_bytes = n_data_values * sizeof(U16);
    if (full_header_bytes + data_bytes > size) {
        eos_logf(EOS_LOG_ERROR,
                 "MISE file truncated; expected at least %d bytes, "
                 "but file size is %d",
                 full_header_bytes + data_bytes, size);
        return EOS_MISE_LOAD_ERROR;
    }
    memcpy(obs->data, 
           const_byte_offset(data, full_header_bytes),
           data_bytes);
    for (i = 0; i < data_bytes / sizeof(U16) ; i++) {
        correct_endianness_U16(EOS_BIG_ENDIAN, system, &(obs->data[i]));
    }

    return EOS_SUCCESS;
}

EosStatus load_mise(const void* data, const U64 size, EosMiseObservation* obs) {
    U32 header_str_bytes;
    U32 padding_bytes;
    U32 header_start_bytes;
    U8 version;

    if (eos_assert(data != NULL)) { return EOS_ASSERT_ERROR; }

    header_str_bytes = strlen(MISE_HEADER_STR);
    padding_bytes = _get_padding_bytes(header_str_bytes, MISE_ALIGNMENT,
                                       MISE_VERSION_BYTES);
    header_start_bytes = header_str_bytes + padding_bytes
                                + MISE_VERSION_BYTES;

    if (size < header_start_bytes) {
        eos_log(EOS_LOG_ERROR, "MISE file too small for header.");
        return EOS_MISE_LOAD_ERROR;
    }

    if (strncmp((const CHAR*)data, MISE_HEADER_STR, header_str_bytes) != 0) {
        eos_log(EOS_LOG_ERROR, "Unexpected MISE header string.");
        return EOS_MISE_LOAD_ERROR;
    }

    version = ((const U8*)data)[header_str_bytes + padding_bytes];

    switch (version) {
        case 0x01:
            return _load_mise_v1(data, size, obs, header_start_bytes);
        default:
            eos_logf(EOS_LOG_ERROR, "Unknown MISE version %d", version);
            return EOS_MISE_VERSION_ERROR;
    }
}

/******** PIMS ********/

/* Reads in only NUM_MODES, MAX_BINS, and NUM_OBS from the file. */
EosStatus read_pims_observation_attributes(const void* data, const U64 size, U32* num_modes, U32* max_bins, U32* num_obs){
    EosEndianness system = system_endianness();

    /* Read header. */
    U32 header_str_bytes = strlen(PIMS_HEADER_STR);
    U32 padding_bytes = _get_padding_bytes(header_str_bytes, PIMS_ALIGNMENT, PIMS_VERSION_BYTES);
    U32 header_bytes = header_str_bytes + padding_bytes + PIMS_VERSION_BYTES;
    U32 real_header_bytes = PIMS_FILE_HEADER_ENTRIES * sizeof(U32);

    U32 file_header_bytes = header_bytes + real_header_bytes;
    if (size < file_header_bytes) {
        eos_log(EOS_LOG_ERROR, "PIMS file truncated before header.");
        return EOS_PIMS_LOAD_ERROR;
    }

    U32 header[PIMS_FILE_HEADER_ENTRIES];
    memcpy((void*)header, const_byte_offset(data, header_bytes), real_header_bytes);

    for (U32 i = 0; i < PIMS_FILE_HEADER_ENTRIES; ++i) {
        correct_endianness_U32(EOS_BIG_ENDIAN, system, &header[i]);
    }

    *num_modes = header[1];
    *max_bins = header[2];
    *num_obs = header[3];

    return EOS_SUCCESS;
}

EosStatus _load_pims_v1(const void* data, const U64 size,
                        EosPimsObservationsFile* file, U32 header_bytes) {

    if (eos_assert(data != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(file != NULL))  { return EOS_ASSERT_ERROR; }

    EosEndianness system = system_endianness();

    /* Read file headers. */
    U32 real_header_bytes = PIMS_FILE_HEADER_ENTRIES * sizeof(U32);
    U32 header[PIMS_FILE_HEADER_ENTRIES];

    U32 file_header_bytes = header_bytes + real_header_bytes;
    if (size < file_header_bytes) {
        eos_log(EOS_LOG_ERROR, "PIMS file truncated before header.");
        return EOS_PIMS_LOAD_ERROR;
    }

    memcpy((void*)header,
           const_byte_offset(data, header_bytes),
           real_header_bytes);

    for (U32 i = 0; i < PIMS_FILE_HEADER_ENTRIES; ++i) {
        correct_endianness_U32(EOS_BIG_ENDIAN, system, &header[i]);
    }

    file -> file_id = header[0];
    file -> num_modes = header[1];
    file -> max_bins = header[2];
    file -> num_observations = header[3];
    
    eos_logf(EOS_LOG_INFO, "Reading PIMS file: ID = %d, NUM_MODES = %d, MAX_BINS = %d, NUM_OBSERVATIONS = %d",
        file -> file_id, file -> num_modes, file -> max_bins, file -> num_observations);

    /* Read mode information. */
    eos_logf(EOS_LOG_INFO, "Reading mode information:");
    U32 bin_definitions_size = file -> max_bins * sizeof(F32);
    F32 bin_definitions[file -> max_bins];

    if (size < file_header_bytes + (file -> num_modes) * bin_definitions_size) {
        eos_log(EOS_LOG_ERROR, "PIMS file truncated before mode information.");
        return EOS_PIMS_LOAD_ERROR;
    }

    for(U32 mode = 0; mode < file -> num_modes; ++mode){
        memcpy((void*)bin_definitions,
                const_byte_offset(data, file_header_bytes + mode * bin_definitions_size),
                bin_definitions_size);

        for (U32 i = 0; i < file -> max_bins; ++i) {
            correct_endianness_F32(EOS_BIG_ENDIAN, system, &(bin_definitions[i]));
        }

        file -> modes_info[mode].num_bins = file -> max_bins;
        for (U32 i = 0; i < file -> max_bins; ++i) {

            /* Mode bin definitions are terminated with INFINITY. */
            if(bin_definitions[i] == INFINITY){
                file -> modes_info[mode].num_bins = i;
                break;
            }

            file -> modes_info[mode].bin_log_energies[i] = bin_definitions[i];
            eos_logf(EOS_LOG_INFO, "- Mode %d: Bin %d = %0.2f", mode, i, bin_definitions[i]);
        }

        /* Check on mode's num_bins. */
        if(file -> modes_info[mode].num_bins == 0){
            eos_logf(EOS_LOG_ERROR, "Mode %d has 0 bins.", mode);
            return EOS_PIMS_LOAD_ERROR;
        }

        eos_logf(EOS_LOG_INFO, "- Mode %d has %d bins.", mode, file -> modes_info[mode].num_bins);
    }

    /* Read observations. */
    U32 file_header_and_mode_bytes = file_header_bytes + (file -> num_modes) * bin_definitions_size; 
    U32 obs_header_size = PIMS_OBS_HEADER_ENTRIES * sizeof(U32);
    U32 obs_header[PIMS_OBS_HEADER_ENTRIES];
    U32 obs_data_size = (file -> max_bins) * sizeof(U32);
    U32 obs_data[file -> max_bins];
    U32 obs_size = obs_header_size + obs_data_size;

    if (size < file_header_and_mode_bytes + (file -> num_observations) * obs_size) {
        eos_log(EOS_LOG_ERROR, "PIMS file truncated before observations.");
        return EOS_PIMS_LOAD_ERROR;
    }

    for(U32 obs = 0; obs < file -> num_observations; ++obs){

        /* Read observation header. */
        memcpy((void*)obs_header,
                const_byte_offset(data, file_header_and_mode_bytes + obs * obs_size),
                obs_header_size);

        for (U32 i = 0; i < PIMS_OBS_HEADER_ENTRIES; ++i) {
            correct_endianness_U32(EOS_BIG_ENDIAN, system, &obs_header[i]);
        }

        file -> observations[obs].observation_id = obs_header[0];
        file -> observations[obs].timestamp = obs_header[1];
        file -> observations[obs].num_bins = obs_header[2];
        file -> observations[obs].mode = obs_header[3];

        eos_logf(EOS_LOG_INFO, "Reading PIMS observation: ID = %d, TIMESTAMP = %d, NUM_BINS = %d, MODE = %d",
                 obs_header[0], obs_header[1], obs_header[2], obs_header[3]);

        /* Read observation data, and correct for endian-ness. */
        memcpy((void*)obs_data,
                const_byte_offset(data, file_header_and_mode_bytes + obs * obs_size + obs_header_size),
                obs_data_size);

        for(U32 i = 0; i < file -> max_bins; ++i){
            correct_endianness_U32(EOS_BIG_ENDIAN, system, &obs_data[i]);
        }

        /* Check if each entry in obs_data fits into the pims_count_t datatype.
         * Otherwise, clip to the maximum value that we can store in the
         * pims_count_t datatype, based on the flag EOS_PIMS_U16_DATA . */
        for(U32 i = 0; i < file -> max_bins; ++i){
            if(obs_data[i] > PIMS_COUNT_T_MAX){
                obs_data[i] = PIMS_COUNT_T_MAX;
            }
        }

        /* Assign to the current observation's counts.
         * There is an implicit cast here if EOS_PIMS_U16_DATA is set. */
        for(U32 i = 0; i < file -> max_bins; ++i){
            file -> observations[obs].bin_counts[i] = obs_data[i];
        }

        /* For the bin definitions, just use the mode's bin definitions. */
        U32 mode = file -> observations[obs].mode;
        file -> observations[obs].bin_log_energies = file -> modes_info[mode].bin_log_energies;

        /* Check that the number of bins match what is given by the mode. */
        if(file -> observations[obs].num_bins != file -> modes_info[mode].num_bins){
            eos_logf(EOS_LOG_ERROR, "Observation %d has %d bins, but associated mode %d has only %d bins.",
                     obs, file -> observations[obs].num_bins, mode, file -> modes_info[mode].num_bins);
            return EOS_PIMS_LOAD_ERROR;
        }
    }

    return EOS_SUCCESS;
}

EosStatus load_pims(const void* data, const U64 size, EosPimsObservationsFile* file) {
    U32 header_str_bytes;
    U32 padding_bytes;
    U32 header_start_bytes;
    U8 version;

    if (eos_assert(data != NULL)) { return EOS_ASSERT_ERROR; }

    header_str_bytes = strlen(PIMS_HEADER_STR);
    padding_bytes = _get_padding_bytes(header_str_bytes, PIMS_ALIGNMENT, PIMS_VERSION_BYTES);
    header_start_bytes = header_str_bytes + padding_bytes + PIMS_VERSION_BYTES;

    if (size < header_start_bytes) {
        eos_log(EOS_LOG_ERROR, "PIMS file too small for header.");
        return EOS_PIMS_LOAD_ERROR;
    }

    if (strncmp((const CHAR*)data, PIMS_HEADER_STR, header_str_bytes) != 0) {
        eos_log(EOS_LOG_ERROR, "Unexpected PIMS header string.");
        return EOS_PIMS_LOAD_ERROR;
    }

    version = ((const U8*)data)[header_str_bytes + padding_bytes];

    switch (version) {
        case 0x01:
            return _load_pims_v1(data, size, file, header_start_bytes);
        default:
            eos_logf(EOS_LOG_ERROR, "Unknown PIMS version %d", version);
            return EOS_PIMS_VERSION_ERROR;
    }

    return EOS_SUCCESS;
}
