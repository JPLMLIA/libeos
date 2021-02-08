#!/usr/bin/env python

"""
Convert PIMS data (.npy files) to LIBEOS binary files.
"""

import os
import numpy as np
from glob import glob
from builtins import chr
from progressbar import ProgressBar, ETA, Bar

PIMS_FILE_EXTENSION = '.pim'

# An observation, across all bins.
class PIMSObs(object):

    def __init__(self, obs_id, timestamp, mode, counts, num_bins=None):

        # Quick-checks!
        assert obs_id >= 0

        # Restrict to num_bins if passed.
        if num_bins is not None:
            counts = counts[:num_bins]

        # Accumulate.
        self.num_bins = len(counts)
        self.header = np.array([
            obs_id,
            timestamp,
            self.num_bins,
            mode,
        ])
        self.counts = np.array(counts)
        
        # Set NaNs as zero.
        self.counts[np.isnan(self.counts)] = 0

        # Convert to appropriate data-type.
        self.header = self.header.astype('>u4')
        self.counts = self.counts.astype('>u4')

        self._bytes = None

    def tobytes(self, max_bins=None):
        if self._bytes is None:
            # Add observation header.
            self._bytes = self.header.tobytes()

            # Add the counts.
            counts = self.counts

            # Pad upto max_bins, if supplied.
            if max_bins is not None:
                padded_counts = np.zeros(max_bins, dtype='>u4')
                padded_counts[:counts.size] = counts
                counts = padded_counts
                assert len(counts) == max_bins

            self._bytes += counts.tobytes()

        return self._bytes


# A bunch of observations, in a file.
class PIMSFile(object):

    HEADER = b'EOS_PIMS'
    PAD_VALUE = 0xFF
    ALIGNMENT = 4
    VERSION = 1

    def __init__(self, file_id, max_bins, mode_bin_definitions, observations):

        # Quick-checks!
        assert file_id >= 0
        assert max_bins >= 0

        # Pad 'mode_bin_definitions' according to max_bins.
        for index, defs in enumerate(mode_bin_definitions):
            mode_bin_definitions[index] = np.pad(defs, (0, max_bins - len(defs)), 'constant', constant_values=np.inf)

        # Maintain references.
        self.mode_bin_definitions = mode_bin_definitions
        self.observations = observations
        num_modes = len(mode_bin_definitions)
        num_obs = len(observations)
        self.max_bins = max_bins
        self.header = [
            file_id,
            num_modes,
            max_bins,
            num_obs,
        ]
    
        # Convert to appropriate data-type.
        self.header = np.array(self.header, dtype='>u4')
        self.mode_bin_definitions = np.array(self.mode_bin_definitions, dtype='>f4')

        self._bytes = None

    def padded_header_string(self):
        # Header string.
        hstr = PIMSFile.HEADER

        # Determine how many padding bytes we need to be byte-aligned.
        padding_bytes = (
            PIMSFile.ALIGNMENT - ((len(PIMSFile.HEADER) + 1) % PIMSFile.ALIGNMENT)
        ) % PIMSFile.ALIGNMENT
        if padding_bytes == 0:
            padding_bytes = PIMSFile.ALIGNMENT

        # Add padding bytes.
        for _ in range(padding_bytes):
            hstr += chr(PIMSFile.PAD_VALUE).encode('latin-1')
        return hstr

    def version_byte(self):
        return chr(PIMSFile.VERSION).encode('latin-1')

    def tobytes(self):
        if self._bytes is None:
            # Add first row, except version.
            self._bytes = self.padded_header_string()

            # Add version byte.
            self._bytes += self.version_byte()

            # Add file header.
            self._bytes += self.header.tobytes()

            # Add mode bin definitions.
            self._bytes += self.mode_bin_definitions.tobytes()

            # Add each observation.
            for obs in self.observations:
                self._bytes += obs.tobytes(max_bins=self.max_bins)

        return self._bytes


def main(inputdir, outputdir):

    def get_outputfile(inputfile):
        base = os.path.splitext(os.path.basename(inputfile))[0]
        return os.path.join(outputdir, base + PIMS_FILE_EXTENSION)

    def convert(inputfile, outputfile):
        data = np.load(inputfile)
        pims = PIMSFile(**data)
        with open(outputfile, 'wb') as f:
            f.write(pims.tobytes())

    if os.path.isdir(inputdir):
        if not os.path.isdir(outputdir):
            raise ValueError(
                'If inputdir is a directory, but outputdir is a file'
            )

        inputfiles = glob(os.path.join(inputdir, '*.npz'))
        progress = ProgressBar(widgets=['Converting: ', Bar('='), ETA()])
        for inputfile in progress(inputfiles):
            outputfile = get_outputfile(inputfile)
            convert(inputfile, outputfile)

    else:
        # `inputdir` is actually a file
        inputfile = inputdir
        if os.path.isdir(outputdir):
            outputfile = get_outputfile(inputdir)
        else:
            # `outputdir` is actually a file
            outputfile = outputdir
        convert(inputfile, outputfile)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(argument_default=argparse.SUPPRESS)

    parser.add_argument('inputdir')
    parser.add_argument('outputdir')

    args = parser.parse_args()
    main(**vars(args))
