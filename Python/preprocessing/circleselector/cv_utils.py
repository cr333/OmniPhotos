import os

import cv2
import numpy as np
from matplotlib import pyplot as plt
from skimage.metrics import structural_similarity

import flownet2.utils.computeColor as computeColor


def resize(img, scale_percent, verbose=False):
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
    flow[:, :, 0] += np.arange(w)
    flow[:, :, 1] += np.arange(h)[:, np.newaxis]
    res = cv2.remap(img, flow, None, cv2.INTER_LINEAR, borderMode=cv2.BORDER_REPLICATE)
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


def warp_images(img1, img2, savedir: str = None, look_at_angle: float = 0):
    # pad image on which flow will be calculated
    padding = 180
    img1 = slice_eqimage(img1, look_at_angle, padding=padding)
    img2 = slice_eqimage(img2, look_at_angle, padding=padding)

    output_im1 = resize(img1, 50)
    output_im2 = resize(img2, 50)

    resized1 = resize(img1, 25)
    resized2 = resize(img2, 25)

    # cast image to greyscale
    prvs = cv2.cvtColor(resized1, cv2.COLOR_BGR2GRAY)
    next = cv2.cvtColor(resized2, cv2.COLOR_BGR2GRAY)
    dis = cv2.DISOpticalFlow_create()

    # calculate forward flow
    flow_forward = dis.calc(prvs, next, None)
    flow_forward = cv2.resize(flow_forward, (output_im1.shape[1], output_im1.shape[0]),
                              interpolation=cv2.INTER_LINEAR) * 2
    # calculate backward flow
    flow_backward = dis.calc(next, prvs, None)
    flow_backward = cv2.resize(flow_backward, (output_im1.shape[1], output_im1.shape[0]),
                               interpolation=cv2.INTER_LINEAR) * 2

    next_img = warp_flow(output_im1, flow_forward)
    prvs_img = warp_flow(output_im2, flow_backward)

    # unpad the images
    unpadded_next = next_img[:, padding // 2:next_img.shape[1] - padding // 2]
    unpadded_prvs = prvs_img[:, padding // 2:prvs_img.shape[1] - padding // 2]

    if savedir is not None:
        plt.imsave(os.path.join(savedir, "forward_flow.jpg"),
                   computeColor.computeImg(flow_forward)[:, padding:next_img.shape[1] - (padding)])
        plt.imsave(os.path.join(savedir, "backward_flow.jpg"),
                   computeColor.computeImg(flow_backward)[:, padding:next_img.shape[1] - (padding)])
        plt.imsave(os.path.join(savedir, "2_output2.jpg"), np.flip(unpadded_prvs, axis=2))
        plt.imsave(os.path.join(savedir, "4_output1.jpg"), np.flip(unpadded_next, axis=2))
        plt.imsave(os.path.join(savedir, "1_input_img1.jpg"),
                   np.flip(output_im1[:, padding // 2:next_img.shape[1] - padding // 2], axis=2))
        plt.imsave(os.path.join(savedir, "3_input_img2.jpg"),
                   np.flip(output_im2[:, padding // 2:prvs_img.shape[1] - padding // 2], axis=2))
    return unpadded_next, unpadded_prvs


def slice_eqimage(img: np.array, look_at_angle: float, padding: int = 0):
    """

    :param img: equirectangular image
    :param look_at_angle: radians
    :param padding: number of indicies to add to each end of the eq image slicing. (e.g 60)
    :return: img hemisphere, centered at lookatang
    """
    hemisphere_width = img.shape[1] // 4
    padded_img = np.hstack((img, img, img))
    # 3 stacked equirectangular images leads to a total angle of 6 pi. The equirectangular images are stacked
    # for when we are looking at the outer edge of the equirectangular image. (where the wrap around occurs).
    lookatindex = round(padded_img.shape[1] * (3 * np.pi + look_at_angle) / (6 * np.pi))
    lower = lookatindex - hemisphere_width - padding
    upper = lookatindex + hemisphere_width + padding
    return padded_img[:, lower:upper]


def calculate_metrics(interval: tuple, dataset_path: str, savedir: str = None, rel_input_image_path='Input',
                      look_at_angle: float = 0) -> tuple:
    """

    :param interval: tuple containing interaval metrics are being calculated for
    :param dataset_path: path to dataset folder (e.g path/to/GenoaCathedral)
    :param rel_input_image_path: input image path relative to dataset_path
    :param savedir: will save OF output to savedir if not None
    :param look_at_angle: direction relative to center in radians
    :return: ssim, psnr
    """

    # read in the images according to the indexes in the interval
    # NOTE: the code assumes all the images are listed, in order, in the image directory.
    input_path = os.path.join(dataset_path, rel_input_image_path)
    images = os.listdir(input_path)

    # remove any file that are not images from the list.
    for filename in images:
        if os.path.splitext(filename)[-1] not in [".png", ".jpg"]:
            images.remove(filename)
    path1 = os.path.join(input_path, images[interval[0]])
    path2 = os.path.join(input_path, images[interval[1]])
    img1 = cv2.imread(path1, 1)
    img2 = cv2.imread(path2, 1)

    for enum, array in enumerate([img1, img2]):
        if not array.size:
            raise FileNotFoundError("array was empty for " + [path1, path2][enum])

    remap1, remap2 = warp_images(img1, img2, savedir, look_at_angle)
    img1 = slice_eqimage(resize(img1, 50), look_at_angle)
    img2 = slice_eqimage(resize(img2, 50), look_at_angle)

    # crop poles to remove distortions
    remap1 = crop_poles(remap1)
    remap2 = crop_poles(remap2)
    img1 = crop_poles(img1)
    img2 = crop_poles(img2)

    ssim = (calculate_ssim(img1, remap2) + calculate_ssim(img2, remap1)) / 2
    psnr = (calculate_psnr(img1, remap2) + calculate_psnr(img2, remap1)) / 2

    return ssim, psnr


def calculate_metrics_lst(point_dicts: [dict], dataset_path: str) -> [dict]:
    """

    :param point_dicts: list of intervals wanting to calcuate metrics on
    :param dataset_path: path to dataset folder (e.g path/to/GenoaCathedral)
    """
    for enum, dct in enumerate(point_dicts):
        print(enum, "/", len(point_dicts))
        ssim, psnr = calculate_metrics(dct["interval"], dataset_path)
        dct["inv_ssim"], dct["inv_psnr"] = 1 / ssim, 1 / psnr
    return point_dicts


def crop_poles(img):
    margin = round(0.05 * img.shape[0])
    return img[margin:img.shape[0] - margin]
