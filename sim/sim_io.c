/*
 *  Implements file I/O functions for simulation; used for both Linux and
 *  VxWorks simulations. Simulation framework (EOS and logging) should be
 *  initialized prior to calling these functions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <eos.h>
#include <eos_log.h>

EosStatus read_observation(const char* filename, const char* mode,
        void **data_ptr, uint64_t *size) {

    FILE *fp = NULL;
    fp = fopen(filename, mode);
    if (fp == NULL) {
        eos_logf(EOS_LOG_ERROR, "Could not open \"%s\"", filename);
        return EOS_ERROR;
    }

    // Seek to end of file to see how big it is
    fseek(fp, 0L, SEEK_END);
    *size = ftell(fp);

    // Allocate memory to store file contents
    *data_ptr = malloc(*size);
    if (*data_ptr == NULL) {
        eos_logf(EOS_LOG_ERROR,
            "Could not allocate space for \"%s\"", filename);
        return EOS_ERROR;
    }

    // Return to beginning of file
    fseek(fp, 0L, SEEK_SET);

    // Read contents to data_ptr and close file
    fread(*data_ptr, sizeof(unsigned char), *size, fp);
    fclose(fp);

    return EOS_SUCCESS;
}
