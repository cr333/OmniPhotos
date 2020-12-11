# -*- coding: utf-8 -*-

import unittest

# add parent directory to path
import os,sys,inspect
current_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parent_dir = os.path.dirname(current_dir)
sys.path.insert(0, parent_dir)

from colmap import ScriptingColmap

class TestColmap(unittest.TestCase):

    def test_model_converter(self):
        root_dir = "D:/workdata/testDatasets/circular/KyotoShrines/Capture-backup/COLMAP/"
        colmap_bin = "D:/utilities/COLMAP/COLMAP-3.6-dev.3-windows/COLMAP.bat"

        full_model_ba_dir = os.path.join(root_dir, 'full-model-ba/')
        full_model_ba_txt_dir = os.path.join(root_dir, 'full-model-ba-txt/')
        ScriptingColmap.model_converter(colmap_bin, full_model_ba_dir, full_model_ba_txt_dir)

if __name__ == '__main__':
    unittest.main()
