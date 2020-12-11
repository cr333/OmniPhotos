# Remove images from the "Input" directory which are not used in the preprocessed dataset.
# Iterates over all cameras.csv's in the "Cache" directory and collects input images,
# then deletes the complement from the input image folder.

import argparse
import csv
import os
from os import listdir
from os.path import isfile, join
from pathlib import Path


def collectUsedImagesFromCamerasCSV(camera_file):
    line_count = 0
    list_images = []

    with open(camera_file) as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=',')
        for row in csv_reader:
            input_image = Path(row[22])  # image path
            list_images.append(input_image.name)
            line_count += 1

    print(f'Processed {line_count} lines.')
    return list_images


def cleanInputFolder(dataset_folder):
    """Removes unused input images in the given dataset folder."""

    # Step 1. Find all cache folders (could be more than 1)
    cache_folder = Path(dataset_folder) / "Cache"
    list_subfolders_with_paths = [f.path for f in os.scandir(cache_folder) if f.is_dir()]
    if len(list_subfolders_with_paths) == 0:
        raise Exception("Dataset does not contain any cache folders")

    # Step 2. Find all images that are used in the cache folders
    used_images = []
    for cache_folder in list_subfolders_with_paths:
        local_dir = Path(cache_folder)
        # TODO: check whether Cameras.csv exists in local directory
        cameras_file = local_dir / "Cameras.csv"
        local_used_images = collectUsedImagesFromCamerasCSV(cameras_file)
        for used_image in local_used_images:
            used_images.append(used_image)

    # Step 3. List all images in the Input directory
    input_folder = Path(dataset_folder) / "Input"
    input_files = [Path(f).name for f in listdir(input_folder) if isfile(join(input_folder, f))]

    # Step 4. Find the unused images and mark them for deletion
    files_to_delete = []
    for input_image in input_files:
        if input_image not in used_images:
            image_path = input_folder / input_image
            files_to_delete.append(image_path)

    # Step 5. Delete unused images
    for image in files_to_delete:
        os.remove(image)
    print(f"Deleted {len(files_to_delete)} out of {len(input_files)}")


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument("-f", "--folder", required=True, type=str,
                    help="Dataset folder whose input images we want to cleanup.")

    args = vars(ap.parse_args())
    folder = args["folder"]

    cleanInputFolder(folder)
