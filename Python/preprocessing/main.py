# -*- coding: utf-8 -*-
"""
The entry of the preprocessing program
"""
import argparse

from preproc_app import PreprocAPP

if __name__ == '__main__':
    # parse arguments
    PARSE = argparse.ArgumentParser()
    PARSE.add_argument(\
            '-c', '--config_file', \
            help='the YAML configuration file of preprocessing', \
            required=True)
    ARGS = PARSE.parse_args()

    # start preprocessing
    APP = PreprocAPP(vars(ARGS))
    APP.run()
