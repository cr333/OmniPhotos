import math
import multiprocessing as mp
import os
import time

import numpy as np
from mathutils import Quaternion

import circleselector.cv_utils as cv_utils
from .datatypes import PointDict, CameraCenters


class Metrics(object):
    """
    class that can :
        - run on prexisting point_dcts list
        - select what errors to calculate
        - calculate all errors in parallel (if required)
    """

    def __init__(self, points=None, point_dcts=None, errors=None, verbose=False, dataset_path=None):
        self.points = points
        self.point_dcts = None
        self.errors = ["endpoint_error",
                       "flatness_error",
                       "perimeter_error",
                       "pairwise_distribution",
                       "ssim",
                       "psnr"]
        if point_dcts is not None:
            self.point_dcts = point_dcts
        if errors is not None:
            self.errors = errors

        self.verbose = verbose
        self.dataset_path = dataset_path
        self.rel_input_image_path = 'Input'
        self.__save_of__ = False

    def run_on_interval(self, interval: tuple) -> dict:

        if self.points is None:
            raise ValueError("need to initialise points before running calculations.")

        point_dict = {"interval": interval}
        sub_arr = np.array(self.points[interval[0]:interval[1]])
        centroid = self.calculate_centroid(sub_arr)
        if "flatness_error" in self.errors:
            points_centered = sub_arr - centroid
            u, sigma, _ = np.linalg.svd(points_centered.T)
            error = min(sigma)
            point_dict["flatness_error"] = error

        std = None
        path_length = None

        if "perimeter_error" in self.errors:
            radius = sum([np.linalg.norm(point - centroid) for point in sub_arr]) / len(sub_arr)
            exp_perimeter = 2 * np.pi * radius
            path_length, std = self.find_path_length(interval)

            point_dict["perimeter_error"] = abs(1 - exp_perimeter / path_length)

        if "pairwise_distribution" in self.errors:
            if std is None:
                _, std = self.find_path_length(interval)

            point_dict["pairwise_distribution"] = std

        if "endpoint_error" in self.errors:
            n = len(sub_arr)

            if path_length is None:
                path_length = self.find_path_length(interval)[0]

            endpoint_gap = np.linalg.norm(sub_arr[0] - sub_arr[-1])
            point_dict["endpoint_error"] = endpoint_gap / path_length

        psnr = None
        savedir = None
        lookatang = None
        if self.__save_of__:
            savedir = os.path.join(self.dataset_path, "CircleFittingResults", "interval_" + str(interval))
            os.makedirs(savedir)
        if "ssim" in self.errors:
            lookatang = self.calculate_angle(centroid, interval)
            ssim, psnr = cv_utils.calculate_metrics(interval, self.dataset_path, savedir, self.rel_input_image_path,
                                                    lookatang)
            point_dict["ssim"] = ssim

        if "psnr" in self.errors:
            if lookatang is None:
                lookatang = self.calculate_angle(centroid, interval)
            if psnr is None:
                _, psnr = cv_utils.calculate_metrics(interval, self.dataset_path, savedir, self.rel_input_image_path,
                                                     lookatang)
            point_dict["psnr"] = psnr

        return point_dict

    def run_on_pair_lst(self, point_dicts) -> PointDict:
        if point_dicts is None and self.point_dcts is None:
            self.init_data()
            point_dicts = self.point_dcts
        for enum, dct in enumerate(point_dicts):
            dct.update(self.run_on_interval(dct["interval"]))
        return point_dicts

    def create_pairs(self):
        n = len(self.points)
        pairs = []
        for i in range(n):
            for j in range(i + 2, n):
                if 10 > abs(i - j):
                    continue
                pairs.append((i, j))
        return pairs

    def init_data(self):
        if self.points is None:
            raise ValueError("need to initialise points before running calculations.")

        if self.point_dcts is None or len(self.point_dcts) == 0:
            pairs = self.create_pairs()
            self.point_dcts = [dict(interval=pair) for pair in pairs]

    def run(self):
        if self.__save_of__:
            path = os.path.join(self.dataset_path, "CircleFittingResults")
            if not os.path.exists(path):
                os.makedirs(path)
        self.init_data()

        interval = len(self.point_dcts) // mp.cpu_count()
        inmax = 0
        pair_subdiv = []
        while inmax < len(self.point_dcts):
            pair_subdiv.append(self.point_dcts[inmax:min(inmax + interval, len(self.point_dcts))])
            inmax += interval
        pool = mp.Pool(mp.cpu_count())
        if self.verbose:
            print("time started", time.ctime(time.time()))
        pd_lst = []
        for enum, sub_lst in enumerate(pair_subdiv):
            pd_lst.append(pool.apply_async(self.run_on_pair_lst, args=([sub_lst])))

        pool.close()
        pool.join()
        if self.verbose:
            print("time ended", time.ctime(time.time()))

        point_dicts = []
        for lst in pd_lst:
            point_dicts.extend(lst.get())
        self.point_dcts = point_dicts

    def find_path_length(self, interval: tuple) -> (float, float):
        """

        :param interval: tuple, interval on self.points
        :return: float,float
        """

        norms = []
        points = self.points[interval[0]:interval[1]]
        for idx in range(len(points) - 1):
            norms.append(np.linalg.norm(points[idx] - points[idx + 1]))

        # wrap around
        norms.append(np.linalg.norm(points[0] - points[-1]))

        return np.sum(norms), np.std(norms)

    def calculate_angle(self, centroid: np.array, interval: tuple) -> float:
        angs = []
        orientations = self.points.orientations
        for idx in interval:
            c_prime = self.points[idx] - centroid
            transform = Quaternion(orientations[idx]).to_matrix()
            proj_c = np.dot(transform, c_prime)
            ang = math.atan2(proj_c[0], proj_c[2])
            angs.append(ang)
        return np.mean(angs)

    def calculate_centroid(self, arr: np.array):
        return np.mean(arr, axis=0)

    def __str__(self):
        parameters = []
        parameters.append("errors:         " + str(self.errors))
        parameters.append("dataset_path:   " + str(self.dataset_path))
        parameters.append("len point dict: " + str(len(self.point_dcts)))
        parameters.append("len points:     " + str(len(self.points)))
        return "\n".join(parameters)


def decorator(Metrics):
    def calc(points: CameraCenters,
             point_dcts=None,
             errors=None,
             verbose=False,
             dataset_path=None,
             rel_input_image_path='Input',
             mp=True,
             interval=None,
             save_of=False) -> [dict]:
        """
        Calculates the metrics on the given points. By default will perform all metrics on all the possible intervals
        in the camera path provided. WARNING: this will take a long time.

        :param points: array like. ~1K camera centers in a camera path. NOTE: ssim and psnr calculations will fail
        if camera orientations not provided. (type CameraCenters from loader.load)
        :param point_dcts: listof(dict). If this parameter is passed the
        metrics will be calculated on only the intervals present in this dict.
        :param errors: list. names of metrics that should be calculated.
        :param verbose:
        :param dataset_path: path to rootdir of dataset. eg. path/to/GenoaCathedral
        :param mp: calculated in parallel (using threads) or not
        :param rel_input_image_path: path to images for cv relative to dataset_path
        :param interval: overrides all other behaviour and will calculate on given interval of points.
        :param: save_of: will save the Optical Flow results to dataset_path/CircleFittingResults
        :return: PointDict
        """

        metrics = Metrics(points, point_dcts, errors, verbose, dataset_path)
        metrics.rel_input_image_path = rel_input_image_path
        if save_of and os.path.exists(str(dataset_path)):
            metrics.__save_of__ = True
        for error in ["ssim", "psnr"]:
            if error in metrics.errors and metrics.dataset_path is None:
                raise Exception(error + " being calculated but dataset_path was not set.")

        if interval is not None:
            return [metrics.run_on_interval(interval)]
        if not mp:
            return PointDict(metrics.run_on_pair_lst(point_dcts))
        metrics.run()
        if verbose:
            print(metrics)
        return PointDict(metrics.point_dcts)

    return calc


calc = decorator(Metrics)
