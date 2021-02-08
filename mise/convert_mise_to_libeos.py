#!/usr/bin/env python
"""
Convert HDF5 MISE DN data to libeos format
"""

import os
import numpy as np

from mise_data import MISEData
from libeos_MISE import MISE, MISE_FILE_EXTENSION

MISE_N_BANDS = 421

def main(inputfile, outputfile):
    if outputfile is None:
        outputfile = os.path.splitext(inputfile)[0] + MISE_FILE_EXTENSION

    cube = MISEData(inputfile)

    assert cube.data.dtype == np.uint16

    # Trim off the last set of bands so we're left with the expected number
    trimmed_data = cube.data[..., :MISE_N_BANDS]

    mise = MISE(0, 0, trimmed_data)
    mise.save(outputfile)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(argument_default=argparse.SUPPRESS)

    parser.add_argument('inputfile')
    parser.add_argument('-o', '--outputfile', default=None)

    args = parser.parse_args()
    main(**vars(args))
