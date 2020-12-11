#!/usr/bin/env python -u
# -*- coding: utf-8 -*-

# get_ipython().magic('pylab inline')

import os
import cv2
import numpy as np

## Install the following Python libraries:
##   pip install rotlib
##   pip install skylibs
import envmap

working_dir = r"D:\Insta360"

output_dir = working_dir + '-pinhole'
if not os.path.exists(output_dir):
    os.mkdir(output_dir)

# angle = 0 / np.pi  # 0 is forward, 90 is right, 180 is backward, 270 is left
# rotation = np.mat([[np.cos(angle), 0, np.sin(angle)], [0, 1, 0], [-np.sin(angle), 0, np.cos(angle)]])
vfov = 120
aspect = 1.
resolution = (1200, 1200)

for frame in range(1, 20000):
# for frame in range(925, 1133, 5):
    filename = os.path.join(working_dir, '%04d.jpg' % frame)
#     print(filename)

    image = cv2.imread(filename)
    image = image[:, :, ::-1] / 255.  # convert to unit range RGB
#     imshow(image)

    env = envmap.EnvironmentMap(image, 'latlong')

    angle = np.deg2rad(0)  # 0 is forward, 90 is right, 180 is backward, 270 is left
    rotation = np.mat([[np.cos(angle), 0, np.sin(angle)], [0, 1, 0], [-np.sin(angle), 0, np.cos(angle)]])
    pinhole = env.project(vfov, rotation, ar=aspect, resolution=resolution)
    cv2.imwrite(os.path.join(output_dir, '%04d.jpg' % frame), 255 * pinhole[:,:,::-1])

    if frame == 1:
        mask = env.project(vfov, rotation, ar=aspect, mode='mask')
        cv2.imwrite(os.path.join(output_dir, 'mask.png'), 255 * mask)

    # # Extract the view to the left.
    # angle = np.deg2rad(-60)  # 0 is forward, 90 is right, 180 is backward, 270 is left
    # rotation = np.mat([[np.cos(angle), 0, np.sin(angle)], [0, 1, 0], [-np.sin(angle), 0, np.cos(angle)]])
    # pinhole = env.project(vfov, rotation, ar=aspect, resolution=resolution)
    # cv2.imwrite(os.path.join(output_dir, '%04d-left.jpg' % frame), 255 * pinhole[:,:,::-1])
    #
    # # Extract the view to the right.
    # angle = np.deg2rad(60)  # 0 is forward, 90 is right, 180 is backward, 270 is left
    # rotation = np.mat([[np.cos(angle), 0, np.sin(angle)], [0, 1, 0], [-np.sin(angle), 0, np.cos(angle)]])
    # pinhole = env.project(vfov, rotation, ar=aspect, resolution=resolution)
    # cv2.imwrite(os.path.join(output_dir, '%04d-right.jpg' % frame), 255 * pinhole[:,:,::-1])

    print(frame)

# ## Construct intrinsic matrix L.
# f_x = (resolution[0] / 2.) / tan(vfov * aspect / 180. * np.pi / 2.)
# f_y = (resolution[1] / 2.) / tan(vfov / 180. * np.pi / 2.)
# K = np.mat([[f_x, 0., resolution[0] / 2.], [0., f_y, resolution[1] / 2.], [0., 0., 1.]])
# print(K)
