# -*- coding: utf-8 -*-

from tkinter import *
import unittest

# add parent directory to path
import os,sys,inspect
current_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parent_dir = os.path.dirname(current_dir)
sys.path.insert(0, parent_dir)

import gui

class TestPreprocessor(unittest.TestCase):

    def test_model_converter(self):
        #create window
        root = Tk()
        root.geometry("800x600")
        self.app = gui.Window(root, "hello world")
        self.app.show_text("hello world")
        root.mainloop()

if __name__ == '__main__':
    unittest.main()
