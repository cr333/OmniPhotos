import unittest
import argparse
import sys
import json
import os

# append the preprocessing directory to the path. this is necessary as unittests "needs" its own mainfile and if this
# isn't in the root directory the imports don't work.

path = os.path.split(os.path.dirname(__file__))[0]
sys.path.append(path)

from itests import *
from utests import *

def update_paths(path):
    save_dir = os.path.split(__file__)[0]
    save_dir = os.path.join(save_dir,"paths.json")
    with open(save_dir, "r") as ifile:
        dict = json.load(ifile)
        dict["test_data_root_dir"] = path

    with open(save_dir, "w") as ofile:
        json.dump(dict, ofile)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--dir',
                        required=True,
                        help="the test dataset directory")
    # necessary work around to stop unittest from complaining
    parser.add_argument('unittest_args', nargs='*')

    args = parser.parse_args()
    update_paths(args.dir)
    # Now set the sys.argv to the unittest_args (leaving sys.argv[0] alone)
    sys.argv[1:] = args.unittest_args
    unittest.main()
