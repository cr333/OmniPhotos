import unittest
import circleselector
from preproc_app import PreprocAPP
import os
import json

class TestCircleSelector(unittest.TestCase):
    def setUp(self) -> None:
        self.curr_dir = os.path.dirname(__file__)
        with open(os.path.join(self.curr_dir, "paths.json"), "r") as ifile:
            dct = json.load(ifile)
        self.test_data_root_dir = os.environ["OP_TEST_ROOT_DIR"].replace('"','')

        self.ovslam_csv = os.path.join(self.test_data_root_dir,
                                       dct["relative_path_frame_trajectory_csv"])
        self.best_intervals_json = os.path.join(self.test_data_root_dir,
                                                dct["relative_path_best_intervals_test"])
        self.data_json = os.path.join(self.curr_dir, "data.json")

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
                                           errors=["endpoint_error",
                                                   "flatness_error",
                                                   "perimeter_error"])
        # save data for a later test
        data.toJSON(self.data_json)
        self.assertIsNotNone(circleselector.metrics.calc(points,
                                                         errors=["endpoint_error",
                                                                 "flatness_error",
                                                                 "perimeter_error",
                                                                 "pairwise_distribution"]))

if __name__ == '__main__':
    unittest.main()
