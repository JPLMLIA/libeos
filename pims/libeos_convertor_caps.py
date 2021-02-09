#!/usr/bin/env python

"""
Convert CAPS ELS data to LIBEOS binary files.
"""
# External dependencies.
from __future__ import division
import numpy as np
import os
from pathlib2 import Path
from datetime import datetime as dt
from matplotlib.dates import date2num
import yaml

# Internal dependencies.
from data_utils import get_ELS_data_no_interpolation
from libeos_convertor import PIMSFile, PIMSObs

# All timestamps are with respect to this date.
TIMESTAMP_START = date2num(dt(year=2000, month=1, day=1))

# Mode 0 is the normal, 63-bin mode. 
# Mode 1 is the energy-summed, 32-bin mode.
def compute_modes(energy_ranges):
    return (np.sum(~np.isnan(energy_ranges), axis=1) != 63).astype(np.uint32)

# Convert times in days to times in seconds, since the start of the year 2004.
# We use 2004 because CAPS observations start in 2004.
def convert_to_seconds(times):
    times -= TIMESTAMP_START
    times = np.round(times * (60 * 60 * 24)).astype(np.uint32)
    return times

# Based on how mode 0 and mode 1 are defined.
def compute_num_bins(modes):
    num_bins = np.zeros(modes.shape, dtype=np.uint32)
    num_bins[np.where(modes == 0)[0]] = 63
    num_bins[np.where(modes == 1)[0]] = 32
    return num_bins

# Converts a ELS data_file
def convert(els_data_file, quantity='anode5', start_time=dt.min, end_time=dt.max, index=0):

    # Read CAPS data.
    counts, energy_ranges, times = get_ELS_data_no_interpolation(els_data_file, quantity, start_time, end_time)

    # Convert and normalize times to seconds.
    times = convert_to_seconds(times)

    # Compute modes.
    modes = compute_modes(energy_ranges)

    # Compute num_bins.
    num_bins = compute_num_bins(modes)

    # Now, convert!
    observations_converted = [PIMSObs(obs_id=index,
                                      counts=timestep_counts,
                                      mode=mode,
                                      timestamp=timestamp,
                                      num_bins=timestep_bins)
                              for index, (timestep_counts, mode, timestamp, timestep_bins)
                              in enumerate(zip(counts, modes, times, num_bins))]

    # These are always fixed! 
    # Again, mode 0 is 63 bins, mode 1 is 32 bins.
    mode_bin_definitions = [
        [2.6040e+04, 2.2227e+04, 1.8991e+04, 1.6256e+04, 
         1.3876e+04, 1.1867e+04, 1.0143e+04, 8.6740e+03, 
         7.4150e+03, 6.3360e+03, 5.4160e+03, 4.6300e+03,
         3.9560e+03, 3.3830e+03, 2.8900e+03, 2.4710e+03, 
         2.1120e+03, 1.8050e+03, 1.5440e+03, 1.3190e+03, 
         1.1280e+03, 9.6410e+02, 8.2400e+02, 7.0430e+02,
         6.0180e+02, 5.1480e+02, 4.3940e+02, 3.7590e+02, 
         3.2150e+02, 2.7480e+02, 2.3500e+02, 2.0090e+02,
         1.7170e+02, 1.4690e+02, 1.2510e+02, 1.0740e+02,
         9.1760e+01, 7.8180e+01, 6.7150e+01, 5.7450e+01, 
         4.9000e+01, 4.1810e+01, 3.5840e+01, 3.0490e+01,
         2.6340e+01, 2.2210e+01, 1.9260e+01, 1.6330e+01,
         1.3980e+01, 1.1640e+01, 9.8900e+00, 8.7200e+00,
         7.5600e+00, 6.3900e+00, 5.2300e+00, 4.6400e+00, 
         4.0600e+00, 3.4800e+00, 2.9000e+00, 2.3200e+00, 
         1.7400e+00, 1.1600e+00, 5.8000e-01],
        [2.3971e+04, 1.7506e+04, 1.2785e+04, 9.3458e+03,
         6.8294e+03, 4.9894e+03, 3.6450e+03, 2.6626e+03,
         1.9454e+03, 1.4219e+03, 1.0390e+03, 7.5904e+02,
         5.5458e+02, 4.0494e+02, 2.9615e+02, 2.1649e+02,
         1.5824e+02, 1.1549e+02, 8.4391e+01, 6.1886e+01,
         4.5098e+01, 3.2937e+01, 2.4099e+01, 1.7670e+01,
         1.2710e+01, 9.2529e+00, 6.9252e+00, 4.9083e+00,
         3.7452e+00, 2.5844e+00, 1.4155e+00, 5.7999e-01],
    ]

    # Now, file-level properties.
    file_id = index
    max_bins = 63
    pimsfile = PIMSFile(file_id=file_id, max_bins=max_bins, 
                        mode_bin_definitions=mode_bin_definitions, 
                        observations=observations_converted)

    # We also record metadata.
    # We use raw (not interpolated) CAPS data here, so some properties, like
    # 'blur_sigma' and 'bin_selection', are all defaults.
    metadata = {
        'data_file': os.path.abspath(els_data_file),
        'quantity': quantity,
        'TIMESTAMP_START': TIMESTAMP_START,
        'blur_sigma': 0,
        'bin_selection': 'all',
        'filter': 'no_filter',
        'filter_size': 0,
    }
    return pimsfile, metadata

# Returns a name for the output file corresponding to an input file.
def libeos_name(file_name):
    return os.path.splitext(os.path.basename(file_name))[0] + '.pim'

# Returns a name for the metadata file corresponding to an input file.
def metadata_name(file_name):
    return os.path.splitext(os.path.basename(file_name))[0] + '.meta'

def main(input, output):

    # Multiplex depending on whether 'input' and 'output' are files or directories.
    if os.path.isdir(input):
        input_files = [input + '/' + input_f for input_f in os.listdir(input)]
        if os.path.exists(output):
            if not os.path.isdir(output):
                raise ValueError('Output not a directory.')
        else:
            # If 'output' doesn't exist, we will create a directory with the same name.
            Path(output).mkdir(parents=True)
        
        output_files = [output + '/' + libeos_name(input) for input in input_files]
        metadata_files = [output + '/' + metadata_name(input) for input in input_files]

    else:
        input_files = [input]
        if os.path.isdir(output):
            output_files = [output + '/' + libeos_name(input)]
            metadata_files = [output + '/' + metadata_name(input)]
        else:
            output_files = [output]
            metadata_files = [os.path.splitext(output) + '.meta']
    
    # Loop over each pair.
    for index, (input_f, output_f, metadata_f) in enumerate(zip(input_files, output_files, metadata_files)):
        if '.DAT' not in input_f:
            print 'Ignoring file %s as it is not a ELS .DAT file.' % input_f
            continue

        pimsfile, metadata = convert(index=index, els_data_file=input_f)
        with open(output_f, 'wb') as f:
            f.write(pimsfile.tobytes())
        with open(metadata_f, 'w') as f:
            yaml.dump(metadata, f)

if __name__ == '__main__':

    import argparse
    parser = argparse.ArgumentParser(argument_default=argparse.SUPPRESS)

    parser.add_argument('input', help='Input ELS file (or a directory containing ELS files).')
    parser.add_argument('output', help='Output .pim file (or a directory which will be filled with .pim files).')

    args = parser.parse_args()
    main(**vars(args))
