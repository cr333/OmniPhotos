# -*- coding: utf-8 -*-
"""
The main entry point of the OmniPhotos dataset preprocessing pipeline.
"""
import argparse

from preproc_app import PreprocAPP

if __name__ == '__main__':
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
