# -*- coding: utf-8 -*-

import unittest
import PIL
import numpy as np
import pathlib

# add parent directory to path
import os
import sys
import inspect

current_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parent_dir = os.path.dirname(current_dir)
sys.path.insert(0, parent_dir)

import preprocessor
from colmap import ScriptingColmap


class TestPreprocessor(unittest.TestCase):

    def setUp(self):
        # initial runtime environment
        args = {"config_file": "../config_omniphotos.yaml"}
        self.preprocessor = preprocessor.Preprocessor(args)

        self.preprocessor.root_dir = \
            pathlib.Path("D:/workdata/testDatasets/circular/KyotoShrines_test")
        self.preprocessor.image_output_path = \
            pathlib.Path("D:/workdata/testDatasets/circular/KyotoShrines_test/Input")
        self.preprocessor.FPS = 50
        self.preprocessor.omniphotos_config_template_path = \
            "D:/workspace/Python/preprocessing/template/config.yaml.template"

    def test_model_converter(self):
        print("test_model_convertor")
        root_dir = "D:/workdata/testDatasets/circular/KyotoShrines/Capture-backup/COLMAP/"
        colmap_bin = "D:/utilities/COLMAP/COLMAP-3.6-dev.3-windows/COLMAP.bat"

        full_model_ba_dir = os.path.join(root_dir, 'full-model-ba/')
        full_model_ba_txt_dir = os.path.join(root_dir, 'full-model-ba-txt/')
        ScriptingColmap.model_converter(colmap_bin, full_model_ba_dir, full_model_ba_txt_dir)

    def test_image_rotate(self):
        print("test_image_rotate")
        image_file_path = "D:/workspace/data/original-0087.png"
        image_data = np.array(PIL.Image.open(image_file_path))
        self.preprocessor.input_data = np.expand_dims(image_data, axis=0)
        self.preprocessor.config["preprocessing.image_rotation"] = [0.0, 90.0]
        self.preprocessor.rotate_image()
        self.preprocessor.show_image(self.preprocessor.input_data[0])

    def test_file_structure_trim(self):
        print("test_file_structure_trim")
        self.preprocessor.file_structure_trim()


if __name__ == '__main__':
    unittest.main()
