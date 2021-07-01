import unittest
import circleselector
import numpy as np
import os
class TestCircleFitting(unittest.TestCase):
    def setUp(self) -> None:
        self.ovslam_csv = "C:\\Setup\\python_projects\\CircleFitting\\data\\BathAbbey1\\openvslam_result\\BathAbbey1_traj.csv"
        self.npy_dataset = "C:\\Setup\\python_projects\\CircleFitting\\results\\metrics2.0\\BathAbbey1.npy"
        self.npy_result = "C:\\Setup\\python_projects\\CircleFitting\\results\\metrics2.0\\BathAbbey1_intervals.npy"
        self.npy_result_cv = "C:\\Setup\\python_projects\\CircleFitting\\results\\metrics2.0\\BathAbbey1_intervals_cv.npy"

    def test_loader(self):
        if not os.path.exists(self.ovslam_csv):
            self.skipTest(self.npy_dataset + " npy dataset not found")
        self.assertIsNotNone(circleselector.loader.load_file(self.ovslam_csv))

    def test_calc(self):
        points = circleselector.loader.load_file(self.ovslam_csv)[:100]
        self.assertIsNotNone(circleselector.metrics.calc(points,
                                                         errors=["endpoint_error", "flatness_error", "perimeter_error"]))

    def test_sort(self):
        if not os.path.exists(self.npy_dataset):
            self.skipTest(self.npy_dataset + " npy dataset not found")
        if not os.path.exists(self.npy_result):
            self.skipTest(self.npy_dataset + " npy result not found")
        testres = circleselector.datatypes.PointDict(np.load(self.npy_dataset, allow_pickle=True).tolist()).find_local_minima(811)
        compres = circleselector.datatypes.PointDict(np.load(self.npy_result, allow_pickle=True).tolist())
        self.assertEqual(testres,compres)

    def test_cv(self):
        points = circleselector.loader.load_file(self.ovslam_csv)
        intervals = circleselector.datatypes.PointDict(np.load(self.npy_result, allow_pickle=True).tolist())
        if not os.path.exists(self.npy_dataset):
            self.skipTest(self.npy_result_cv + " npy dataset not found")
        old_intervals = circleselector.datatypes.PointDict(np.load(self.npy_result_cv, allow_pickle=True).tolist())
        intervals = circleselector.metrics.calc(points, intervals, errors=["ssim", "psnr"], dataset_path="C:\\Setup\\python_projects\\CircleFitting\\data\\BathAbbey1\\")
        ssim_diff = abs(np.array(intervals.get("ssim")) - np.array(old_intervals.get("ssim")))
        psnr_diff = abs(np.array(intervals.get("psnr")) - np.array(old_intervals.get("psnr")))
        self.assertTrue(np.sum(ssim_diff) + np.sum(psnr_diff) < 0.0001)

if __name__ == '__main__':
    unittest.main()
