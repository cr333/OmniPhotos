# -*- coding: utf-8 -*-
"""Define the abstract class of UI.
"""

from matplotlib import pyplot as plt


class AbsUI:
    """abstract class of UI.
    """

    def __init__(self):
        pass

    def show_info(self, info_string, level="info"):
        """
        show info in console & UI
        :param info_string: information string
        :param level: the level of information, should be ["info", "error"]
        """
        if level == "info":
            print(info_string)
            return
        if level == "warning":
            print("!!!!!warning!!!!" + info_string)
            return
        if level == "error":
            print("!!!!!error!!!!" + info_string)
            raise Exception(info_string)

        print(info_string)
        print("level define error : {}".format(level))

    def show_image(self, image_data):
        """
        show image in a windows
        :param image_data: image data in numpy array
        :type image_data: numpy.array
        """
        plt.imshow(image_data, interpolation='nearest')
        plt.show()

    def show_question(self, msg):
        """
        :param msg: text information
        :return : information level
        """
        return False 

    def run(self):
        """
        run the main loop of ui
        """
