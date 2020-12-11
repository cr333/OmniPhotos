# -*- coding: utf-8 -*-
"""
CLI class to output run-time info to the command line.
"""
from abs_ui import AbsUI


class ClUI(AbsUI):
    """
    CLI to show text log & information in the comand line.
    """
    def __init__(self):
        self.color_warning = '\033[93m'
        self.color_fail = '\033[91m'
        self.color_endc = '\033[0m'
        super(ClUI).__init__()

    def show_info(self, info_string, level="info"):
        """
        show info in console & UI
        :param info_string: information string
        :param level: the level of information, should be ["info", "error"]
        """
        if level == "info":
            print(info_string)
        elif level == "warning":
            print(self.color_warning + "!!!!!warning!!!!" + info_string + self.color_endc)
        elif level == "error":
            print(self.color_fail + "!!!!!error!!!!" + info_string + self.color_endc)
            raise Exception(info_string)
        else:
            print("level define error : {}".format(level))

    def show_image(self, image_data):
        """
        show image in a windows
        :param image_data: image data in numpy array
        :type image_data: numpy.array
        """
        # plt.imshow(image_data, interpolation='nearest')
        # plt.show()
        # return

    def show_question(self, msg):
        """
        :param msg: text information
        :return : information level
        """
        reply = str(input(msg + ' (y/n): ')).lower().strip()
        if reply[0] == 'y':
            return True
        if reply[0] == 'n':
            return False
