#!/usr/bin/env python
# Define a Python class to represent MISE data and save it in libeos format
# Kiri Wagstaff, 3/24/20

import os
import numpy as np
from glob import glob
from builtins import chr
from progressbar import ProgressBar, ETA, Bar

MISE_FILE_EXTENSION = '.mis'

class MISE(object):

    HEADER = b'EOS_MISE'
    PAD_VALUE = 0xFF
    ALIGNMENT = 4
    VERSION = 1

    # Data is rows x cols x 451 bands, to be stored as uint16 (DNs)
    def __init__(self, observation_id, timestamp, data):
        bands = range(0, data.shape[2])
        for band in bands:
            assert np.max(data[:,:,band]) <= (2**16 - 1)
            assert np.min(data[:,:,band]) >= 0

        assert observation_id >= 0
        assert timestamp >= 0

        self.observation_id = observation_id
        self.timestamp = timestamp
        self.data = data.astype(np.uint16)
        #print(self.data.tobytes())
        self._bytes = None

    def padded_header_string(self):
        # Header string
        hstr = MISE.HEADER

        # Determine how many padding bytes we need to be byte-aligned
        padding_bytes = (
            MISE.ALIGNMENT - ((len(MISE.HEADER) + 1) % MISE.ALIGNMENT)
        ) % MISE.ALIGNMENT
        if padding_bytes == 0:
            padding_bytes = MISE.ALIGNMENT

        # Add padding bytes
        for p in range(padding_bytes):
            hstr += chr(MISE.PAD_VALUE).encode('latin-1')

        return hstr

    def header_values(self):
        # Construct array of header values
        header_values = [
            self.observation_id,
            self.timestamp,
        ]
        header_values.append(self.data.shape[1]) # cols
        header_values.append(self.data.shape[0]) # rows
        header_values.append(self.data.shape[2]) # bands

        header = np.array(header_values, dtype='>u4')
        
        return header

    def tobytes(self):
        if self._bytes is None:
            self._bytes = self.padded_header_string()

            # Add version byte
            self._bytes += chr(MISE.VERSION).encode('latin-1')

            header = self.header_values()
            self._bytes += header.tobytes()

            # Construct array of data values in BIP (row,col,band) format
            #data = np.transpose(self.data, axes=[1,2,0]).astype('>u2')
            data = self.data.astype('>u2')
            #print data
            self._bytes += data.tobytes()

        return self._bytes

    def save(self, outputfile):
        with open(outputfile, 'wb') as f:
            f.write(self.tobytes())
