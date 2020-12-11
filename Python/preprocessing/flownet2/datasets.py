import torch
import torch.utils.data as data

import os, math, random
from os.path import *
import pathlib
from glob import glob

import numpy as np
from skimage.transform import resize
import csv

#from scipy.misc import imread, imresize
#from matplotlib.pyplot import imread

from .utils import frame_utils as frame_utils

class StaticRandomCrop(object):
    def __init__(self, image_size, crop_size):
        self.th, self.tw = crop_size
        h, w = image_size
        self.h1 = random.randint(0, h - self.th)
        self.w1 = random.randint(0, w - self.tw)

    def __call__(self, img):
        return img[self.h1:(self.h1+self.th), self.w1:(self.w1+self.tw),:]

class StaticCenterCrop(object):
    def __init__(self, image_size, crop_size):
        self.th, self.tw = crop_size
        self.h, self.w = image_size
    def __call__(self, img):
        return img[(self.h-self.th)//2:(self.h+self.th)//2, (self.w-self.tw)//2:(self.w+self.tw)//2,:]

class OmniPhotos_csv(data.Dataset):
    """
    OmniPhotos images loader, with the cache/xxx/Cameras.csv as input
    """

    def __init__(self, args, is_cropped=False, root='', dstype='clean', replicates=1):
        """
        :param args: the args from main.py
        :param is_cropped:
        :param root:
        ;param replicates:
        """
        self.args = args
        self.image_list_pair = [] # the input image pairs, [[image_path_pre.jpeg, img_cur.jpeg, image_path_next.jpeg],...]
        self.output_of_file_list = [] # output flow file name, [[xx-next.flo, xx-previous.flo],....]

        self.render_size = args.inference_size # the flownet2 input image size
        self.downsample_scalar = args.downsample_scalar
        self.frame_size = [] # the original image size
        self.frame_size_downsampled = [-1,-1]
        self.pad_size = []
        self.images_path = root

        # load csv file
        file_headler =  open(self.args.omniphotos_csv, 'r')
        csv_file_handle = csv.reader(file_headler, delimiter=',')
        csv_data = list(csv_file_handle)

        for idx in range(len(csv_data)):
            # import ipdb; ipdb.set_trace()
            image_path_pre = csv_data[(idx + len(csv_data) - 1) % len(csv_data)][22]
            image_path_next = csv_data[(idx + len(csv_data) + 1) % len(csv_data)][22]
            if not os.path.exists(os.path.join(self.images_path, image_path_pre)):
                print("error: image {} do not exist!".format(image_path_pre))
            if not os.path.exists(os.path.join(self.images_path, image_path_next)):
                print("error: image {} do not exist!".format(image_path_next))

            #self.image_list_pair.append([image_path_pre, csv_data[idx][22], image_path_next])
            self.image_list_pair.append([csv_data[idx][22], image_path_next]) # next
            self.image_list_pair.append([csv_data[idx][22], image_path_pre]) # previous

            # change the absolute path to relative path
            next_of_file_name = pathlib.Path(csv_data[idx][23]).name
            pre_of_file_name = pathlib.Path(csv_data[idx][24]).name
            self.output_of_file_list.append(next_of_file_name) # next
            self.output_of_file_list.append(pre_of_file_name) # previous

        self.size = len(self.image_list_pair) # the number of input image pairs

        self.frame_size.append(np.shape(frame_utils.read_gen(os.path.join(self.images_path,self.image_list_pair[0][0])))[0])
        self.frame_size.append(np.shape(frame_utils.read_gen(os.path.join(self.images_path,self.image_list_pair[0][0])))[1])

        # compute the input images size
        if (self.render_size[0] < 0 and self.render_size[1] < 0):
            if [-1.0, -1.0] != self.downsample_scalar:
                self.frame_size_downsampled[0] = int(self.frame_size[0] * self.downsample_scalar[0])
                self.frame_size_downsampled[1] = int(self.frame_size[1] * self.downsample_scalar[1])

            if  (self.render_size[0] % 64 or self.render_size[1] % 64):
                #self.render_size[0] = ((self.render_size[0]) // 64) * 64
                #self.render_size[1] = ((self.render_size[1]) // 64) * 64
                # compute the pad size and pad the input images
                self.render_size[0] = ((self.frame_size_downsampled[0] + 63) // 64) * 64
                self.render_size[1] = ((self.frame_size_downsampled[1] + 63) // 64) * 64
                self.pad_size.append(self.render_size[0] - self.frame_size_downsampled[0])
                self.pad_size.append(self.render_size[1] - self.frame_size_downsampled[1])

            args.inference_size = self.render_size
        else:
            self.frame_size_downsampled[0] = self.render_size[0]
            self.frame_size_downsampled[1] = self.render_size[1]

        print("network input image size is {} x {}".format(self.render_size[0],self.render_size[1]))

    def __getitem__(self, index):

        index = index % (self.size * 2)

        img1 = frame_utils.read_gen(os.path.join(self.images_path, self.image_list_pair[index][0]))
        img2 = frame_utils.read_gen(os.path.join(self.images_path, self.image_list_pair[index][1]))

        # the pad part is on the right and bottom of the image
        img1 = resize(img1, self.frame_size_downsampled, preserve_range=True)
        img2 = resize(img2, self.frame_size_downsampled, preserve_range=True)
        img1 = np.pad(img1, ((0, self.pad_size[0]), (0, self.pad_size[1]), (0,0)), 'reflect')
        img2 = np.pad(img2, ((0, self.pad_size[0]), (0, self.pad_size[1]), (0,0)), 'reflect')

        images = [img1, img2]
        flow = np.zeros((np.shape(img1)[0], np.shape(img1)[1], 2), np.float32)

        images = np.array(images).transpose(3,0,1,2)
        flow = flow.transpose(2,0,1)

        images = torch.from_numpy(images.astype(np.float32))
        flow = torch.from_numpy(flow.astype(np.float32))

        return [images], [flow]

    def __len__(self):
        return self.size


class OmniPhotos(data.Dataset):
    """
    OmniPhotos images loader
    """

    def __init__(self, args, is_cropped=False, root='', dstype='clean', replicates=1):
        """
        :param args: the args from main.py
        :param is_cropped:
        :param root:
        ;param replicates:
        """
        self.args = args
        self.render_size = args.inference_size # the flownet2 input image size
        self.frame_size = None # the original image size
        self.output_of_file_list = [] # output flow file name
        self.image_list = [] # the input image pairs
        self.size = None # the number of input image pairs
        self.downsample_scalar = args.downsample_scalar
        self.images_path = root

        file_list = glob(join(self.images_path, '*.png'))
        file_list = file_list + glob(join(self.images_path, '*.jpg'))
        file_list = file_list + glob(join(self.images_path, '*.jpeg'))
        sorted(file_list)

        for idx in range(len(file_list) - 1):
            img1 = join(self.images_path, file_list[idx])
            img2 = join(self.images_path, file_list[idx + 1])

            if not isfile(img1) or not isfile(img2):
                print("file do not exist {} {}".format(img1, img2))
                continue

            self.image_list += [[img1, img2]]
            self.image_list += [[img2, img1]]

            path, filename = os.path.split(file_list[idx])
            filename, ext = os.path.splitext(filename)

            self.output_of_file_list.append(filename + "-FlowToNext.flo")
            self.output_of_file_list.append(filename + "-FlowToPrevious.flo")

        self.size = len(self.image_list)
        self.frame_size = []
        self.frame_size.append(np.shape(frame_utils.read_gen(self.image_list[0][0]))[0])
        self.frame_size.append(np.shape(frame_utils.read_gen(self.image_list[0][0]))[1])

        # downsample the input images
        if (args.inference_size[0] < 0 and args.inference_size[1] < 0):
            if [-1.0, -1.0] != self.downsample_scalar:
                self.render_size[0] = int(self.frame_size[0] * self.downsample_scalar[0])
                self.render_size[1] = int(self.frame_size[1] * self.downsample_scalar[1])

            if  (self.render_size[0]%64 or self.render_size[1]%64):
                self.render_size[0] = ((self.render_size[0]) // 64) * 64
                self.render_size[1] = ((self.render_size[1]) // 64) * 64

            args.inference_size = self.render_size

        print("network input image size is {} x {}".format(self.render_size[0],self.render_size[1]))
        assert (len(self.image_list) == ((len(file_list)) - 1 ) * 2)

    def __getitem__(self, index):

        index = index % self.size

        img1 = frame_utils.read_gen(self.image_list[index][0])
        img2 = frame_utils.read_gen(self.image_list[index][1])

        img1 = resize(img1, self.render_size, preserve_range=True)
        img2 = resize(img2, self.render_size, preserve_range=True)

        images = [img1, img2]
        flow = np.zeros((np.shape(img1)[0], np.shape(img1)[1], 2), np.float32)

        images = np.array(images).transpose(3,0,1,2)
        flow = flow.transpose(2,0,1)

        images = torch.from_numpy(images.astype(np.float32))
        flow = torch.from_numpy(flow.astype(np.float32))

        return [images], [flow]

    def __len__(self):
        return self.size

class MpiSintel(data.Dataset):
    def __init__(self, args, is_cropped = False, root = '', dstype = 'clean', replicates = 1):
        self.args = args
        self.is_cropped = is_cropped
        self.crop_size = args.crop_size
        self.render_size = args.inference_size
        self.replicates = replicates

        flow_root = join(root, 'flow')
        image_root = join(root, dstype)

        file_list = sorted(glob(join(flow_root, '*/*.flo')))

        self.flow_list = []
        self.image_list = []

        for file in file_list:
            if 'test' in file:
                # print file
                continue

            fbase = file[len(flow_root)+1:]
            fprefix = fbase[:-8]
            fnum = int(fbase[-8:-4])

            img1 = join(image_root, fprefix + "%04d"%(fnum+0) + '.png')
            img2 = join(image_root, fprefix + "%04d"%(fnum+1) + '.png')

            if not isfile(img1) or not isfile(img2) or not isfile(file):
                continue

            self.image_list += [[img1, img2]]
            self.flow_list += [file]

        self.size = len(self.image_list)

        self.frame_size = frame_utils.read_gen(self.image_list[0][0]).shape

        if (self.render_size[0] < 0) or (self.render_size[1] < 0) or (self.frame_size[0]%64) or (self.frame_size[1]%64):
            self.render_size[0] = ( (self.frame_size[0])//64 ) * 64
            self.render_size[1] = ( (self.frame_size[1])//64 ) * 64

        args.inference_size = self.render_size

        assert (len(self.image_list) == len(self.flow_list))

    def __getitem__(self, index):

        index = index % self.size

        img1 = frame_utils.read_gen(self.image_list[index][0])
        img2 = frame_utils.read_gen(self.image_list[index][1])

        flow = frame_utils.read_gen(self.flow_list[index])

        images = [img1, img2]
        image_size = img1.shape[:2]

        if self.is_cropped:
            cropper = StaticRandomCrop(image_size, self.crop_size)
        else:
            cropper = StaticCenterCrop(image_size, self.render_size)
        images = list(map(cropper, images))
        flow = cropper(flow)

        images = np.array(images).transpose(3,0,1,2)
        flow = flow.transpose(2,0,1)

        images = torch.from_numpy(images.astype(np.float32))
        flow = torch.from_numpy(flow.astype(np.float32))

        return [images], [flow]

    def __len__(self):
        return self.size * self.replicates

class MpiSintelClean(MpiSintel):
    def __init__(self, args, is_cropped = False, root = '', replicates = 1):
        super(MpiSintelClean, self).__init__(args, is_cropped = is_cropped, root = root, dstype = 'clean', replicates = replicates)

class MpiSintelFinal(MpiSintel):
    def __init__(self, args, is_cropped = False, root = '', replicates = 1):
        super(MpiSintelFinal, self).__init__(args, is_cropped = is_cropped, root = root, dstype = 'final', replicates = replicates)

class FlyingChairs(data.Dataset):
  def __init__(self, args, is_cropped, root = '/path/to/FlyingChairs_release/data', replicates = 1):
    self.args = args
    self.is_cropped = is_cropped
    self.crop_size = args.crop_size
    self.render_size = args.inference_size
    self.replicates = replicates

    images = sorted( glob( join(root, '*.ppm') ) )

    self.flow_list = sorted( glob( join(root, '*.flo') ) )

    assert (len(images)//2 == len(self.flow_list))

    self.image_list = []
    for i in range(len(self.flow_list)):
        im1 = images[2*i]
        im2 = images[2*i + 1]
        self.image_list += [ [ im1, im2 ] ]

    assert len(self.image_list) == len(self.flow_list)

    self.size = len(self.image_list)

    self.frame_size = frame_utils.read_gen(self.image_list[0][0]).shape

    if (self.render_size[0] < 0) or (self.render_size[1] < 0) or (self.frame_size[0]%64) or (self.frame_size[1]%64):
        self.render_size[0] = ( (self.frame_size[0])//64 ) * 64
        self.render_size[1] = ( (self.frame_size[1])//64 ) * 64

    args.inference_size = self.render_size

  def __getitem__(self, index):
    index = index % self.size

    img1 = frame_utils.read_gen(self.image_list[index][0])
    img2 = frame_utils.read_gen(self.image_list[index][1])

    flow = frame_utils.read_gen(self.flow_list[index])

    images = [img1, img2]
    image_size = img1.shape[:2]
    if self.is_cropped:
        cropper = StaticRandomCrop(image_size, self.crop_size)
    else:
        cropper = StaticCenterCrop(image_size, self.render_size)
    images = list(map(cropper, images))
    flow = cropper(flow)


    images = np.array(images).transpose(3,0,1,2)
    flow = flow.transpose(2,0,1)

    images = torch.from_numpy(images.astype(np.float32))
    flow = torch.from_numpy(flow.astype(np.float32))

    return [images], [flow]

  def __len__(self):
    return self.size * self.replicates

class FlyingThings(data.Dataset):
  def __init__(self, args, is_cropped, root = '/path/to/flyingthings3d', dstype = 'frames_cleanpass', replicates = 1):
    self.args = args
    self.is_cropped = is_cropped
    self.crop_size = args.crop_size
    self.render_size = args.inference_size
    self.replicates = replicates

    image_dirs = sorted(glob(join(root, dstype, 'TRAIN/*/*')))
    image_dirs = sorted([join(f, 'left') for f in image_dirs] + [join(f, 'right') for f in image_dirs])

    flow_dirs = sorted(glob(join(root, 'optical_flow_flo_format/TRAIN/*/*')))
    flow_dirs = sorted([join(f, 'into_future/left') for f in flow_dirs] + [join(f, 'into_future/right') for f in flow_dirs])

    assert (len(image_dirs) == len(flow_dirs))

    self.image_list = []
    self.flow_list = []

    for idir, fdir in zip(image_dirs, flow_dirs):
        images = sorted( glob(join(idir, '*.png')) )
        flows = sorted( glob(join(fdir, '*.flo')) )
        for i in range(len(flows)):
            self.image_list += [ [ images[i], images[i+1] ] ]
            self.flow_list += [flows[i]]

    assert len(self.image_list) == len(self.flow_list)

    self.size = len(self.image_list)

    self.frame_size = frame_utils.read_gen(self.image_list[0][0]).shape

    if (self.render_size[0] < 0) or (self.render_size[1] < 0) or (self.frame_size[0]%64) or (self.frame_size[1]%64):
        self.render_size[0] = ( (self.frame_size[0])//64 ) * 64
        self.render_size[1] = ( (self.frame_size[1])//64 ) * 64

    args.inference_size = self.render_size

  def __getitem__(self, index):
    index = index % self.size

    img1 = frame_utils.read_gen(self.image_list[index][0])
    img2 = frame_utils.read_gen(self.image_list[index][1])

    flow = frame_utils.read_gen(self.flow_list[index])

    images = [img1, img2]
    image_size = img1.shape[:2]
    if self.is_cropped:
        cropper = StaticRandomCrop(image_size, self.crop_size)
    else:
        cropper = StaticCenterCrop(image_size, self.render_size)
    images = list(map(cropper, images))
    flow = cropper(flow)


    images = np.array(images).transpose(3,0,1,2)
    flow = flow.transpose(2,0,1)

    images = torch.from_numpy(images.astype(np.float32))
    flow = torch.from_numpy(flow.astype(np.float32))

    return [images], [flow]

  def __len__(self):
    return self.size * self.replicates

class FlyingThingsClean(FlyingThings):
    def __init__(self, args, is_cropped = False, root = '', replicates = 1):
        super(FlyingThingsClean, self).__init__(args, is_cropped = is_cropped, root = root, dstype = 'frames_cleanpass', replicates = replicates)

class FlyingThingsFinal(FlyingThings):
    def __init__(self, args, is_cropped = False, root = '', replicates = 1):
        super(FlyingThingsFinal, self).__init__(args, is_cropped = is_cropped, root = root, dstype = 'frames_finalpass', replicates = replicates)

class ChairsSDHom(data.Dataset):
  def __init__(self, args, is_cropped, root = '/path/to/chairssdhom/data', dstype = 'train', replicates = 1):
    self.args = args
    self.is_cropped = is_cropped
    self.crop_size = args.crop_size
    self.render_size = args.inference_size
    self.replicates = replicates

    image1 = sorted( glob( join(root, dstype, 't0/*.png') ) )
    image2 = sorted( glob( join(root, dstype, 't1/*.png') ) )
    self.flow_list = sorted( glob( join(root, dstype, 'flow/*.flo') ) )

    assert (len(image1) == len(self.flow_list))

    self.image_list = []
    for i in range(len(self.flow_list)):
        im1 = image1[i]
        im2 = image2[i]
        self.image_list += [ [ im1, im2 ] ]

    assert len(self.image_list) == len(self.flow_list)

    self.size = len(self.image_list)

    self.frame_size = frame_utils.read_gen(self.image_list[0][0]).shape

    if (self.render_size[0] < 0) or (self.render_size[1] < 0) or (self.frame_size[0]%64) or (self.frame_size[1]%64):
        self.render_size[0] = ( (self.frame_size[0])//64 ) * 64
        self.render_size[1] = ( (self.frame_size[1])//64 ) * 64

    args.inference_size = self.render_size

  def __getitem__(self, index):
    index = index % self.size

    img1 = frame_utils.read_gen(self.image_list[index][0])
    img2 = frame_utils.read_gen(self.image_list[index][1])

    flow = frame_utils.read_gen(self.flow_list[index])
    flow = flow[::-1,:,:]

    images = [img1, img2]
    image_size = img1.shape[:2]
    if self.is_cropped:
        cropper = StaticRandomCrop(image_size, self.crop_size)
    else:
        cropper = StaticCenterCrop(image_size, self.render_size)
    images = list(map(cropper, images))
    flow = cropper(flow)


    images = np.array(images).transpose(3,0,1,2)
    flow = flow.transpose(2,0,1)

    images = torch.from_numpy(images.astype(np.float32))
    flow = torch.from_numpy(flow.astype(np.float32))

    return [images], [flow]

  def __len__(self):
    return self.size * self.replicates

class ChairsSDHomTrain(ChairsSDHom):
    def __init__(self, args, is_cropped = False, root = '', replicates = 1):
        super(ChairsSDHomTrain, self).__init__(args, is_cropped = is_cropped, root = root, dstype = 'train', replicates = replicates)

class ChairsSDHomTest(ChairsSDHom):
    def __init__(self, args, is_cropped = False, root = '', replicates = 1):
        super(ChairsSDHomTest, self).__init__(args, is_cropped = is_cropped, root = root, dstype = 'test', replicates = replicates)

class ImagesFromFolder(data.Dataset):
  def __init__(self, args, is_cropped, root = '/path/to/frames/only/folder', iext = 'png', replicates = 1):
    self.args = args
    self.is_cropped = is_cropped
    self.crop_size = args.crop_size
    self.render_size = args.inference_size
    self.replicates = replicates

    images = sorted( glob( join(root, '*.' + iext) ) )
    self.image_list = []
    for i in range(len(images)-1):
        im1 = images[i]
        im2 = images[i+1]
        self.image_list += [ [ im1, im2 ] ]

    self.size = len(self.image_list)

    self.frame_size = frame_utils.read_gen(self.image_list[0][0]).shape

    if (self.render_size[0] < 0) or (self.render_size[1] < 0) or (self.frame_size[0]%64) or (self.frame_size[1]%64):
        self.render_size[0] = ( (self.frame_size[0])//64 ) * 64
        self.render_size[1] = ( (self.frame_size[1])//64 ) * 64

    args.inference_size = self.render_size

  def __getitem__(self, index):
    index = index % self.size

    img1 = frame_utils.read_gen(self.image_list[index][0])
    img2 = frame_utils.read_gen(self.image_list[index][1])

    images = [img1, img2]
    image_size = img1.shape[:2]
    if self.is_cropped:
        cropper = StaticRandomCrop(image_size, self.crop_size)
    else:
        cropper = StaticCenterCrop(image_size, self.render_size)
    images = list(map(cropper, images))
    
    images = np.array(images).transpose(3,0,1,2)
    images = torch.from_numpy(images.astype(np.float32))

    return [images], [torch.zeros(images.size()[0:1] + (2,) + images.size()[-2:])]

  def __len__(self):
    return self.size * self.replicates

'''
import argparse
import sys, os
import importlib
from scipy.misc import imsave
import numpy as np

import datasets
reload(datasets)

parser = argparse.ArgumentParser()
args = parser.parse_args()
args.inference_size = [1080, 1920]
args.crop_size = [384, 512]
args.effective_batch_size = 1

index = 500
v_dataset = datasets.MpiSintelClean(args, True, root='../MPI-Sintel/flow/training')
a, b = v_dataset[index]
im1 = a[0].numpy()[:,0,:,:].transpose(1,2,0)
im2 = a[0].numpy()[:,1,:,:].transpose(1,2,0)
imsave('./img1.png', im1)
imsave('./img2.png', im2)
flow_utils.writeFlow('./flow.flo', b[0].numpy().transpose(1,2,0))

'''
