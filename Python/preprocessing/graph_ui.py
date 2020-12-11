# -*- coding: utf-8 -*-
"""define GraphUI class
"""
import tkinter
import tkinter.scrolledtext as tkst
import tkinter.messagebox

from PIL import Image, ImageTk
import numpy as np

from abs_ui import AbsUI


class GraphUI(AbsUI):
    """
    GUI to show text log & text information
    """

    def __init__(self, title="GUI"):
        """
        initialization
        :param title:
        """
        super().__init__()

        self.window_height = 800
        self.window_width = 1000

        # initial interface
        self.root = tkinter.Tk()
        self.root.geometry("{}x{}".format(
            self.window_width, self.window_height))
        self.root.title(title)  # changing the title

        self.frame = tkinter.Frame(self.root)

        # image size
        self.image_label_height = 495
        self.image_label_width = self.window_width - 10

        self.text_scrolledtext = None
        self.top_menu = None
        self.image_label = None
        self.init_window()

    def init_window(self):
        """
        Creation of init_window
        """
        # frame take the full root window
        self.frame.pack(fill=tkinter.BOTH, expand=1)

        # creating the top menu
        self.top_menu = tkinter.Menu(self.root)
        self.root.config(menu=self.top_menu)
        menu_file = tkinter.Menu(self.top_menu)  # add exit menu
        self.top_menu.add_cascade(label="File", menu=menu_file)
        menu_file.add_command(label="Exit", command=self.window_exit)

        # add label to display image in frame
        empty_image_data = np.zeros(
            [self.image_label_height, self.image_label_width, 3], dtype=np.uint8)
        empty_image_data.fill(156)
        image = Image.fromarray(obj=empty_image_data, mode='RGB')
        image_tk = ImageTk.PhotoImage(image=image)
        self.image_label = tkinter.Label(
            self.frame, height=self.image_label_height, \
                width=self.image_label_width, image=image_tk)
        # keep a reference to the PhotoImage object
        self.image_label.image_ref = image_tk
        self.image_label.pack(side=tkinter.TOP)

        # add text label to show run-time info in frame
        self.text_scrolledtext = tkst.ScrolledText(
            self.frame, wrap=tkinter.WORD, width=120, height=20)
        self.text_scrolledtext.yview(tkinter.END)
        self.text_scrolledtext.pack(side=tkinter.LEFT)
        # set text color for different level
        self.text_scrolledtext.tag_config('INFO', foreground='black')
        self.text_scrolledtext.tag_config('DEBUG', foreground='gray')
        self.text_scrolledtext.tag_config('WARNING', foreground='orange')
        self.text_scrolledtext.tag_config('ERROR', foreground='red')

    def before_exit(self):
        """
        exit the whole app
        :return:
        """
        self.show_info("close the gui windos")

    def run(self):
        """
        create GUI main loop
        """
        self.root.protocol("WM_DELETE_WINDOW", self.window_exit)
        self.root.mainloop()

    def window_exit(self):
        """
        terminate all thread & process
        """
        self.before_exit()

    def show_image(self, image_data):
        """
        show image on
        :param image_data: image data
        :type image_data: numpy
        """
        image_data_height = int(self.image_label_height)
        image_data_width = int(
            self.image_label_height * (float(np.shape(image_data)[1]) / np.shape(image_data)[0]))
        image = Image.fromarray(obj=image_data, mode='RGB').resize(
            (image_data_width, image_data_height))
        image_tk = ImageTk.PhotoImage(image=image)

        self.image_label.configure(image=image_tk)
        self.image_label.image = image_tk

    def show_info(self, info_string, level="info"):
        """
        :param info_string: text information
        :param level: information level, 'info', 'error', 'warning'
        """
        if level == "info":
            info_level = "INFO"
            info_string = info_string
        elif level == "error":
            info_level = "ERROR"
            info_string = "[ERROR]: " + info_string
        elif level == "warning":
            info_level = "WARNING"
            info_string = "[WARNING]: " + info_string
        else:
            raise ValueError("the show_info's message level error {}".format(level))

        # append the mesage to textpad and scrolled to bottom
        self.text_scrolledtext.insert(tkinter.END, info_string + '\n', info_level)
        self.text_scrolledtext.see(tkinter.END)

        # raise error 
        if level == "error":
            raise RuntimeError(info_string)

    def show_question(self, msg):
        """
        :param msg: text information
        :return : information level
        """
        answer_str = tkinter.messagebox.askquestion (title='warning',message=msg ,icon = 'warning')
        if answer_str == "yes":
            return True
        elif answer_str == "no":
            return False
        else:
            ValueError("question box return error value")
