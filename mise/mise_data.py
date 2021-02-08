#!/usr/bin/env python
# mise_data.py: Read in and make available MISE data (from matlab cube)
# 
# Kiri Wagstaff, Nov. 21, 2017
import os
import numpy as np
import h5py

class MISEData(object):

    def __init__(self, filename):
        self.data_file = filename

        self._load()

    def _load(self):
        # Matlab files can be read if saved with '-v7.3' option from Matlab
        # Must use HDF5 reader:
        misefile = h5py.File(self.data_file, 'r')

        cube = misefile['cube']
        cube = np.array(cube)

        # APL file is wavelengths x height x width
        # Our generated data is height x width x wavelengths
        # Assume all images have height = width.
        # To handle either case, check size of first two dimensions.
        # If not equal, assume transpose is needed.
        if cube.shape[0] != cube.shape[1]:
            print('Warning: cube.shape[0] != cube.shape[1]; assuming transpose needed.')
            self.data = cube.T
        else:
            self.data = cube

        # If wavelengths are specified, use them
        if 'wavelengths' in misefile.keys():
            self.wavelengths = np.array(misefile['wavelengths']).T
        else:
            self.wavelengths = np.arange(800.0,5001.0,10.0);  # Per Diana
        

