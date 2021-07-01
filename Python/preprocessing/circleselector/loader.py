from .datatypes import CameraCenters
from decimal import Decimal
import numpy as np

video_fps = 50
coordinate_scalar = 1

def load_file(path) -> CameraCenters:
    """
    mingze's code

    :param path: path to list
    :return: list of 3d points (np.arrays)
    """
    xx = []
    yy = []
    zz = []
    quats = []

    f = open(path)
    frame_tra_all = f.read().splitlines()
    current_frame_idx = -1

    for frame_tra in frame_tra_all:

        frame_tra_list = frame_tra.split(' ')

        current_frame_idx = int(
            float(frame_tra_list[0]) * video_fps + 0.5)

        # if current_frame_idx % 30 == 0:
        #     print("add camera index {} / {}".format(current_frame_idx,
        #                                             len(frame_tra_all)))

        quats.append((Decimal(frame_tra_list[7]),
                                       Decimal(frame_tra_list[4]),
                                       Decimal(frame_tra_list[5]),
                                       Decimal(frame_tra_list[6])))


        location_x = float(
            frame_tra_list[1]) * coordinate_scalar
        location_y = float(
            frame_tra_list[2]) * coordinate_scalar
        location_z = float(
            frame_tra_list[3]) * coordinate_scalar
        xx.append(location_x)
        yy.append(location_y)
        zz.append(location_z)

    n = len(xx)
    res = CameraCenters([np.array((xx[i], yy[i], zz[i])) for i in range(n)])
    res.orientations = quats
    f.close()
    return res