import unittest
from circleselector.datatypes import PointDict
import os

class UTestPreprocessing(unittest.TestCase):
    """
    Unit tests for datatypes module in circleselector.
    """
    def setUp(self) -> None:
        self.data = PointDict([{'interval': [0, 100],
                                'flatness_error': 0.1}])
        self.save_path = "PointDict.json"

    def test_splitinterval(self):
        expected = PointDict([{'interval_start':0,
                               'interval_end':100,
                               'flatness_error':0.1}])
        self.assertEqual(expected,self.data.split_interval())

    def test_get(self):
        self.assertEqual([0.1],self.data.get('flatness_error'))

    def test_set(self):
        expected = PointDict([{'interval': [0, 100],
                               'flatness_error': 0.1,
                               'endpoint_error':0.9}])

        # take copy of data so as not to mutate the data for later tests
        data_copy = PointDict(self.data.copy())
        data_copy.set('endpoint_error',[0.9])

        self.assertEqual(expected,data_copy)

    def test_jsonio(self):
        self.data.toJSON(self.save_path)
        expected = ['[{"interval": [0, 100], "flatness_error": 0.1}]']
        with open(self.save_path,"r") as ifile:
            string = ifile.readlines()
        self.assertEqual(expected,string)

    def test_keyword_loading(self):

        # need to generate the data again
        self.data.toJSON(self.save_path)

        # generate the object
        expected = PointDict()
        # update it using the data we just generated
        expected.fromJSON(self.save_path)
        self.assertEqual(expected,PointDict(from_file=self.save_path))

    def tearDown(self) -> None:
        if os.path.isfile(self.save_path):
            os.remove(self.save_path)

if __name__ == '__main__':
    unittest.main()