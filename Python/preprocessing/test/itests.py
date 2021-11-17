import unittest
import circleselector
from circleselector.datatypes import PointDict
import numpy as np
import os
import json

class TestCircleSelector(unittest.TestCase):
    def setUp(self) -> None:
        self.currdir = os.path.dirname(__file__)
        with open(os.path.join(self.currdir,"paths.json"),"r") as ifile:
            dct = json.load(ifile)

        self.ovslam_csv = os.path.join(dct["test_data_root_dir"],
                                       dct["relative_path_frame_trajectory_csv"])
        self.best_intervals_json = os.path.join(dct["test_data_root_dir"],
                                                dct["relative_path_best_intervals_test"])
        self.data_json = os.path.join(self.currdir,"data.json")

    def test_loader(self):
        if not os.path.exists(self.ovslam_csv):
            self.skipTest(self.ovslam_csv + " not found")
        camera_centers = circleselector.loader.load_file(self.ovslam_csv)
        self.assertTrue(len(camera_centers)>0)



    def test_calc(self):
        if not os.path.exists(self.ovslam_csv):
            self.skipTest(self.ovslam_csv + " not found")
        points = circleselector.loader.load_file(self.ovslam_csv)
        data = circleselector.metrics.calc(points,
                                           errors=["endpoint_error", "flatness_error",
                                                   "perimeter_error"])
        # save data for a later test
        data.toJSON(self.data_json)
        self.assertIsNotNone(circleselector.metrics.calc(points,
                                                         errors=["endpoint_error", "flatness_error",
                                                                 "perimeter_error","pairwise_distribution"]))

    def test_sort(self):
        if not os.path.exists(self.best_intervals_json):
            self.skipTest(self.best_intervals_json + " not found.")
        if not os.path.exists(self.data_json):
            self.skipTest(self.data_json + " not found.")
        data = PointDict(from_file=self.data_json)
        test = data.find_local_minima().get("interval")
        comp = PointDict(from_file=self.best_intervals_json).get("interval")

        self.assertEqual(comp,test)




    # def test_cv(self):
    #     if not os.path.exists(self.best_intervals_json):
    #         self.skipTest(self.best_intervals_json + " not found.")
    #     if not os.path.exists(self.data_json):
    #         self.skipTest(self.data_json + " not found.")
    #
    #     data = PointDict(from_file=self.data_json)
    #     points = circleselector.loader.load_file(self.ovslam_csv)
    #     test = circleselector.metrics.calc(points,data.find_local_minima(),errors=["ssim","psnr"])

if __name__ == '__main__':
    unittest.main()
