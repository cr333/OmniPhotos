import csv
import numpy as np
import json
from skimage.feature import peak_local_max

class PointDict(list):
    def __init__(self, *args, **kwargs):
        super().__init__(*args,**kwargs)
        self.keys = []
        if len(self) > 0:
            self.keys=list(self[0].keys())
        self.arr = None
    def __str__(self):
        output = []
        if self.keys is None:
            self.keys = list(self[0].keys())
        headings = self.keys[0]
        for key in self.keys[1:]:
            headings += ' ' + key
        output.append(headings)
        for dct in self:
            lst = []
            for key in dct:
                lst.append(str(dct[key]))
            output.append(', '.join(lst))
        return '\n'.join(output)

    def toCSV(self, filename):
        self.keys = self[0].keys()
        with open(filename, 'w', newline='') as output_file:
            dict_writer = csv.DictWriter(output_file, self.keys)
            dict_writer.writeheader()
            dict_writer.writerows(self)

    def fromCSV(self,filename):
        """
        Warning: ICE
        :param filename:
        :return:
        """
        with open(filename,'r') as ifile:
            reader = csv.reader(ifile)
            lst = [row for row in reader]
        self.keys = lst[0]
        for enum,key in enumerate(self.keys):
            self.set(key,[row[enum] for row in lst[1:]])
    def toJSON(self,filename):
        with open(filename, 'w') as fout:
            json.dump(self, fout)
    def fromJSON(self,filename):
        with open(filename,'r') as fin:
            data = PointDict(json.load(fin))
            self.extend(data)
            self.keys = data.keys
    def get(self, key:str)->list:
        """
        will return a list of the specified error if found in
        :param key: one of errors in point dict (check self.keys)
        :return: lst
        """
        return [dct[key] for dct in self]

    def set(self, key:str, vals:list):
        """
        will assign the values given in vals to the keys in self
        :param key: interval,endpoint_error,...etc
        :param vals: list of vals (floats)
        :return: None
        """
        if key not in self.keys:
            self.keys.append(key)
        if len(self) == 0:
            for val in vals:
                if key == "interval" and type(val) == str:
                    val = val.replace("(",'')
                    val = val.replace(")",'')
                    self.append({key:tuple(map(int,val.split(', ')))})
                else:
                    self.append({key:float(val)})
            return
        for idx in range(len(self)):
            if key == "interval" and type(vals[idx]) == str:
                val = vals[idx].replace("(", '')
                val = val.replace(")", '')
                self.append({key: tuple(map(int, val.split(', ')))})
            else:
                self[idx][key]=float(vals[idx])

    def find_local_minima(self, numpoints=None, errors:list=None,save_sum=True):
        """
        :param numpoints: ~1k points in path. if None, will calculate it by iterating through the point dict and finding
        the max value in "intervals".
        :param errors: what errors to include
        :param save_sum: will save the sum under "summed_errors" in the list
        :return:
        """
        if numpoints is None:
            numpoints = 0
            for dct in self:
                numpoints = max((dct["interval"][0],dct['interval'][1],numpoints)) + 1
        if errors is None:
            errors = list(self.keys)[1:]
        lst = []
        for dct in self:
            lst.append(sum([dct[error] for error in errors]))
        if save_sum:
            self.set("summed_errors",lst)
        arr = np.zeros((numpoints, numpoints))
        for enum,dct in enumerate(self):
            arr[dct["interval"][0], dct["interval"][1]] = 1 / lst[enum]
        self.arr = arr
        coords = peak_local_max(arr, min_distance=10, threshold_rel=0.5)
        bool_arr = np.zeros(arr.shape)
        for coord in coords:
            bool_arr[coord[0],coord[1]] = 1
        intervals = []
        for dct in self:
            if bool_arr[dct["interval"][0], dct["interval"][1]]:
                intervals.append(dct)
        return PointDict(intervals)

class CameraCenters(list):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.orientations = []