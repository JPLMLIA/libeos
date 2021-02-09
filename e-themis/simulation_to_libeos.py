#!/usr/bin/env python
"""
Convert simulation trial to LIBEOS format
"""
import os
import numpy as np
from glob import glob
from builtins import chr
from progressbar import ProgressBar, ETA, Bar

ETHEMIS_FILE_EXTENSION = '.etm'

class ETM(object):

    HEADER = b'EOS_ETHEMIS'
    PAD_VALUE = 0xFF
    ALIGNMENT = 4
    VERSION = 1

    def __init__(self, observation_id, timestamp, band1, band2, band3):
        for band in (band1, band2, band3):
            assert np.max(band) <= (2**16 - 1)
            assert np.min(band) >= 0

        assert observation_id >= 0
        assert timestamp >= 0

        self.observation_id = observation_id
        self.timestamp = timestamp
        self.bands = (
            band1.astype(np.uint16),
            band2.astype(np.uint16),
            band3.astype(np.uint16),
        )
        self._bytes = None

    def padded_header_string(self):
        # Header string
        hstr = ETM.HEADER

        # Determine how many padding bytes we need to be byte-aligned
        padding_bytes = (
            ETM.ALIGNMENT - ((len(ETM.HEADER) + 1) % ETM.ALIGNMENT)
        ) % ETM.ALIGNMENT
        if padding_bytes == 0:
            padding_bytes = ETM.ALIGNMENT

        # Add padding bytes
        for p in range(padding_bytes):
            hstr += chr(ETM.PAD_VALUE).encode('latin-1')

        return hstr

    def header_values(self):
        # Construct array of header values
        header_values = [
            self.observation_id,
            self.timestamp,
        ]
        for band in self.bands:
            header_values.append(band.shape[1]) # cols
            header_values.append(band.shape[0]) # rows

        header = np.array(header_values, dtype='>u4')
        
        return header

    def tobytes(self):
        if self._bytes is None:
            self._bytes = self.padded_header_string()

            # Add version byte
            self._bytes += chr(ETM.VERSION).encode('latin-1')

            header = self.header_values()
            self._bytes += header.tobytes()

            # Construct array of data values
            data = np.hstack([
                np.ravel(band, order='C')
                for band in self.bands
            ]).astype('>u2')
            self._bytes += data.tobytes()

        return self._bytes

def get_outputfile(outputdir, resultfile):
    base = os.path.splitext(os.path.basename(resultfile))[0]
    return os.path.join(outputdir, base + ETHEMIS_FILE_EXTENSION)

def convert(inputfile, outputfile):
    data = np.load(inputfile)

    etm = ETM(
        data['trial'], data['event_time'],
        data['band1_dn'], data['band2_dn'], data['band3_dn']
    )

    with open(outputfile, 'wb') as f:
        f.write(etm.tobytes())

def main(resultdir, outputdir):
    if os.path.isdir(resultdir):
        if not os.path.isdir(outputdir):
            raise ValueError(
                'If resultdir is a directory, but outputdir is a file'
            )

        resultsfiles = glob(os.path.join(resultdir, '*.npz'))
        progress = ProgressBar(widgets=['Converting: ', Bar('='), ETA()])
        for resultfile in progress(resultsfiles):
            outputfile = get_outputfile(outputdir, resultfile)
            convert(resultfile, outputfile)

    else:
        # `resultdir` is actually a file
        resultfile = resultdir
        if os.path.isdir(outputdir):
            outputfile = get_outputfile(outputdir, resultdir)
        else:
            # `outputdir` is actually a file
            outputfile = outputdir
        convert(resultfile, outputfile)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(argument_default=argparse.SUPPRESS)

    parser.add_argument('resultdir')
    parser.add_argument('outputdir')

    args = parser.parse_args()
    main(**vars(args))
