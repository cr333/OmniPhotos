# -*- coding: utf-8 -*-
"""
The module implement camera trajectory reconstruction
"""
import subprocess
import os
import copy
import time

import numpy as np

from colmap import colmap2stepReconstruction
from abs_preprocessor import AbsPreprocessor


class TrajPreprocessor(AbsPreprocessor):
    """
    Reconstructing the trajectory of camera .
    """

    def __init__(self, args):
        """
        load all configuration and set-up runtime environment (directories)
        :param args: a parsed CLI input options
        :type args: dict
        """
        super().__init__(args)

        self.input_data = None
        self.input_data_rotated = None

        # create output directory and file path
        self.dir_make(self.traj_output_directory_path)

        # the template of openvslam config file
        dist_dir = os.path.dirname(__file__)
        self.openvslam_config_template_file = dist_dir + '/template/openvslam.template.yaml'
        self.openvslam_program_path = os.path.join(dist_dir, self.config["preprocessing.openvslam.execution_file_path"])
        self.openvslam_vocab_path = os.path.join(dist_dir,self.config["preprocessing.openvslam.dbow2_file_path"])

    def trajectory_reconstruct(self):
        """
        reconstruct camera trajectory
        """
        if not self.trajectory_tool in self.trajectory_tool_list:
            msg = "Preprocessing.trajectory.tool is {}, should be \"openvslam\" or \"colmap\""\
                .format(self.trajectory_tool)
            self.show_info(msg, "error")
            return

        # clean GUI canvas
        self.show_image(np.full((2000, 9000, 3), 255, np.uint8))

        if self.trajectory_tool == "all" or self.trajectory_tool == "colmap":
            self.show_info("start COLMAP reconstruction")
            # initial COLMAP environment
            self.dir_make(self.output_directory_path_colmap)
            # call colmap reconstruction
            self.trajectory_reconstruct_colmap(str(self.image_perspective_output_path))
            self.show_info("Finish COLMAP reconstruction")

        if self.trajectory_tool == "all" or self.trajectory_tool == "openvslam":
            # check the trajectory result
            # if all necessary exist, do not reconstruct the trajectory.
            need_reconstruction = False
            if self.output_directory_path_ovslam.exists():
                file_list = os.listdir(str(self.output_directory_path_ovslam))
                # check openvslam output file
                openvslam_output_file_exist = True
                for file_name in self.openvslam_output_file_list:
                    if not file_name in file_list:
                        openvslam_output_file_exist = False
                        break
                # check the OmniPhotos essential file
                openvslam_essential_file_exist = True
                for file_name in self.openvslam_essential_file_list:
                    if not file_name in file_list:
                        openvslam_essential_file_exist = False
                        break

                need_reconstruction = not (openvslam_output_file_exist or openvslam_essential_file_exist)
            else:
                need_reconstruction = True

            if not need_reconstruction:
                self.show_info("the openvslam essential data file exist in the folder {}"\
                    .format(str(self.output_directory_path_ovslam)), "info")
                return

            # check the openvslam program file
            if not os.path.exists(self.openvslam_program_path):
                raise FileExistsError("OpenVSLAM program not found at {}".format(self.openvslam_program_path))

            self.show_info("start OpenVSLAM reconstruction")
            # initial openvslam environment
            self.dir_make(self.output_directory_path_ovslam)

            # generate the openvslam config file
            replace_dict = {
                r'\$image_fps\$': str(self.frame_fps),
                r'\$image_width\$': str(self.frame_width),
                r'\$image_height\$': str(self.frame_height)}

            self.instance_template(self.openvslam_config_template_file,
                                   str(self.openvslam_config_file),
                                   replace_dict)

            # call openvslam reconstruction
            self.trajectory_reconstruct_openvslam(
                self.openvslam_program_path,
                self.openvslam_vocab_path,
                str(self.openvslam_config_file),
                str(self.traj_input_images_dir),
                str(self.output_directory_path_ovslam / "map.msg"),
                str(self.output_directory_path_ovslam / "frame_trajectory.txt"),
                self.config["preprocessing.openvslam.mapping_repeat_times"])

            self.show_info("Finish OpenVSLAM reconstruction")


    def trajectory_reconstruct_colmap(self, input_image_folder):
        """
        if the  original image is panoramic images, extract perspective images first.
        The middle perspective image storage as $root_directory$/colmap_perspective
        :param input_image_folder: reconstructed image input directory
        """
        # initial environment.
        colmap_database_dir = self.root_dir / "Capture" / "COLMAP"
        self.dir_make(colmap_database_dir)

        mapping_frame_interval = self.config["preprocessing.colmap.frame_interval"]

        # reconstruct the camera pose with colmap
        colmap2stepReconstruction.trajectory_reconstruct_colmap(
            colmap_bin=self.config["preprocessing.colmap.execution_file_path"],
            images_dir=input_image_folder,
            colmap_database_dir=str(colmap_database_dir),
            vocab_tree_path=self.config["preprocessing.colmap.vocab_file_path"],
            show_result_step_1=True,
            mapping_frame_interval=mapping_frame_interval)

    def trajectory_reconstruct_openvslam(self,
                                         openvslam_execution_path,
                                         openvslam_dbow2_path,
                                         openvslam_config_yaml_path,
                                         openvslam_input_images_path,
                                         openvslam_output_map_database_path,
                                         openvslam_output_trajectory_path,
                                         openvslam_mapping_repeat_times="5"):
        """
        reconstruct trajectory with openvslam
        :param openvslam_execution_path: the program file path
        :param openvslam_dbow2_path: dbow2 path
        :param openvslam_config_yaml_path: openvslam yaml config file path
        :param openvslam_input_images_path: input image directory path
        :param openvslam_output_map_database_path: output msg file path
        :param openvslam_output_trajectory_path: output trajectory.txt file path
        :param openvslam_mapping_repeat_times: the iteration time for reconstruction msg(map) file
        """
        self.show_info("start openvslam trajectory reconstruction")
        openvslam_parameters_list = [openvslam_execution_path]

        openvslam_parameters_list.append("--eval-log")
        openvslam_parameters_list.append("--auto-term")
        openvslam_parameters_list.append("--debug")
        openvslam_parameters_list.append("--no-sleep")

        openvslam_parameters_list.append("--frame-skip")  # input images folder
        openvslam_frame_skip = "1"
        openvslam_parameters_list.append(openvslam_frame_skip)

        openvslam_parameters_list.append("-v")  # dbow2
        openvslam_parameters_list.append(openvslam_dbow2_path)

        openvslam_parameters_list.append("-c")  # openvslam config.yaml
        openvslam_parameters_list.append(openvslam_config_yaml_path)

        openvslam_parameters_list.append("-i")  # input images folder
        openvslam_parameters_list.append(openvslam_input_images_path)

        openvslam_parameters_list.append("--map-db")  # input images folder
        openvslam_parameters_list.append(openvslam_output_map_database_path)

        # 1) reconstruct map
        msg = "Begin OpenVSLAM map reconstruction, the reconstruction time is {}." \
            .format(openvslam_mapping_repeat_times)
        self.show_info(msg)
        openvslam_mapping_params_list = copy.deepcopy(openvslam_parameters_list)

        openvslam_mapping_params_list.append("-t")  # run type mapping
        openvslam_mapping_params_list.append("map")

        openvslam_mapping_params_list.append("--repeat-times")  # run type mapping
        openvslam_mapping_params_list.append(str(openvslam_mapping_repeat_times))

        subprocess.run(openvslam_mapping_params_list, capture_output=True, check=True)

        # 2) reconstruct camera location
        msg = "Begin OpenVSLAM location."
        self.show_info(msg)
        openvslam_location_params_list = copy.deepcopy(openvslam_parameters_list)

        openvslam_location_params_list.append("-t")  # run type mapping
        openvslam_location_params_list.append("trajectory")

        openvslam_location_params_list.append("--trajectory_path")  # output trajectory path
        openvslam_location_params_list.append(openvslam_output_trajectory_path)

        subprocess.run(openvslam_location_params_list, check=True)

        # to wait the finish of openvslam output file writing
        time.sleep(5)
