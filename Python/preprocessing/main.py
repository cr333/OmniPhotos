# -*- coding: utf-8 -*-
"""
The main entry point of the OmniPhotos dataset preprocessing pipeline.
"""
import argparse
import multiprocessing as mp
from preproc_app import PreprocAPP

if __name__ == '__main__':
    mp.freeze_support()
    # parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-c', '--config_file',
        help='YAML configuration file for the preprocessing pipeline',
        required=True)
    args = parser.parse_args()

    # start preprocessing
    app = PreprocAPP(vars(args))
    app.run()
