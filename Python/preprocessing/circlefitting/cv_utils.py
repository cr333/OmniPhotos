import numpy as np
import cv2
import circlefitting.computeColor as computeColor
import os
from os.path import join as pjoin
from skimage.metrics import structural_similarity
from matplotlib import pyplot as plt

savedir = "results\\flow_remap"



def resize(img,scale_percent,verbose=False):
    if verbose:
        print('Original Dimensions : ', img.shape)
    width = int(img.shape[1] * scale_percent / 100)
    height = int(img.shape[0] * scale_percent / 100)
    dim = (width, height)

    # resize image
    resized = cv2.resize(img, dim, interpolation=cv2.INTER_AREA)
    if verbose:
        print('Resized Dimensions : ', resized.shape)
    return resized

# remap images using calculated flows
def warp_flow(img, flow):
    h, w = flow.shape[:2]
    flow = -flow
    flow[:,:,0] += np.arange(w)
    flow[:,:,1] += np.arange(h)[:,np.newaxis]
    res = cv2.remap(img, flow, None, cv2.INTER_LINEAR,borderMode=cv2.BORDER_REPLICATE)
    return res


def calculate_psnr(img1, img2, max_value=255):
    """"Calculating peak signal-to-noise ratio (PSNR) between two images."""
    mse = np.mean((np.array(img1, dtype=np.float32) - np.array(img2, dtype=np.float32)) ** 2)
    if mse == 0:
        return 100
    return 20 * np.log10(max_value / (np.sqrt(mse)))

# calculate_psnr(gt_img, ours_img)


def calculate_ssim(img1, img2):
    return structural_similarity(img1, img2, multichannel=True)

def warp_images(img1,img2,savedir:str=None,lookatang:float=0):
    # pad image on which flow will be calculated
    padding = 180
    img1 = slice_eqimage(img1,lookatang,padding=padding)
    img2 = slice_eqimage(img2,lookatang,padding=padding)

    output_im1 = resize(img1, 50)
    output_im2 = resize(img2, 50)

    resized1 = resize(img1, 25)
    resized2 = resize(img2, 25)

    # cast image to greyscale
    prvs = cv2.cvtColor(resized1, cv2.COLOR_BGR2GRAY)
    next = cv2.cvtColor(resized2, cv2.COLOR_BGR2GRAY)
    dis = cv2.DISOpticalFlow_create()  # what the fuck is this syntax?

    # calculate forward flow
    flow_forward = dis.calc(prvs, next, None)  # why does this function not specify its RETURN VALUE
    # remove padding
    flow_forward = cv2.resize(flow_forward, (output_im1.shape[1], output_im1.shape[0]),
                              interpolation=cv2.INTER_LINEAR) * 2
    # calculate backward flow
    flow_backward = dis.calc(next, prvs, None)
    flow_backward = cv2.resize(flow_backward, (output_im1.shape[1], output_im1.shape[0]),
                               interpolation=cv2.INTER_LINEAR) * 2

    nxt_img = warp_flow(output_im1, flow_forward)
    prvs_img = warp_flow(output_im2, flow_backward)

    # unpad the images
    unpadded_nxt = nxt_img[:,padding//2:nxt_img.shape[1]-padding//2]
    unpadded_prvs = prvs_img[:,padding//2:prvs_img.shape[1]-padding//2]

    if savedir is not None:
        plt.imsave(pjoin(savedir, "forward_flow.jpg"),
                   computeColor.computeImg(flow_forward)[:, padding:nxt_img.shape[1]-(padding)])
        plt.imsave(pjoin(savedir, "backward_flow.jpg"),
                   computeColor.computeImg(flow_backward)[:, padding:nxt_img.shape[1]-(padding)])
        plt.imsave(pjoin(savedir, "2_output2.jpg"), np.flip(unpadded_prvs,axis=2))
        plt.imsave(pjoin(savedir, "4_output1.jpg"), np.flip(unpadded_nxt, axis=2))
        plt.imsave(pjoin(savedir, "1_input_img1.jpg"),
                   np.flip(output_im1[:,padding//2:nxt_img.shape[1]-padding//2], axis=2))
        plt.imsave(pjoin(savedir, "3_input_img2.jpg"),
                   np.flip(output_im2[:,padding//2:prvs_img.shape[1]-padding//2], axis=2))
    return unpadded_nxt, unpadded_prvs
def slice_eqimage(img:np.array,lookatang:float,padding:int=0):
    """

    :param img: equirectangular image
    :param lookatang:
    :param padding: number of indicies to add to each end of the eq image slicing. (e.g 60)
    :return: img hemisphere, centered at lookatang
    """
    hemisphere_width = img.shape[1]//4
    padded_img = np.hstack((img,img,img))
    lookatindex = round(padded_img.shape[1] * (3 * np.pi + lookatang)/(6*np.pi))
    lower = lookatindex-hemisphere_width-padding
    upper = lookatindex+hemisphere_width+padding
    return padded_img[:,lower:upper]

def calculate_metrics(interval:tuple, dataset_path:str,savedir:str=None, lookatang:float = 0) -> tuple:
    """

    :param interval: tuple containing interaval metrics are being calculated for
    :param dataset_path: path to dataset folder (e.g path/to/GenoaCathedral)
    :param savedir: will save OF output to savedir if not None
    :return: ssim, psnr
    """

    input_path = pjoin(dataset_path,"Input")
    images = os.listdir(input_path)
    for filename in images:
        if os.path.splitext(filename)[-1] not in [".png",".jpg"]:
            images.remove(filename)
    path1 = pjoin(input_path,images[interval[0]])
    path2 = pjoin(input_path,images[interval[1]])
    img1 = cv2.imread(path1,1)
    img2 = cv2.imread(path2,1)

    for enum,array in enumerate([img1,img2]):
        if not array.size:
            raise FileNotFoundError("array was empty for " + [path1,path2][enum])

    remap1, remap2 = warp_images(img1, img2,savedir,lookatang)
    img1, img2 = slice_eqimage(resize(img1, 50),lookatang), slice_eqimage(resize(img2, 50),lookatang)
    remap1,remap2,img1,img2 = crop_poles(remap1),crop_poles(remap2),crop_poles(img1),crop_poles(img2)
    # plt.imshow(np.flip(img1,axis=2))
    # plt.show()
    ssim = (calculate_ssim(img1,remap2) + calculate_ssim(img2,remap1))/2
    psnr = (calculate_psnr(img1,remap2) + calculate_psnr(img2,remap1))/2

    return ssim, psnr
def calculate_metrics_lst(point_dicts:[dict],dataset_path:str) -> [dict]:
    """

    :param point_dicts: list of intervals wanting to calcuate metrics on
    :param dataset_path: path to dataset folder (e.g path/to/GenoaCathedral)
    """
    for enum,dct in enumerate(point_dicts):
        print(enum,"/",len(point_dicts))
        ssim,psnr = calculate_metrics(dct["interval"],dataset_path)
        dct["inv_ssim"], dct["inv_psnr"] = 1/ssim, 1/psnr
    return point_dicts

def slice_eq_unittest():
    """
    TODO: impletement unittests
    :return:
    """
    dimx = 1920
    dimy = 1920
    im_half1 = np.stack((np.ones((dimx,dimy)),np.zeros((dimx,dimy)),np.zeros((dimx,dimy))),axis=2)
    im_half2 = np.stack((np.zeros((dimx,dimy)),np.zeros((dimx,dimy)),np.zeros((dimx,dimy))),axis=2)
    img = np.hstack((im_half1,im_half2))
    sliced_img = slice_eqimage(img,np.pi/8)
    plt.imshow(sliced_img)
    plt.show()
    plt.close()
def crop_poles(img):
    margin = round(0.05*img.shape[0])
    return img[margin:img.shape[0]-margin]