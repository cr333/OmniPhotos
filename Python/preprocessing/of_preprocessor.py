# -*- coding: utf-8 -*-
"""
Implement optical flow computation with FlowNet2.
"""
import os
import shutil

from abs_preprocessor import AbsPreprocessor

class OfPreprocessor(AbsPreprocessor):
    """
    Implements optical flow computation with\
         flownet2-pytorch code (https://github.com/NVIDIA/flownet2-pytorch)
    """

    def __init__(self, args):
        """
        load all configuration and set-up runtime environment (directories)
        :param args: parameter from CLI
        """
        super().__init__(args)
        # generate the COMMAND line input string
        self.flownet_args = None  # the options arguments for flownet2-pytorch

        self.cache_data_root = self.root_dir / "Cache"
        self.dir_make(self.cache_data_root)
        self.save_visualization = self.config["preprocessing.of.save_visualization"]

        # get of method configuration
        self.of_method_list = ["flownet2"]
        self.of_method = self.config["preprocessing.of.method"]
        if not self.of_method in self.of_method_list:
            self.show_info("{} optical flow error".format(self.of_method_list), "error")

        # flownet2 parameters
        self.inference_dataset = "OmniPhotos_csv"
        self.of_save_path = self.cache_data_root / "flownet2-opticalflow"
        self.dir_make(self.of_save_path)

        self.of_downsample_scalar = self.config["preprocessing.of.downsample_scalar"]

        # get the path of mp cache folder and flownet2 folder
        self.cache_op_dis_flow_dir = None
        self.cache_op_flownet2_flow_dir = None


    def compute_of(self):
        """
        compute optical flow for OmniPhotos
        """
        # 0) set-up run-time variables
        from flownet2 import flownet2
        for idx, dir_term in enumerate(os.listdir(str(self.cache_data_root))):
            if not os.path.isdir(str(self.cache_data_root / dir_term)):
                continue
            if idx == 0:
                self.cache_op_dis_flow_dir = self.cache_data_root / dir_term
            if "-flownet2" in str(dir_term).lower():
                self.cache_op_flownet2_flow_dir = self.cache_data_root / dir_term

        # 1) show GPU resource information
        gpu_infor_str = flownet2.get_gpu_resource()
        self.show_info(gpu_infor_str)

        # 2) comput the optical flow with flownet2
        # initial flownet2-pytorch
        self.flownet2_init_run_time()
        flownet2.flownet2_compute_of(\
            self.flownet_args, self.show_infor_interval, self.show_image, self.show_info)

        # 3) convert the folder layout as OmniPhotos request
        self.flownet2_omniphotos()


    def flownet2_omniphotos(self):
        """
        clean the flownet2 output and make it fit to OmniPhotos request
        """
        self.show_info("move the flow data from {} to {}"\
            .format(str(self.of_save_path), str(self.cache_op_flownet2_flow_dir)))

        # move all *.flo and *.png files to OmniPhotos folder
        for item in self.of_save_path.iterdir():
            src_file_path = self.of_save_path / item.name
            tar_file_path = self.cache_op_flownet2_flow_dir / item.name
            if src_file_path.suffix == ".png" or src_file_path.suffix == ".flo" \
                or src_file_path.suffix == ".jpeg" or src_file_path.suffix == ".jpg" \
                or src_file_path.suffix == ".floss":
                shutil.move(str(src_file_path), str(tar_file_path))

        # remove the flownet2 output folder
        #shutil.rmtree(str(self.of_save_path))


    def flownet2_init_run_time(self):
        """
        set-up gpu run time environments for flownet2-python
        :param trajectory_tool:
        """
        # check and create run-time files & folder
        if self.cache_op_dis_flow_dir is None:
            self.show_info("there are not cache data for OmniPhotos in Cache folder,\
                 please check the OmniPhotos preprocess program", "error")
        if self.cache_op_flownet2_flow_dir is None:
            self.show_info("there are not cache data for flownet in Cache folder,\
                 please check the step 3", "error")

        self.dir_make(self.cache_op_flownet2_flow_dir)

        # check OmniPhotos generated Cameras.csv
        csv_path = self.cache_op_flownet2_flow_dir / "Cameras.csv"
        if not csv_path.exists():
            self.show_info("the {} not exist, please run the OmniPhotos \
                preprocessing program first.".format(str(csv_path)), "error")

        # check flownet2 model
        flownet2_model_list = ["FlowNet2", "FlowNet2C", "FlowNet2S",\
            "FlowNet2SD", "FlowNet2CS", "FlowNet2CSS"]
        flownet2_model_name = self.config["preprocessing.of.flownet2.model_name"]
        if not flownet2_model_name in flownet2_model_list:
            self.show_info("load flownet2 model {} error".format(flownet2_model_name), "error")

        # set the the parameter for flownet2-pytorch
        self.flownet_args = ["--inference",
                             "--model", flownet2_model_name,
                             "--save_flow",
                             "--inference_dataset", self.inference_dataset,
                             "--inference_dataset_root", str(self.cache_op_flownet2_flow_dir),
                             "--omniphotos_csv", str(csv_path),
                             "--batch_size", "1",
                             "--save", str(self.of_save_path),
                             "--resume", self.config["preprocessing.of.flownet2.model_path"],
                             "--number_workers", "1",
                             "--downsample_scalar", str(self.of_downsample_scalar[0]),\
                                                    str(self.of_downsample_scalar[1])]

        # torch.cuda.set_device(1)
