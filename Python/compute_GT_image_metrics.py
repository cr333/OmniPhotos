#!/usr/bin/env python
# coding: utf-8

# get_ipython().run_line_magic('pylab', 'inline')
import csv
from math import sqrt

import imageio
import numpy as np
import os
# import cv2
from skimage.metrics import structural_similarity


def calculate_psnr(img1, img2, max_value=255):
    """"Calculating peak signal-to-noise ratio (PSNR) between two images."""
    mse = np.mean((np.array(img1, dtype=np.float32) - np.array(img2, dtype=np.float32)) ** 2)
    if mse == 0:
        return 100
    return 20 * np.log10(max_value / (np.sqrt(mse)))

# calculate_psnr(gt_img, ours_img)


def calculate_ssim(img1, img2):
    return structural_similarity(img1, img2, multichannel=True)


def calculate_lpips(img1, img2):
    global lpips_model, use_gpu

    img1 = util.im2tensor(img1)
    img2 = util.im2tensor(img2)

    if use_gpu:
        img1 = img1.cuda()
        img2 = img2.cuda()

    return lpips_model.forward(img1, img2).item()


def max_shifted_metric(function, img1, img2, max_shift=1):
    max_value = 0

    for y_shift in range(-max_shift, max_shift + 1):
        y_start = max(0, y_shift)
        y_end = min(img1.shape[0], img1.shape[0] + y_shift)
        for x_shift in range(-max_shift, max_shift + 1):
            x_start = max(0, x_shift)
            x_end = min(img1.shape[1], img1.shape[1] + x_shift)

            img1_shifted = img1[y_start:y_end, x_start:x_end]
            img2_cropped = img2[:img1_shifted.shape[0], :img1_shifted.shape[1]]

            value = function(img1_shifted, img2_cropped)
            #             print(y_shift, x_shift, value)
            if value > max_value:
                max_value = value

    return max_value


def min_shifted_metric(function, img1, img2, min_shift=1):
    min_value = 1e10

    for y_shift in range(-min_shift, min_shift + 1):
        y_start = max(0, y_shift)
        y_end = min(img1.shape[0], img1.shape[0] + y_shift)
        for x_shift in range(-min_shift, min_shift + 1):
            x_start = max(0, x_shift)
            x_end = min(img1.shape[1], img1.shape[1] + x_shift)

            img1_shifted = img1[y_start:y_end, x_start:x_end]
            img2_cropped = img2[:img1_shifted.shape[0], :img1_shifted.shape[1]]

            value = function(img1_shifted, img2_cropped)
            # print(y_shift, x_shift, value)
            if value < min_value:
                min_value = value

    return min_value


datasets = ['apartment_0', 'hotel_0', 'office_0', 'office_4', 'room_0', 'room_1']
cubemap_sides = 'FLRBUD'

gt_path = r"G:\OmniPhotos\Data\new\GT-Replica\{dataset}\cubmap_image\image\{dataset}_{index:04}_{side}.jpg"
output_path = r"G:\OmniPhotos\GT quantitative comparison"

# compute = ['PSNR']
# compute = ['SSIM']
compute = ['LPIPS']

# comparison = "DIS flow"
# comparison = "Fewer images (15-DIS)"
# comparison = "Fewer images (30-DIS)"
# comparison = "Fewer images (45-DIS)"
# comparison = "High-res proxy"
# comparison = "Huber-depth-sres"
# comparison = "L2-depth-sres"
# comparison = "Less smoothness"
# comparison = "Low-res proxy"
# comparison = "MegaParallax-cylinder-3m"
# comparison = "MegaParallax-plane-3m"
# comparison = "More smoothness"
# comparison = "No flow (linear blending)"
# comparison = "No normalised residuals"
# comparison = "No robust data term"
# comparison = "Optimising depth (not inverse)"
# comparison = "Our complete method"
# comparison = "Our method (GT inputs)"
# comparison = "Parallax360-cylinder-3m"

for comparison in [
    "DIS flow",
    "Fewer images (15-DIS)",
    "Fewer images (30-DIS)",
    "Fewer images (45-DIS)",
    "High-res proxy",
    "Huber-depth-sres",
    "L2-depth-sres",
    "Low-res proxy",
    "MegaParallax-cylinder-3m",
    "MegaParallax-plane-3m",
    "No flow (linear blending)",
    "No normalised residuals",
    "No robust data term",
    "Optimising depth (not inverse)",
    "Our complete method",
    "Our method (GT inputs)",
    "Parallax360-cylinder-3m"
]:

    ours_path = os.path.join(output_path, comparison, "{dataset}-Replica-cubemaps-{index:04}_{side}.png")

    print("Computing " + (" and ".join(compute)) + f" for '{comparison}' ...")

    if 'PSNR' in compute:
        total_count = 0
        total_psnr = 0
        total_psnr_squared = 0

        with open(os.path.join(output_path, comparison + " - PSNR.csv"), 'w', newline='') as csvfile:
            csvwriter = csv.writer(csvfile, delimiter=',', quoting=csv.QUOTE_NONE)
            csvwriter.writerow(["Dataset", "Index", "Face", "PSNR"])

            for dataset in datasets:
                dataset_count = 0
                dataset_psnr = 0

                print(dataset)
                for index in range(81):
                    print(f"  {index}.")
                    for side in cubemap_sides:
                        ## Paths to images
                        gt_image_path = gt_path.format(**locals())
                        ours_image_path = ours_path.format(**locals())

                        ## Load images
                        gt_img = imageio.imread(gt_image_path)
                        ours_img = imageio.imread(ours_image_path)

                        ## Compute metrics
                        psnr = max_shifted_metric(calculate_psnr, gt_img, ours_img)

                        ## Log result
                        # print(f"{dataset} -- {index:02} -- {side} -- PSNR: {psnr:.2f}")
                        csvwriter.writerow([dataset, index, side, psnr])
                        dataset_count += 1
                        dataset_psnr += psnr
                        total_psnr_squared += psnr * psnr

                psnr = dataset_psnr / dataset_count
                print(f'PSNR: {psnr:.2f}')

                total_count += dataset_count
                total_psnr += dataset_psnr

        mean_psnr = total_psnr / total_count
        stdev_psnr = sqrt(total_psnr_squared / total_count - pow(mean_psnr, 2))
    sem_psnr = stdev_psnr / sqrt(total_count)
    print(f"PSNR: {mean_psnr:.2f} +/- {sem_psnr:.2f}")


if 'SSIM' in compute:
    total_count = 0
    total_ssim = 0
        total_ssim_squared = 0

        with open(os.path.join(output_path, comparison + " - SSIM.csv"), 'w', newline='') as csvfile:
            csvwriter = csv.writer(csvfile, delimiter=',', quoting=csv.QUOTE_NONE)
            csvwriter.writerow(["Dataset", "Index", "Face", "SSIM"])

            for dataset in datasets:
                dataset_count = 0
                dataset_ssim = 0

                print(dataset)
                for index in range(81):
                    print(f"  {index}.")
                    for side in cubemap_sides:
                        ## Paths to images
                        gt_image_path = gt_path.format(**locals())
                        ours_image_path = ours_path.format(**locals())

                        ## Load images
                        gt_img = imageio.imread(gt_image_path)
                        ours_img = imageio.imread(ours_image_path)

                        ## Compute metrics
                        ssim = max_shifted_metric(calculate_ssim, gt_img, ours_img)

                        ## Log result
                        csvwriter.writerow([dataset, index, side, ssim])
                        dataset_count += 1
                        dataset_ssim += ssim
                        total_ssim_squared += ssim * ssim

                ssim = dataset_ssim / dataset_count
                print(f'SSIM: {ssim:.4f}')

                total_count += dataset_count
                total_ssim += dataset_ssim

        mean_ssim = total_ssim / total_count
        stdev_ssim = sqrt(total_ssim_squared / total_count - pow(mean_ssim, 2))
    sem_ssim = stdev_ssim / sqrt(total_count)
    print(f"SSIM: {mean_ssim:.4f} +/- {sem_ssim:.4f}")


if 'LPIPS' in compute:

    ## Help python find LPIPS-related modules
    import sys
    sys.path.append("LPIPS")
    import LPIPS.models
    from LPIPS.util import util

        # Initialise the LPIPS model
        use_gpu = True
        lpips_model = LPIPS.models.PerceptualLoss(model='net-lin', net='alex', use_gpu=use_gpu, version='0.1')

        total_count = 0
        total_lpips = 0
        total_lpips_squared = 0

        with open(os.path.join(output_path, comparison + " - LPIPS.csv"), 'w', newline='') as csvfile:
            csvwriter = csv.writer(csvfile, delimiter=',', quoting=csv.QUOTE_NONE)
            csvwriter.writerow(["Dataset", "Index", "Face", "LPIPS"])

            for dataset in datasets:
                dataset_count = 0
                dataset_lpips = 0

                print(dataset)
                for index in range(81):
                    print(f"  {index}.")
                    for side in cubemap_sides:
                        ## Paths to images
                        gt_image_path = gt_path.format(**locals())
                        ours_image_path = ours_path.format(**locals())

                        ## Load images
                        img0 = util.load_image(gt_image_path)
                        img1 = util.load_image(ours_image_path)

                        ## Compute metric
                        lpips = min_shifted_metric(calculate_lpips, img0, img1)

                        ## Log result
                        csvwriter.writerow([dataset, index, side, lpips])
                        dataset_count += 1
                        dataset_lpips += lpips
                        total_lpips_squared += lpips * lpips

                lpips = dataset_lpips / dataset_count
                print(f'LPIPS: {lpips:.4f}')

                total_count += dataset_count
                total_lpips += dataset_lpips

        mean_lpips = total_lpips / total_count
        stdev_lpips = sqrt(total_lpips_squared / total_count - pow(mean_lpips, 2))
        sem_lpips = stdev_lpips / sqrt(total_count)
        print(f"LPIPS: {mean_lpips:.4f} +/- {sem_lpips:.4f}")
