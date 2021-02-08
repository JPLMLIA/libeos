#!/usr/bin/env python
import os
import numpy as np

from libeos_convertor import PIMSFile, PIMSObs

def main(outputfile, num_observations=2, num_bins=67):

    obs_ids = np.arange(num_observations)
    timestamps = np.arange(2*num_observations, step=2)
    all_counts = 10000 * np.random.random_sample((num_observations, num_bins))
    modes = np.zeros(num_observations)

    pims_obs = [
        PIMSObs(obs_id=obs_id, timestamp=timestamp, mode=mode, counts=counts)
        for obs_id, timestamp, mode, counts in
        zip(obs_ids, timestamps, modes, all_counts)
    ]

    mode_bin_definitions = np.tile(np.linspace(1, 1000, num=num_bins), (4, 1))

    pimsfile = PIMSFile(file_id=123,
                        max_bins=num_bins,
                        mode_bin_definitions=mode_bin_definitions,
                        observations=pims_obs)

    with open(outputfile, 'wb') as f:
        f.write(pimsfile.tobytes())

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(argument_default=argparse.SUPPRESS)

    parser.add_argument('outputfile')
    parser.add_argument('-n', dest='num_observations', type=int, default=4)

    args = parser.parse_args()
    main(**vars(args))
