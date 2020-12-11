# -*- coding: utf-8 -*-
"""
The preprocessing main module.
"""
import threading
import datetime
import sys

from abs_preprocessor import AbsPreprocessor
from data_preprocessor import DataPreprocessor
from traj_preprocessor import TrajPreprocessor
from op_preprocessor import OpPreprocessor
from of_preprocessor import OfPreprocessor
from graph_ui import GraphUI
#from cl_ui import ClUI


class PreprocAPP:
    """
    The preprocessing class to manage the interface (GUI, CLI) and methods.
    """

    def __init__(self, args):
        """
        initial function of appPreprocessing
        :param args: parameter from CLI
        """
        self.args = args
        self.time_recoder = datetime.datetime.now()
        self.method_thread = None

        #self.interface = ClUI()
        self.interface = GraphUI("OmniPhotos Preprocessing")
        # register the callback function
        self.interface.before_exit = self.callback_exit
        AbsPreprocessor.abs_ui = self.interface  # register UI

        self.data_preproc = DataPreprocessor(args)
        self.traj_preproc = TrajPreprocessor(args)
        self.op_preproc = OpPreprocessor(args)
        self.of_preproc = OfPreprocessor(args)

    def callback_exit(self):
        """
        exit the program
        :return:
        """
        sys.exit(0)

    def show_time_now(self):
        """
        show the time now on interface, with format "yyyy-mm-dd, hh-mm-ss"
        """
        datetime_now = datetime.datetime.now()
        time_delta = datetime_now - self.time_recoder
        msg = datetime.datetime.now().isoformat() + " elapsed from the previous record: " + str(
            time_delta.total_seconds()) + "seconds"
        self.interface.show_info(msg)
        self.time_recoder = datetime_now

    def method(self):
        """
        the preprocessing core method
        """
        self.show_time_now()

        step_begin = self.data_preproc.config["preprocessing.step_start"]
        step_end = self.data_preproc.config["preprocessing.step_end"]
        if step_end == -1:
            step_end = 4

        preprocessor_flow = {
            1: ["preprocessing the data with rotation, extraction and saving",\
                self.data_preproc.preprocess_data],
            2: ["reconstructing camera trajectory ", self.traj_preproc.trajectory_reconstruct],
            3: ["generate config file for OmniPhotos", self.op_preproc.generating_config_files],
            4: ["computing optical flow", self.of_preproc.compute_of]}
        for step_idx, step_data in preprocessor_flow.items():
            self.interface.show_info("----step {}: {}----".format(step_idx, step_data[0]))
            if step_begin <= step_idx <= step_end:
                self.interface.show_info("run")
                self.show_time_now()
                step_data[1]()
                self.show_time_now()
                self.interface.show_info("----step {}: finished".format(step_idx))
            else:
                self.interface.show_info("--skip")

    def run(self):
        """
        main loop
        """
        # create preprocessing thread
        self.method_thread = threading.Thread(target=self.method)
        self.method_thread.daemon = True  # for terminate the thread
        self.method_thread.start()

        # create UI main loop
        self.interface.run()
        self.method_thread.join()
