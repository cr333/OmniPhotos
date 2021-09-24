# -*- coding: utf-8 -*-
"""
implement data preparing by rotation, extraction images & video.\
    To generate the data for trajectory reconstruction
"""
import os
import pathlib
import multiprocessing
import concurrent.futures

import numpy as np
from scipy.spatial.transform import Rotation as R
import PIL
import ffmpeg
import imageio

from envmap import EnvironmentMap

from abs_preprocessor import AbsPreprocessor


class DataPreprocessor(AbsPreprocessor):
    """
    data prepare class
    """

    def __init__(self, args):
        """
        load all configuration and set-up runtime environment (directories)
        :param args: a directory containing config info
        """
        super().__init__(args)
        self.input_data = None
        self.image_list = []

        self.persp_forward = self.config["preprocessing.colmap.perspective_forward"]
        self.persp_fov_v = self.config["preprocessing.colmap.perspective_fov_vertical"]
        self.persp_resolution_height = \
            self.config["preprocessing.colmap.perspective_resolution_height"]
        self.persp_resolution_width = \
            self.config["preprocessing.colmap.perspective_resolution_width"]
        self.rotation = self.config["preprocessing.image_rotation"]
        self.frame_fixed_number = self.config["preprocessing.frame_fixed_number"]

        self.dir_make(self.traj_input_images_dir)
        if self.image_type == "panoramic" and (self.trajectory_tool == "all" or self.trajectory_tool == "colmap"):
            self.dir_make(self.image_perspective_output_path)

        # set the thread number for image processing, include rotation, etc.
        self.rotation_num_threads = 1

    def save_image_seq(self):
        """
        save the numpy data to image
        """
        for i in range(0, np.shape(self.input_data)[0]):  # multi-thread
            image = PIL.Image.fromarray(self.input_data[i])
            file_name = self.original_filename_expression % i
            # generate output image file path
            output_image_path = self.traj_input_images_dir / file_name
            if output_image_path.suffix in [".jpeg", ".jpg"]:
                image.save(str(output_image_path), quality=95)
            else:
                image.save(str(output_image_path))
            if i % self.show_infor_interval == 0:
                self.show_info("Image idx is {}, output to {}.".format(i, output_image_path))
                self.show_image(self.input_data[i])

    def save_images(self, output_path_list, data):
        """
        save the numpy data to image
        :param output_path_list: the output path for storage images, \
            length is same as data first dimension
        :type output_path_list: list
        :param data: output image data, dimension is [x, height, width, 3]
        :type data: numpy
        """
        if len(output_path_list) != np.shape(data)[0]:
            self.show_info("The output data size no right.", "error")

        if not isinstance(output_path_list[0], pathlib.Path):
            self.show_info("The path type should be pathlib.", "error")

        for i in range(0, np.shape(data)[0]):  # multi-thread
            image = PIL.Image.fromarray(data[i])
            if output_path_list[i].suffix in [".jpeg", ".jpg"]:
                image.save(str(output_path_list[i]), quality=95)
            else:
                image.save(str(output_path_list[i]))

    def rotate_image_fast(self, data):
        """
        rotation panoramic image with drift matrix
        :param data: image data will be rotated, dimension is 4 [ x , width, height, 3]
        :return : weather rotated images
        """
        if [0.0, 0.0] == self.rotation:
            return data

        phi_roll_numb = self.rotation[0] / 360.0 * np.shape(data)[2]
        if phi_roll_numb == int(phi_roll_numb) and self.rotation[1] == 0.0:
            # do not need to interpolate, use numpy roll operation
            data[:] = np.roll(data, int(phi_roll_numb), axis=2)
            return data

        # interpolate (rotate) with skylibs
        return self.rotate_image(data)

    def rotate_image(self, data):
        """
        rotation images with skylibs
        :param data: image data will be rotated, dimension is 4 [ x , width, height, 3]
        :return : weather rotated the images
        """
        if [0.0, 0.0] == self.rotation:
            return data

        rotation_matrix = np.zeros([3, 3])
        self.spherical2dcm(self.rotation, rotation_matrix)
        for i in range(0, np.shape(data)[0]):
            if i % self.show_infor_interval == 0:
                self.show_info("Rotation image with 'rotate_image' function, index is {}.".format(i))
            # show_image(image)
            image = data[i]
            envmap = EnvironmentMap(image, format_='latlong')
            new_image = envmap.rotate("DCM", rotation_matrix).data
            new_image = new_image.astype(np.uint8)
            data[i] = new_image
        return data

    def extract_perspective_image(self, frame_data):
        """
        extract perspective images from panoramic images. \
            The aspect ratio of image is the proportional difference between width and height.
        :param frame_data: frame data
        """
        # collect the parameters
        aspect_ratio = float(self.persp_resolution_width) / self.persp_resolution_height
        resolution = (self.persp_resolution_width, self.persp_resolution_height)
        # self.show_info("generate perspective images with rotation, {}".format(persp_forward))

        # get forward rotation matrix
        rotation_matrix = np.zeros([3, 3])
        self.spherical2dcm(self.persp_forward, rotation_matrix)

        prespect_frame_data = \
            np.ones((np.shape(frame_data)[0], self.persp_resolution_height, \
                    self.persp_resolution_width, 3), dtype=np.uint8)
        for idx in range(0, np.shape(frame_data)[0]):
            try:
                envmap = EnvironmentMap(frame_data[idx], format_='latlong')
            except AssertionError as error:
                msg = "ERROR! extract perspective image error \" {} \"".format(error)
                print(msg)
                raise

            # get perspective images
            new_image = envmap.project(vfov=self.persp_fov_v, \
                rotation_matrix=rotation_matrix, ar=aspect_ratio, resolution=resolution)
            prespect_frame_data[idx, :, :, :] = new_image.astype(np.uint8)
        return prespect_frame_data

    def preprocess_data(self):
        """
        preprocess data from video or images
        """
        if not self.input_path.exists():
            msg = "Preprocessing.input_path do not exist: {}.".format(str(self.input_path))
            self.show_info(msg, "error")

        self.show_info(("Preprocessing {}, the input data is :{}."\
            .format(self.input_type, str(self.input_path))))

        # load data
        if self.input_type == 'video':
            self.preprocess_video(str(self.input_path))  # input_data_temp is readonly
        elif self.input_type == 'image':
            self.preprocess_image(str(self.input_path))
        else:
            msg = 'Input_type error: {}.'.format(self.input_type)
            self.show_info(msg, "error")

    def preprocess_image(self, images_path):
        """
        load image and save the images by rotation, getting pin-hole e.t.c
        :param images_path: the input images path
        :return :
        """
        self.show_info("Start preporcessing the image data.")
        if self.image_type != "panoramic":
            self.show_info("The input images are not panoramic images, {} do nothing."\
                .format(self.__class__.__name__))
            return

        # get the images from Input folder
        input_image_list = []
        self.get_image_file_list(images_path, input_image_list)

        # do with multi-thread
        #with concurrent.futures.ThreadPoolExecutor(max_workers=multiprocessing.cpu_count()) as executor:
        with concurrent.futures.ThreadPoolExecutor(max_workers=self.rotation_num_threads) as executor:
            def process_images(image_path, image_info):
                image_idx = image_info[1]
                image_path = pathlib.Path(image_info[0]) / image_path
                self.show_info("Processing image {}.".format(image_path))

                # 1) load image from disk
                if not image_path.exists():
                    self.show_info("The images do not exist {}.".format(str(image_path)))
                frame_data = np.asarray(imageio.imread(str(image_path)))

                # check image resolution 
                if frame_data.shape[1] != frame_data.shape[0] * 2:
                    self.show_info("The column number is not the 2 time of row number,\
                         crop the image {}.".format(image_path), "warning")
                    if frame_data.shape[1] > frame_data.shape[0] * 2:
                        frame_data = frame_data[:, 0:frame_data.shape[0]*2, :]
                    else:
                        frame_data = frame_data[0:int(frame_data.shape[1]/2), :, :]

                # 2) rotate image and save
                self.rotate_image_fast(frame_data[np.newaxis, :])
                self.save_images([self.traj_input_images_dir / image_path.name], frame_data[np.newaxis, :])

                # 3) extract & save perspective image data for colmap panoramic images
                if self.image_type == "panoramic" and (self.trajectory_tool == "all" or self.trajectory_tool == "colmap"):
                    frame_data_persp = \
                        self.extract_perspective_image(frame_data[np.newaxis, :].astype(np.double))
                    file_name = self.original_filename_expression % image_idx
                    self.save_images([self.image_perspective_output_path / file_name], frame_data_persp)

                # 4) show result
                if image_idx >= 0 and image_idx % self.show_infor_interval == 0:
                    self.show_info("Precessing image: {}.".format(image_path))
                    self.show_image(frame_data)

            return_results = {executor.submit(
                process_images, input_image_list[image_idx], [images_path, image_idx])\
                    :image_idx for image_idx in range(0, len(input_image_list))}
            for future in concurrent.futures.as_completed(return_results):
                if not future.done():
                    self.show_info("Image processing error in function preprocess_image.", "error")


    def preprocess_video(self, video_path):
        """
        extract image from video, with rotation, getting pin-hole e.t.c, and save images.
        :param video_path: the video file path
        :return :
        """
        if self.traj_input_images_dir.exists() and len(os.listdir(str(self.traj_input_images_dir))) != 0 :
            self.show_info("The trajection input folder {} do not exist or empty, do not extract images."\
                .format(str(self.traj_input_images_dir)))
            return

        # extract all the frames
        ouput_path = os.path.join(self.traj_input_images_dir, os.path.splitext(self.original_filename_expression)[0]+".jpg")
        ffmpeg.input(video_path).output(ouput_path,start_number=0,**{'qscale:v': 2}).run(capture_stdout=True)

        # generate the list of desired frame indices
        if self.frame_fixed_number <= 0:
            vframes_size = 1  # NOTE : should be 1
            frame_idx_list = list(range(self.image_start_idx, self.image_end_idx + 1, vframes_size))
        else:
            frame_idx_list = \
                np.linspace(start=self.image_start_idx, \
                    stop=self.image_end_idx + 1, num=self.frame_fixed_number)
            frame_idx_list = list(frame_idx_list.astype(int))

        # remove the unwanted images
        for enum, image_name in enumerate(os.listdir(self.traj_input_images_dir)):
            if enum not in frame_idx_list:
                os.remove(os.path.join(self.traj_input_images_dir, image_name))

        # rotate the images and convert to jpeg
        #self.convert_png_to_jpeg(self.traj_input_images_dir)
        self.preprocess_image(self.traj_input_images_dir)

    def preprocess_images(self, directory_path):
        """
        load images to the memory
        :param directory_path: the path of image directory
        :return : a numpy array, shape is [frame_number, height, width, 3]
        """
        directory_path = pathlib.Path(directory_path)
        # load image to memory
        frame_data = PIL.Image.open(directory_path / self.image_list[0])
        self.input_data = np.zeros((len(self.image_list),) + np.shape(frame_data), np.uint8)
        for idx in range(0, len(self.image_list)):
            frame_data = PIL.Image.open(directory_path / self.image_list[idx])
            self.input_data[idx, :, :, :] = frame_data
            if idx % self.show_infor_interval == 0:
                self.show_info("Load {}th image: {}.".format(idx, self.image_list[idx]))
                self.show_image(np.array(frame_data))

    def spherical2dcm(self, spherical_coordinate, dcm):
        """
        convert spherical coordinate to DCM (Direction Cosine Matrix)
        :param spherical_coordinate: the spherical coordinate Euler angles [phi, theta]
        :param dcm: the rotation matrix (Direction Cosine Matrix), is 3x3 matrix
        :return:
        """
        dcm[:] = R.from_euler("xyz", [spherical_coordinate[1], \
                                      spherical_coordinate[0], 0], degrees=True).as_matrix()