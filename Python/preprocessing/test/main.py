import unittest
import sys
import os

path = os.path.split(os.path.dirname(__file__))[0]
sys.path.append(path)

from preproc_app import PreprocAPP

# append the preprocessing directory to the path. this is necessary as unittests "needs" its own mainfile and if this
# isn't in the root directory the imports don't work.


from itests import *
from utests import *

if __name__ == '__main__':
    unittest.main()
