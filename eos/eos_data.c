#include <string.h>
#include <stdlib.h>

#include "eos_data.h"
#include "eos_log.h"
#include "eos_util.h"

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
