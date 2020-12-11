# -*- coding: utf-8 -*-
import os
import shutil
import pathlib

import colmap.ScriptingColmap as colmap

def trajectory_reconstruct_colmap(colmap_bin,
                                    images_dir,
                                    colmap_database_dir,
                                    vocab_tree_path,
                                    show_result_step_1=True,
                                    mapping_frame_interval=10):
    """
    Utility for COLMAP reconstruction, Recommend COLMAP version 3.6-dev.3

    :param colmap_bin: the path of COLMAP.bat
    :param images_dir: the directory of input images
    :param colmap_database_dir: the directory to storage the *.db and the reconstructed result
    :param vocab_tree_path: vocab tree file path
    :param show_result_step_1: switch of show the result of first step
    :param mapping_frame_interval: the interval of frames used to mapping 
    """
    # set up runtime environment
    dataset_dir = os.path.dirname(colmap_database_dir + "/")
    colmap_database_dir = dataset_dir
    vocab_tree = vocab_tree_path

    if not os.path.exists(colmap_database_dir):
        os.makedirs(colmap_database_dir, exist_ok=True)

    # check the version compatibility
    colmap.check_version(colmap_bin)

    # Step 1. Initial sparse reconstruction. ---------------------------------------------------------------------------
    sparse_colmap_database = os.path.join(colmap_database_dir, 'sparse-reconstruction.db')
    sparse_model_dir = os.path.join(colmap_database_dir, 'sparse-model')
    if not os.path.exists(sparse_model_dir):
        os.makedirs(sparse_model_dir)

    # Take a subset of every 10th image.
    sparse_images_dir = os.path.join(images_dir, '..', os.path.basename(images_dir) + '-{}th'.format(mapping_frame_interval))
    if os.path.exists(sparse_images_dir):
        shutil.rmtree(sparse_images_dir)

    os.makedirs(sparse_images_dir)
    images = sorted(os.listdir(images_dir))
    images = [image for image in images if image[-4:] == '.jpg' or image[-4:] == '.png']
    #images = images[::10]
    images = images[::mapping_frame_interval]

    for image in images:
        shutil.copy2(os.path.join(images_dir, image), sparse_images_dir)

    print("colmap.feature_extractor")
    colmap.feature_extractor(colmap_bin, sparse_colmap_database, sparse_images_dir,
                             params={
                                 'ImageReader.single_camera': 'true',
                                 'ImageReader.camera_model': 'SIMPLE_PINHOLE',
                                 'ImageReader.camera_params': '346.41016151, 600, 600',
                                 'SiftExtraction.estimate_affine_shape': 'true',
                                 # 'SiftExtraction.domain_size_pooling': 'true'
                             })

    print("colmap.exhaustive_matcher")
    colmap.exhaustive_matcher(colmap_bin, sparse_colmap_database,
                              params={
                                  'ExhaustiveMatching.block_size': 10,
                                  'SiftMatching.guided_matching': 'true'
                              })

    print("colmap.mapper")
    colmap.mapper(colmap_bin,
                database_path=sparse_colmap_database,
                image_path=sparse_images_dir,
                output_path=sparse_model_dir,
                input_path="",
                image_list_path="")

    # show result
    if show_result_step_1:
        result_model_list = os.listdir(sparse_model_dir)
        #check and output the reconstruction information
        if 0 == len(result_model_list):
            msg = "warning: colmap reconstruction is fail!!"
            print(msg)
            #raise Exception(msg)
            return
        for result_model in result_model_list:
            colmap.show_model(colmap_bin,
                        import_path=str(pathlib.Path(sparse_model_dir) / result_model),
                        database_path=sparse_colmap_database,
                        image_path=sparse_images_dir)

    # Step 2. Register all images to the model -------------------------------------------------------------------------
    full_colmap_database = os.path.join(colmap_database_dir, 'full-reconstruction.db')
    if not os.path.exists(full_colmap_database):
        shutil.copy2(sparse_colmap_database, full_colmap_database)

    colmap.feature_extractor(colmap_bin, full_colmap_database, images_dir,
                             params={
                                 'ImageReader.single_camera': 'true',
                                 'ImageReader.camera_model': 'SIMPLE_PINHOLE',
                                 'ImageReader.camera_params': '346.41016151, 600, 600',
                                 'SiftExtraction.estimate_affine_shape': 'true',
                                 # 'SiftExtraction.domain_size_pooling': 'true'
                             })

    colmap.vocab_tree_matcher(colmap_bin, full_colmap_database,
                              params={
                                  'vocab_tree_path': vocab_tree.replace('\\', '/'),
                                  'VocabTreeMatching.num_images': 200,  # default: 100
                                  'SiftMatching.guided_matching': 'true'
                              })

    full_model_dir = os.path.join(colmap_database_dir, 'full-model')
    if not os.path.exists(full_model_dir):
        os.makedirs(full_model_dir)
    colmap.image_registrator(colmap_bin, full_colmap_database, os.path.join(sparse_model_dir, '0'), full_model_dir)

    full_model_ba_dir = os.path.join(colmap_database_dir, 'full-model-ba')
    if not os.path.exists(full_model_ba_dir):
        os.makedirs(full_model_ba_dir)
    colmap.bundle_adjuster(colmap_bin, full_model_dir, full_model_ba_dir,
                           params={
                               'BundleAdjustment.max_num_iterations': 200,  # default: 100
                           })

    # convert global bundle adjustment *.bin model file to *.txt
    full_model_ba_txt_dir = os.path.join(colmap_database_dir, 'full-model-ba-txt')
    colmap.model_converter(colmap_bin, full_model_ba_dir, full_model_ba_txt_dir)

    # convert the reconstructed point cloud *.bin files to *.ply
    #dense_point_cloud_ply_path = os.path.join(colmap_database_dir, 'full_model_ba_points3D.ply')
    dense_point_cloud_ply_path = os.path.join(colmap_database_dir, 'full_model_ba_points3D.ply')
    colmap.model_converter_bin2ply(colmap_bin, input_path = full_model_ba_dir,
                           output_path = dense_point_cloud_ply_path)

    # generate project.ini file
    project_ini_file_path = os.path.join(full_model_ba_dir, 'project.ini')
    colmap.reduce_ini(project_ini_file_path)