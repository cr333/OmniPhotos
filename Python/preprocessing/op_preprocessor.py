# -*- coding: utf-8 -*-

"""Define the OpPreprocessor class.

The module use to define the class OpPreprocessor.

  Typical usage example:

  op_preprocessor = OpPreprocessor()
  op_preprocessor.generating_config_files()
"""

import os
import copy
import sys
import pathlib
import hashlib
import shutil
import re
import csv
import subprocess
import concurrent.futures
import multiprocessing
import json

import yaml
import msgpack

import numpy as np
from PIL import Image
from skimage import transform, io

from flownet2.utils.flow_utils import writeFlow, readFlow
from flownet2.utils import computeColor

from abs_preprocessor import AbsPreprocessor
import circleselector

class OpPreprocessor(AbsPreprocessor):
    """Class to generate the config files for OmniPhotos.
    """

    def __init__(self, args):
        """
        load all configuration and set-up runtime environment (directories)
        :param args: parameter from CLI
        """
        super().__init__(args)
        # OmniPhotos configuration file template
        self.omniphotos_config_template_path = \
            self.current_dir + '/template/config.template.yaml'

        self.dataset_name = os.path.basename(str(self.root_dir))

        # the configuration files folder
        self.output_config_file_path = self.root_dir / "Config"
        self.dir_make(self.output_config_file_path)

        # the path & parameters of OmniPhotos executable file
        self.omniphotos_path = self.config["preprocessing.omniphotos.execution_file_path"]
        self.op_frame_number = self.config["preprocessing.frame_fixed_number"]
        if self.op_frame_number == -1:
            self.op_frame_number = len(self.op_image_list)
        self.stable_enable = 1 if self.config["preprocessing.omniphotos.stable_enable"] else 0
        self.circle_radius = self.config["preprocessing.omniphotos.circle_radius"]
        if self.circle_radius is None or self.circle_radius <= 0:
            self.show_info("The circle radius {} are wrong.".format(self.circle_radius),"error")

        self.op_exe_config_file = self.config["preprocessing.omniphotos.yaml_config_path"]

        # get the resolution of input images
        self.op_input_downsample_scalar = self.config["preprocessing.omniphotos.downsample_scalar"]
        # optical flow down sample scalar
        self.of_downsample_scalar = self.config["preprocessing.of.downsample_scalar"]
        self.op_input_frame_width = -1
        self.op_input_frame_height = -1
        self.comput_downsample_scalar()

        # optical flow configuration
        self.opticalflow_method = self.config["preprocessing.of.method"]
        self.save_visualization = self.config["preprocessing.of.save_visualization"]

        # the folder as convention
        self.cache_dir = self.root_dir / "Cache"
        self.dir_make(self.cache_dir)
        self.cache_backup_dir = self.root_dir / "Cache_backup"
        self.capture_data_dir = self.root_dir / "Capture"
        self.capture_data_backup_dir = self.root_dir / "Capture_backup"

        # cache folder naming convention
        # viewNumber-[full/half/quarter]Res-[nostab/stab]-[DIS/flownet2]
        self.cache_folder_path_flownet2 = None
        self.op_preprocessing_cache_dir = None



    def comput_downsample_scalar(self):
        """
        """
        # get the resolution of input images
        if self.op_input_downsample_scalar == [-1.0, -1.0] or self.op_input_downsample_scalar == [1.0, 1.0]:
            self.op_input_frame_height = self.frame_height
            self.op_input_frame_width = self.frame_width
        elif self.op_input_downsample_scalar[0] == self.op_input_downsample_scalar[1] and\
            0.0 < self.op_input_downsample_scalar[0] < 1.0 and 0.0 < self.op_input_downsample_scalar[1] < 1.0 :
            self.op_input_frame_height = int(self.frame_height * self.op_input_downsample_scalar[0])
            self.op_input_frame_width = int(self.frame_width * self.op_input_downsample_scalar[1])
        else:
            self.show_info("Input images downsample scalar {} is wrong.".format(self.op_input_downsample_scalar), "error")
        self.show_info("Input images resolution is {} x {}.".format(self.op_input_frame_height, self.op_input_frame_width))

        # optical flow down sample scalar
        if self.of_downsample_scalar == [-1.0, -1.0] or \
            (self.of_downsample_scalar[0] == self.of_downsample_scalar[1] and 
                0.0 < self.of_downsample_scalar[0] <= 1.0 and \
                0.0 < self.of_downsample_scalar[1] <= 1.0) :
            self.show_info("Input images downsample scalar {} is wrong.".format(self.of_downsample_scalar))
        else:
            self.show_info("Input images downsample scalar {} is wrong.".format(self.of_downsample_scalar), "error")
        # check the scalar for OmniPhotos
        if self.of_downsample_scalar != [-1.0, -1.0] \
                and self.of_downsample_scalar != [1.0, 1.0] \
                and self.of_downsample_scalar != [0.5, 0.5]:
            self.show_info("The OmniPhotos preprocessing program only generate half resultion downsampled optical flow.", "error")

    def generating_cache_filename(self):
        """
        generate the file name of cache directory.
        The name compose with "ImagesNumber-InputImageResolution-OpticalFlowResolution-FlowMethod".
        """
        if self.cache_folder_name is None:
            self.show_info("cache folder name do not set, genreate the cache file name base on the image resolution.")
        else:
            self.cache_folder_path_flownet2 = cache_folder_prefix + "-flownet2"
            self.op_preprocessing_cache_dir = cache_folder_prefix + "-DIS"
            self.show_info("cache folder name with pre-setting prefix: {}".format(self.cache_folder_name))
            return

        resolution_abbr_dict = {"5760x2880":"5_7K","3840x1920":"4k", \
                                "3008x1504":"3k", "2880x1440":"2_9K", \
                                "1920x960":"2k", "1504x752":"1_5k", \
                                "960x480":"1k", "480x240":"0_5k"}
        dir_name_express= "ImagesNumber-InputImageResolution-OpticalFlowResolution-FlowMethod"

        # get input image resolution abbreviation
        input_image_resolution_str = str(int(self.op_input_frame_width)) + "x" + str(int(self.op_input_frame_height))
        try:
            input_image_res_abbr = resolution_abbr_dict[input_image_resolution_str]
        except KeyError as err:
            self.show_info("Input image resolution abbreviation error: {}.".format(input_image_resolution_str,err),"error")

        # get the optical flow resolution abbreviation
        of_frame_height = self.op_input_frame_height * abs(self.of_downsample_scalar[0])
        of_frame_width = self.op_input_frame_width * abs(self.of_downsample_scalar[1])
        of_resolution_str = str(int(of_frame_width)) + "x" + str(int(of_frame_height))
        try:
            of_res_abbr = resolution_abbr_dict[of_resolution_str]
        except KeyError as err:
            self.show_info("Optical flow resolution abbreviation error: {}.".format(of_resolution_str, err),"error")

        # generate the cache directory name
        # generate the filename for MP cache directory
        frame_number = str(self.image_end_idx - self.image_start_idx)
        self.op_preprocessing_cache_dir = frame_number + "-" + input_image_res_abbr + "-" + of_res_abbr + "-DIS"
        # generate the filename for flownet2 cache directory
        self.cache_folder_path_flownet2 = frame_number + "-" + input_image_res_abbr + "-" + of_res_abbr + "-flownet2"


    def generating_config_files(self):
        """
        generate the necessary files for OmniPhotos
        """

        # 1) select the best circle
        # do this here so we copy only the images we need to the Input image directory.
        if self.find_stable_circle:
            self.openvslam_select_stable_circle()
        self.generating_cache_filename()
        # 2) move the image from trajectory directory to ready folder
        #  downsample the input image with the setting \
        # "preprocessing.omniphotos.downsample_scalar"
        if not self.op_images_dir.exists() or \
                len(os.listdir(self.op_images_dir))!=self.image_start_idx-self.image_end_idx:
            self.show_info("Generate the input images for OmniPhotos to {}".format(self.op_images_dir))
            self.omniphotos_generate_input_images()

        # 3) convert and generate the trajectory files as OmniPhotos request format
        self.show_info("Checking necessary file.")
        #  check the OpenVSLAM necessary file
        if self.trajectory_tool == "openvslam" or self.trajectory_tool == "all":
            self.dir_make(self.output_directory_path_ovslam)

            # check whether need the format conversion, if files exist do not convert 
            openvslam_need_convertion = False
            for term in self.openvslam_essential_file_list:
                openvslam_need_convertion = openvslam_need_convertion or (not (self.output_directory_path_ovslam / term).exists())
            # generate the configuration files for OpenVSLAM
            if openvslam_need_convertion:
                # check the openvslam output files
                for term in self.openvslam_output_file_list:
                    if not (self.output_directory_path_ovslam / term).exists():
                        self.show_info("The OpenVSLAM output file {} do not exist.".format(term), "error")

                # convert the openvslam output "frame_trajectory.txt" format
                self.show_info("Convert the OpenVSLAM result to OmniPhotos configuration files.")

                self.openvslam_convert_traj_file(
                    str(self.output_directory_path_ovslam), str(self.openvslam_config_file))
                # generate the cameras.txt used by OmniPhotos
                self.openvslam_create_camera_file(
                    self.output_directory_path_ovslam / "cameras.txt")
                # convert the *.msg file to json, and output point cloud to obj file
                self.openvslam_parser_msg(
                    str(self.output_directory_path_ovslam / "map.msg"))
                # backup the unnecessary files, save reconstruction config file
                self.show_info("save reconstruction config file")
                shutil.copyfile(str(self.openvslam_config_file), str(
                    self.output_directory_path_ovslam / "config.yaml"))
            else:
                self.show_info("The OpenVSLAM essentail file exist. Skip OpenVSLAM file convertion.")

        # check the colmap necessary file complete
        if self.trajectory_tool == "colmap" or self.trajectory_tool == "all":
            file_complete = False
            for term in self.colmap_essential_file_list:
                file_complete = file_complete or not (self.output_directory_path_colmap / term).exists()

            if file_complete:
                self.show_info("The OpenVSLAM essentail file do exist.", "error")

        # 4) make the directory & file structure as the OmniPhotos request
        self.show_info("Backup and copy the trajectory reconstruction.")
        need_file_structure_trim = False
        if self.trajectory_tool == "colmap" or self.trajectory_tool == "all":
            if len(os.listdir(str(self.output_directory_path_colmap))) > len(self.colmap_essential_file_list):
                need_file_structure_trim = need_file_structure_trim or True
            elif len(os.listdir(str(self.output_directory_path_colmap))) < len(self.colmap_essential_file_list):
                self.show_info("The COLMAP essential files are not complete", "error")
        if self.trajectory_tool == "openvslam" or self.trajectory_tool == "all":
            if len(os.listdir(str(self.output_directory_path_ovslam))) > len(self.openvslam_essential_file_list):
                need_file_structure_trim = need_file_structure_trim or True
            elif len(os.listdir(str(self.output_directory_path_ovslam))) < len(self.openvslam_essential_file_list):
                self.show_info("The OpenVSLAM essential files are not complete.", "error")
        if need_file_structure_trim:
            self.show_info("Trimming the trajectory files structure as OmniPhotos request")
            self.file_structure_trim()

        # 5) generate the config file and images for OmniPhotos
        self.show_info("Genera the config files for OmniPhotos viewer & preprocessing.", "info")
        self.omniphotos_generate_viewer_config()

        # 6) run the OmniPhotos pre-processing, generate *.flo, *.json and *.conf in Cache folder
        if not os.path.exists(str(self.cache_dir / self.op_preprocessing_cache_dir)):
            # if Cache folder is empty, run OmniPhotos preprocessor
            self.show_info("Run the pre-processing step of OmniPhotos.")
            self.omniphotos_preprocess()
        else:
            # if the result exist skip
            self.show_info(
                "The {} folder is not empty, skipping the OmniPhotos preprocessing step.".format(str(self.cache_dir)))

        # 7) initial flownet2 run-time environment
        self.show_info("Initial the Flownet2 run-time environment.")
        self.omniphotos_flownet2_init()

    def openvslam_select_stable_circle(self):

        self.show_info("Finding stable circle.")
        cached_res = os.path.join(self.root_dir, "circlefittingresults.json")
        if os.path.exists(cached_res):
            self.show_info("Found cached results at " + cached_res)
            intervals = circleselector.datatypes.PointDict()
            intervals.fromJSON(os.path.join(self.root_dir, "circlefittingresults.json"))

        else:
            points = circleselector.loader.load_file(
                os.path.join(self.output_directory_path_ovslam, "frame_trajectory.txt"))
            self.show_info("Calculating metrics ... ")
            intervals = circleselector.metrics.calc(points, errors = ["endpoint_error", "perimeter_error", "flatness_error",
                                                                "pairwise_distribution"]).find_local_minima(len(points))


            self.show_info(str(len(intervals)) + " valid intervals found.")
            intervals = circleselector.metrics.calc(points,
                                                    point_dcts=intervals,
                                                    errors=["ssim","psnr"],
                                                    mp=False,
                                                    dataset_path=self.root_dir,
                                                    rel_input_image_path='trajectory_images')
            intervals.toJSON(os.path.join(self.root_dir, "circlefittingresults.json"))

        if len(intervals) == 0:
            self.show_info("No intervals found.","error")

        stable_circle = intervals.find_best_interval()["interval"]
        self.image_start_idx, self.image_end_idx = stable_circle[0], stable_circle[1]
    def omniphotos_generate_input_images(self):
        """
        generate the images for OmniPhotos input.
        copy & downsample the image from traj_input_images_dir to op_images_dir
        """
        # create folder for downsample input images
        self.dir_make(self.op_images_dir)

        if self.op_input_frame_height == self.frame_height and self.op_input_frame_width == self.frame_width:
            for idx, val in enumerate(self.trajectory_images_list[self.image_start_idx:self.image_end_idx]):
                src_image_path = self.traj_input_images_dir / val
                dest_image_path = self.op_images_dir / val
                if idx % self.show_infor_interval == 0:
                    self.show_info("Copy image from {} to {}.".format(str(src_image_path), str(dest_image_path)))
                shutil.copyfile(src_image_path, dest_image_path)
        else:
            self.show_info(\
                "Downsample the input images to {} x {}".format(self.op_input_frame_height, self.op_input_frame_width))
            self.omniphotos_downsample_input_images()

    def omniphotos_downsample_input_images(self):
        """
        downsample the input images as the configuration of \
        preprocessing.OmniPhotos.downsample_scalar in yaml file.
        And replace the original full resolution images (Input folder), backup the
        original folder "Input" as "Input_backup".
        """
        #with concurrent.futures.ThreadPoolExecutor(max_workers=1) as executor:
        with concurrent.futures.ThreadPoolExecutor(max_workers=multiprocessing.cpu_count()) as executor:
            def downsample_images(input_image_folder_path, output_info):
                input_image_list = output_info[0]
                output_image_list = output_info[1]
                frame_idx = output_info[2]
                image_data_downsample_size = output_info[3]
                output_image_folder_path = output_info[4]

                input_image_term = input_image_list[frame_idx]
                output_image_term = output_image_list[frame_idx]
                image_data = io.imread(
                    str(input_image_folder_path / input_image_term))
                image_data = transform.resize(
                    image_data, image_data_downsample_size, preserve_range=True)

                if frame_idx % self.show_infor_interval == 0:
                    self.show_info("Save image {} to {}.".format(
                        input_image_term, str(output_image_folder_path)))
                    self.show_image(image_data)

                if os.path.splitext(output_image_term) in ['jpg', 'jpeg']:
                    io.imsave(str(output_image_folder_path /
                                  output_image_term), image_data, quality=95)
                else:
                    io.imsave(str(output_image_folder_path /
                                  output_image_term), image_data)

            return_results = {executor.submit(downsample_images, self.traj_input_images_dir,
                                              [self.trajectory_images_list, self.op_image_list, frame_list_idx,
                                               (self.op_input_frame_height, self.op_input_frame_width, 3), self.op_images_dir]): frame_list_idx for frame_list_idx in range(0, len(self.trajectory_images_list))}
            for future in concurrent.futures.as_completed(return_results):
                if not future.done():
                    self.show_info("Images downsample processing error.", "error")

    def omniphotos_logging(self):
        """
        Output preprocessing log file to the cache directory. 
        The log info including at binary HASH code, commit info, etc.
        """
        logging_str = ""

        # 0) OmniPhotos bin path
        logging_str = logging_str + "The OmniPhotos bin file path: {} \n".format(self.omniphotos_path)

        # 1) HASH code
        BUF_SIZE = 1048576  # read 1MB chunks
        md5 = hashlib.md5()
        sha1 = hashlib.sha1()

        if not os.path.exists(self.omniphotos_path):
            raise RuntimeError("{} do not exist".format(self.omniphotos_path))

        with open(self.omniphotos_path, 'rb') as f:
            while True:
                data = f.read(BUF_SIZE)
                if not data:
                    break
                md5.update(data)
                sha1.update(data)

        logging_str = logging_str + "MD5:" +  md5.hexdigest() + "\n"
        logging_str = logging_str + "SHA1:" + sha1.hexdigest() + "\n"
        return logging_str

    def omniphotos_preprocess(self):
        """
        OmniPhotos preprocessing program composing with following steps:
        """
        # 1) generate the configuration file for the stabilized
        if self.trajectory_tool == "colmap" or self.trajectory_tool == "all":
            modelfiles = "modelFiles.txt"
            self.show_info("OmniPhotos preprocessing with COLMAP trajectory.")
        elif self.trajectory_tool == "openvslam":
            modelfiles = "modelFiles.openvslam"
            self.show_info(
                "OmniPhotos preprocessing with OpenVSLAM trajectory.")
        else:
            self.show_info("Trajectory reconstruction have error.")

        if self.op_exe_config_file == "None":
            stabilization_parameters = {
                r'\$circle_radius\$': str(self.circle_radius),
                r'\$dataset_name\$': self.dataset_name,
                r'\$cameras_number\$': str(self.image_end_idx - self.image_start_idx),
                r'\$process_step\$': "stabilization images",
                r'\$cache_folder_dis\$': self.op_preprocessing_cache_dir,
                r'\$cache_folder_flownet2\$': self.cache_folder_path_flownet2,
                r'\$sfm_file_name\$': modelfiles,
                r'\$load_3d_points\$': str(1),
                r'\$image_filename\$': self.op_filename_expression,
                r'\$load_sparse_point_cloud\$': str(1),
                r'\$image_width\$': str(self.op_input_frame_width),
                r'\$image_height\$': str(self.op_input_frame_height),
                r'\$first_frame\$': str(0),
                r'\$last_frame\$': str(self.image_end_idx - self.image_start_idx),
                r'\$image_fps\$': str(self.frame_fps),
                r'\$change_basis\$': str(0),
                r'\$intrinsic_scale\$': str(1.0),
                r'\$stabilize_images\$': str(self.stable_enable),
                r'\$stabilization_limit\$': str(10000),
                r'\$downsample_flow\$': str(0) if self.of_downsample_scalar[0] == -1 or self.of_downsample_scalar[0] == 1 else str(1),
                r'\$compute_optical_flow\$': str(1),
                r'\$use_point_cloud\$': str(1),
            }
            self.op_exe_config_file = str(
                self.output_config_file_path / "config-preprocessing.yaml")
            self.instance_template(self.omniphotos_config_template_path, self.op_exe_config_file,
                                   stabilization_parameters)

        # 2) call OmniPhotos to do preprocess
        if not os.path.exists(self.omniphotos_path):
            self.show_info("OmniPhotos program {} not found".format(self.omniphotos_path), "error")

        try:
            output = subprocess.run(
                [self.omniphotos_path, "-f", self.op_exe_config_file], check=True)
            output.check_returncode()
        except subprocess.CalledProcessError:
            self.show_info("Starting up OmniPhotos program fail. {}".format(self.omniphotos_path))

        # 3) output log to cache file
        log_str = self.omniphotos_logging()
        log_file_path = self.cache_dir / "preprocessing.log"
        with open(str(log_file_path), "w") as file_handle:
            file_handle.write(log_str)

        self.show_info("output preprocessing log to {}".format(log_file_path))

    def omniphotos_generate_viewer_config(self):
        """
        generate the necessary config file (*.yaml etc) for OmniPhotos viewer
        """
        # 1) generate OmniPhotos config files
        # assign configuration file variables
        omniphotos_config_info_common = {
            r'\$circle_radius\$': str(self.circle_radius),
            r'\$dataset_name\$': self.dataset_name,
            r'\$image_width\$': str(self.op_input_frame_width),
            r'\$image_height\$': str(self.op_input_frame_height),
            r'\$first_frame\$': str(0),
            r'\$last_frame\$': str(self.image_end_idx-self.image_start_idx),
            r'\$image_fps\$': str(self.frame_fps),
            r'\$cameras_number\$': str(self.image_end_idx-self.image_start_idx),
            r'\$image_filename\$': self.op_filename_expression,
            r'\$process_step\$': "stabilization images",
            r'\$cache_folder_dis\$': self.op_preprocessing_cache_dir,
            r'\$cache_folder_flownet2\$': self.cache_folder_path_flownet2,
            r'\$load_3d_points\$': str(1),
            r'\$load_sparse_point_cloud\$': str(1),
            r'\$change_basis\$': str(0),
            r'\$intrinsic_scale\$': str(1.0),
            r'\$stabilize_images\$': str(self.stable_enable),
            r'\$stabilization_limit\$': str(10000),
            r'\$downsample_flow\$': str(0) if self.of_downsample_scalar[0] == -1 or self.of_downsample_scalar[0] == 1 else str(1),
            r'\$compute_optical_flow\$': str(0),
            r'\$use_point_cloud\$': str(1),
        }

        # read OmniPhotos config file template
        with open(self.omniphotos_config_template_path, 'r') as file_handle:
            content = file_handle.read()

        # 2) output OmniPhotos config file for openvslam
        if self.trajectory_tool == "all" or self.trajectory_tool == 'openvslam':
            omniphotos_config_info = copy.copy(omniphotos_config_info_common)
            content_output = copy.copy(content)
            # output the config.yaml file for openvslam
            output_config_file_name = "config-viewer-openvslam.yaml"
            output_model_file_name = "modelFiles.openvslam"
            omniphotos_config_info[r'\$trajectory_tool\$'] = self.trajectory_tool
            omniphotos_config_info[r'\$sfm_file_name\$'] = output_model_file_name

            for key, value in omniphotos_config_info.items():
                content_output = re.sub(
                    key, value, content_output, flags=re.MULTILINE)
            with open(str(self.output_config_file_path / output_config_file_name), 'w') as file_handle:
                file_handle.write(content_output)

            # output model file modelFiles.txt content & check the file
            openvslam_model_file_list = ["../Capture/openvslam/cameras.txt",\
                                                   "../Capture/openvslam/frame_trajectory_with_filename.txt",\
                                                   "../Capture/openvslam/map.msg"]
            openvslam_model_file_list_check = [re.sub(r"\.\.", self.root_dir.as_posix(),\
                                        file_path, flags=re.M) for file_path in openvslam_model_file_list]
            for file in openvslam_model_file_list_check:
                if not os.path.exists(file):
                    self.show_info("The {} do not exist.".format(file), "error")
            model_file_content_openvslam = "\n".join(openvslam_model_file_list)
            with open(str(self.output_config_file_path / output_model_file_name), 'w') as file_handle:
                file_handle.write(model_file_content_openvslam)

        # 3) output OmniPhotos config file for colmap
        if self.trajectory_tool == "all" or self.trajectory_tool == 'colmap':
            omniphotos_config_info = copy.copy(omniphotos_config_info_common)
            content_output = copy.copy(content)

            # output the config file for colmap
            output_config_file_name = "config-viewer-colmap.yaml"
            output_model_file_name = "modelFiles.txt"
            omniphotos_config_info[r'\$trajectory_tool\$'] = self.trajectory_tool
            omniphotos_config_info[r'\$sfm_file_name\$'] = output_model_file_name

            for key, value in omniphotos_config_info.items():
                content_output = re.sub(
                    key, value, content_output, flags=re.MULTILINE)
            with open(str(self.output_config_file_path / output_config_file_name), 'w') as file_handle:
                file_handle.write(content_output)

            # output model file modelFiles.txt content
            colmap_model_file_list = ["../Capture/COLMAP/cameras.txt",\
                                  "../Capture/COLMAP/images.txt",\
                                  "../Capture/COLMAP/points3D.txt"]
            colmap_model_file_list_check = [re.sub(r'..', self.root_dir.as_posix(),\
                                        file_path, flags=re.M) for file_path in colmap_model_file_list]
            for file in colmap_model_file_list_check:
                if not os.path.exists(file):
                    self.show_info("The {} do not exist.".format(file), "error")
            model_file_content_colmap = "\n".join(colmap_model_file_list)
            with open(str(self.output_config_file_path / output_model_file_name), 'w') as file_handle:
                file_handle.write(model_file_content_colmap)

    def omniphotos_flownet2_init(self):
        """
        init the folder for the flownet2 output fitting to OmniPhotos request
        """
        # 1) create folder of flownet2
        self.dir_make(self.cache_dir / self.cache_folder_path_flownet2)

        # 2) copy the essential files
        src_folder = self.cache_dir / self.op_preprocessing_cache_dir
        dest_folder = self.cache_dir / self.cache_folder_path_flownet2
        for term in src_folder.iterdir():
            if term.suffix in [".flo", ".floss"]:
                continue
            shutil.copyfile(str(term), str(dest_folder / term.name))

        # 3) rename the optical flow path in Cameras.csv to the flownet2 flo path
        cameras_csv_all_text = None
        with open(str(src_folder / "Cameras.csv"), "r") as file_handle:
            cameras_csv_all_text = file_handle.read()
        cameras_csv_all_text = cameras_csv_all_text.replace(str(self.op_preprocessing_cache_dir), str(self.cache_folder_path_flownet2))
        #cameras_csv_all_text = cameras_csv_all_text.replace(".floss", ".flo")
        with open(str(dest_folder / "Cameras.csv"), "w") as file_handle:
            file_handle.write(cameras_csv_all_text)

    def file_structure_trim(self):
        """
        Create the destination file structure define in readme.md. \
        Copy the the essential files to the destination directory, and back up the original files.
        NOTE: all file/directory path is relative path to root_dir
        """
        # 1) create the backup directories
        if self.capture_data_backup_dir.exists():
            self.show_info("Triming file structure trim, but the backup directory {} exist"\
                .format(str(self.capture_data_backup_dir)))
        self.dir_make(self.capture_data_backup_dir)

        # 2) backup files, and just remain the essential files
        move_file_list = {}
        copy_file_list = {}
        if self.trajectory_tool == "all" or self.trajectory_tool == "openvslam":
            move_file_list[self.capture_data_dir / "openvslam"] = self.capture_data_backup_dir / "openvslam"
            for term in self.openvslam_essential_file_list:
                copy_file_list.update({self.capture_data_backup_dir / "openvslam" / term: \
                        self.capture_data_dir / "openvslam" / term})

        if self.trajectory_tool == "all" or self.trajectory_tool == "colmap":
            move_file_list[self.capture_data_dir / "COLMAP"] = self.capture_data_backup_dir /  "COLMAP"
            for term in self.colmap_essential_file_list:
                copy_file_list.update({
                    self.capture_data_backup_dir /"COLMAP/full-model-ba-txt" / term: \
                        self.capture_data_dir / "COLMAP" / term})

        # backup files
        for key, value in move_file_list.items():
            if value.exists():
                self.show_info("Backup folder {} exist, can not backup".format(str(value)), "error")
            key.rename(value)
            self.dir_make(key)

        # copy essential OmniPhotos openvslam & COLMAP files to capture directory
        for key, value in copy_file_list.items():
            shutil.copy(str(key), str(value))

    def openvslam_convert_traj_file(self,
                                    openvslam_output_directory,
                                    openvslam_config_file_path):
        """
        transform the original trajectory file format to OmniPhotos format
        :param openvslam_output_directory: *.msg *.txt output directory
        :param openvslam_config_file_path: *.yaml config file for openvslam
        """
        if not os.path.exists(openvslam_output_directory):
            self.show_info("The folder {} do not exist.".format(openvslam_output_directory), "error")
        if not os.path.exists(openvslam_config_file_path):
            self.show_info("The openvslam config file {} do not exist, please put it on the root folder of the dataset."\
                .format(openvslam_config_file_path), "error")

        openvslam_output_directory_path = pathlib.Path(
            openvslam_output_directory)
        openvslam_traj_file_path = openvslam_output_directory_path / "frame_trajectory.txt"
        openvslam_traj_file_path_output = openvslam_output_directory_path / "frame_trajectory_with_filename.txt"

        output_csv_header = ["#index", "filename", "trans_wc.x", "trans_wc.y", "trans_wc.z", "quat_wc.x", "quat_wc.y",
                             "quat_wc.z", "quat_wc.w"]

        # load openvslam YAML config file get FPS
        with open(openvslam_config_file_path, "r") as yaml_config_file_handle:
            ovslam_config_str = yaml_config_file_handle.read()
        ovslam_config_data = yaml.load(ovslam_config_str, Loader=yaml.CLoader)
        ovslam_fps = ovslam_config_data["Camera.fps"]

        # load frame_trajectory.txt
        yaml_traj_file_handle = open(openvslam_traj_file_path)
        yaml_traj_csv_file_handle = csv.reader(
            yaml_traj_file_handle, delimiter=' ', quotechar='|')

        # map timestamp to filename & output newfile with filename
        output_counter= 0
        with open(openvslam_traj_file_path_output, 'w', newline='') as yaml_traj_file_handle_output:
            yaml_traj_csv_file_handle_output = csv.writer(yaml_traj_file_handle_output, delimiter=' ',
                                                          quoting=csv.QUOTE_MINIMAL)
            yaml_traj_csv_file_handle_output.writerow(output_csv_header)
            for enum,row in enumerate(yaml_traj_csv_file_handle):
                if self.image_start_idx < enum < self.image_end_idx:
                    idx = int(ovslam_fps * float(row[0]) + 0.5)
                    row[0] = self.op_filename_expression % idx
                    yaml_traj_csv_file_handle_output.writerow([str(idx)] + row)
                    
    def openvslam_create_camera_file(self, output_path):
        """
        generate the openvslam cameras.txt file for OmniPhotos
        :param output_path: the path of output camera.txt file for openvslam
        :type output_path:
        """
        with open(output_path, 'w', newline='') as output_file_handle:
            output_file_handle.write(
                "# Camera list with one line of data per camera:\r\n")
            output_file_handle.write(
                "#   CAMERA_ID, MODEL, WIDTH, HEIGHT, PARAMS[]\r\n")
            output_file_handle.write("# Number of cameras: 1\r\n")
            camera_info = "1 EQUIRECTANGULAR {} {} {} {} {}\r\n"
            if self.input_type == "video":
                camera_info = camera_info.format(self.frame_width, self.frame_height, 500, self.frame_width / 2,
                                                 self.frame_height / 2)
            elif self.input_type == "image":
                camera_info = camera_info.format(self.frame_width, self.frame_height, 500, self.frame_width / 2,
                                                 self.frame_height / 2)
            output_file_handle.write(camera_info)

    def openvslam_parser_msg(self, msg_file_path):
        """
        parser the msg output the corresponding json and landmark (point cloud).
        output to the same folder of input msg file
        :param msg_file_path: the path of msg file
        """
        json_file_path = msg_file_path + ".json"
        obj_file_path = msg_file_path + ".obj"
        data = open(msg_file_path, 'rb')
        data_text = msgpack.Unpacker(data, raw=False).unpack()

        # 1) convert the msg to json
        self.show_info("Output OpenVSLAM msg file to json : {}".format(json_file_path))
        with open(json_file_path, 'w') as json_file:
            json_file.write(json.dumps(data_text))

        # 2) generate the obj file for point cloud
        self.show_info("Output OpenVSLAM point cloud to: {}".format(obj_file_path))
        landmarks = data_text["landmarks"]
        with open(obj_file_path, 'w') as obj_file:
            for key in landmarks:
                landmark = landmarks[key]["pos_w"]
                point_txt = "v {} {} {} {}".\
                    format(str(landmark[0]), str(
                        landmark[1]), str(landmark[2]), os.linesep)
                obj_file.write(point_txt)
