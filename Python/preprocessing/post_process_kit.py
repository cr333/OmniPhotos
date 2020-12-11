import re
import csv
import sys
import os
import pathlib
import json
import msgpack
import shutil

import numpy as np

from PIL import Image, ImageFilter  
from scipy.spatial.transform import Rotation as R

def blur_photographer(original_image_dir, mask_image_dir, output_dir):
    """
    
    """
    output_dir = pathlib.Path(output_dir)
    if not output_dir.exists():
        output_dir.mkdir()

    for item_path in pathlib.Path(original_image_dir).iterdir():
        if not item_path.suffix == ".jpg":
            continue

        image_original =  Image.open(str(item_path))

        # get blur image
        image_blured  = image_original.filter(ImageFilter.GaussianBlur(radius = 15)) 

        # read mask image
        image_idx = re.findall(r'\d+', str(item_path.name))[0]
        image_mask_filename = "{}.jpg".format(image_idx)
        image_mask = Image.open(str(pathlib.Path(mask_image_dir) / image_mask_filename))

        # blend two images & output 
        masked_image = Image.composite(image_original, image_blured, image_mask)
        output_image_file_name = output_dir / item_path.name
        masked_image.save(str(output_image_file_name), quality=95)

        # show the image
        masked_image.show()


def load_replica_camera_traj(traj_file_path):
    """
    the format:
    index 
    """
    camera_traj = []
    traj_file_handle = open(traj_file_path, 'r')
    for line in traj_file_handle:
        split = line.split()
        #if blank line, skip
        if not len(split):
            continue
        camera_traj.append(split)
    traj_file_handle.close()
    return camera_traj


def openvslam_traj_2_op(input_ov_traj_file, output_op_traj_file, \
                                op_json_file_path, replica_json_file_path):
    """
    """
    # replica coordinate system --> openvslam coordinate system
    replica_2_openvslam_r = R.from_matrix([[0.0, -1.0, 0.0], [0.0, 0.0, -1.0], [1.0, 0.0, 0.0]])
    replica_2_openvslam_t = np.array([0.0, 0.0, 0.0])  # the center of rendering camera 
    replica_2_openvslam_s = np.array([1.0,1.0,1.0])
    # openvslam coordinate system --> OmniPhotos coordinate system
    openvslam_2_op_r = R.from_matrix([[1.0, 0.0, 0.0], [0.0, -1.0, 0.0], [0.0, 0.0, -1.0]])
    openvslam_2_op_t = np.array([0.0, 0.0, 0.0])  # put the camera center to origin of coordinate system
    openvslam_2_op_s = None

    # load openvslam to mp scalar from json
    op_json_file = open(op_json_file_path)
    op_json = json.load(op_json_file)
    openvslam_2_op_s = op_json["Dataset"]["Scaling"]
    op_center_point = np.array([op_json["Dataset"]["CameraCircle"]["Centroid"]["X"], \
                                op_json["Dataset"]["CameraCircle"]["Centroid"]["Y"], \
                                op_json["Dataset"]["CameraCircle"]["Centroid"]["Z"]], dtype="float64")
    op_json_file.close()
    print("the scalar from openvslam 2 mp is {}".format(openvslam_2_op_s))

    # load replica camera center
    replica_json_file = open(replica_json_file_path)
    replica_json = json.load(replica_json_file)
    replica_camera_center = [ \
        replica_json["camera_traj"]["start_position"]["x"], \
        replica_json["camera_traj"]["start_position"]["y"], \
        replica_json["camera_traj"]["start_position"]["z"]]
    replica_camera_center_poses = np.array(replica_camera_center, dtype="float64")
    openvslam_center_point = replica_2_openvslam_r.apply(replica_camera_center_poses)
    replica_json_file.close()

    # load original camera trajectory
    print("1) loading original openvslam camera and do coordinate transform: {}".format(input_ov_traj_file))
    camera_traj_data = []
    input_ov_traj_handle = open(input_ov_traj_file, 'r')
    input_ov_traj = csv.reader(input_ov_traj_handle, delimiter=' ', quotechar='|')
    for line in input_ov_traj:
        if not len(line) or line[0] == "#":
            camera_traj_data.append(line)
            continue

        # do transform: replica coordinate system --> openvslam coordinate system
        replica_camera_position = np.array([float(line[1]), float(line[2]), float(line[3])])
        # do transform: openvslam coordinate system --> OmniPhotos coordinate system
        openvslam_camera_position = replica_2_openvslam_r.apply(replica_camera_position)
        op_point = openvslam_2_op_r.apply(openvslam_camera_position - openvslam_center_point + op_center_point) * openvslam_2_op_s

        line[1] = str(op_point[0])
        line[2] = str(op_point[1])
        line[3] = str(op_point[2])

        camera_traj_data.append(line)

    input_ov_traj_handle.close()

    # to text and obj
    output_ov_traj_text = []
    output_ov_traj_obj = []
    for line in camera_traj_data:
        output_ov_traj_text.append(" ".join(line))
        if not len(line) or line[0] == "#" :
            continue
        else:
            output_ov_traj_obj.append("v {} {} {}".format(line[1], line[2], line[3]))

    # output to target obj file
    print("2) generate new traj & obj file {}".format(output_op_traj_file))
    output_op_traj_handle = open(output_op_traj_file, 'w')
    new_traj_str = '\n'.join(output_ov_traj_text)
    output_op_traj_handle.write("# index, t_x, t_y, t_z, r_x, r_y, r_z\n")
    output_op_traj_handle.write(new_traj_str)
    output_op_traj_handle.close()

    output_op_obj_handle = open(output_op_traj_file + ".obj", 'w')
    new_obj_str = '\n'.join(output_ov_traj_obj)
    output_op_obj_handle.write(new_obj_str)
    output_op_obj_handle.close()


# from replica coordinate to openvslam coordinate
def generate_gt_proxy_geometry(input_obj_file, output_obj_file, \
                                op_json_file_path, camera_center_csv_file_path):
    """
    """
    # replica coordinate system --> openvslam coordinate system
    replica_2_openvslam_r = R.from_matrix([[0.0, -1.0, 0.0], [0.0, 0.0, -1.0], [1.0, 0.0, 0.0]])
    replica_2_openvslam_t = np.array([0.0, 0.0, 0.0])  # the center of rendering camera 
    replica_2_openvslam_s = np.array([1.0,1.0,1.0])

    # openvslam coordinate system --> OmniPhotos coordinate system
    openvslam_2_op_r = R.from_matrix([[1.0, 0.0, 0.0], [0.0, -1.0, 0.0], [0.0, 0.0, -1.0]])
    openvslam_2_op_t = np.array([0.0, 0.0, 0.0])  # put the camera center to origin of coordinate system
    openvslam_2_op_s = None
    # load openvslam 2 mp scalar from json
    op_json_file = open(op_json_file_path)
    op_json = json.load(op_json_file)
    openvslam_2_op_s = op_json["Dataset"]["Scaling"]
    print("the scalar from openvslam 2 mp is {}".format(openvslam_2_op_s))

    camera_center = load_replica_camera_traj(camera_center_csv_file_path)

    replica_center_camera_poses = np.array([float(camera_center[0][1]),float(camera_center[0][2]),\
                                            float(camera_center[0][3]), ], dtype="float64")
    op_center_point = np.array([op_json["Dataset"]["CameraCircle"]["Centroid"]["X"], \
                                      op_json["Dataset"]["CameraCircle"]["Centroid"]["Y"], \
                                      op_json["Dataset"]["CameraCircle"]["Centroid"]["Z"]], dtype="float64")

    # load original obj
    print("1) loading obj file: {}".format(input_obj_file))
    point_cloud_data = []
    obj_file_handle = open(input_obj_file, 'r')
    for line in obj_file_handle:
        split = line.split()
        point_cloud_data.append(split)
    obj_file_handle.close()

    # do transform and output to obj
    print("2) do coordinate transform")
    for idx, line in enumerate(point_cloud_data):
        if idx % 10000 ==0:
            print("{}% : {} / {}".format(float(idx)/ float(len(point_cloud_data)), idx, len(point_cloud_data)))
        if not len(line) or line[0] == "#" or line[0] != "v" :
            continue
        # do transform: replica coordinate system --> openvslam coordinate system
        replica_point = np.array([float(line[1]), float(line[2]), float(line[3])])
        # do transform: openvslam coordinate system --> OmniPhotos coordinate system
        openvslam_point = replica_2_openvslam_r.apply(replica_point)
        openvslam_center_point = replica_2_openvslam_r.apply(replica_center_camera_poses)
        op_point = openvslam_2_op_r.apply(openvslam_point - openvslam_center_point) * openvslam_2_op_s
        line[1] = str(op_point[0] )
        line[2] = str(op_point[1] )
        line[3] = str(op_point[2] )

    # to text
    obj_text = []
    for line in point_cloud_data:
        obj_text.append(" ".join(line))

    # output to target obj file
    print("3) generate obj file {}".format(output_obj_file))
    output_obj_handle = open(output_obj_file, 'w')
    obj_points_str = '\n'.join(obj_text)
    output_obj_handle.write(obj_points_str)
    output_obj_handle.close()


# note msgpack with 0.5.6 version
def openvslam_msg_2_obj(input_msg_file_path, output_obj_file_path):
    """
    """
    json_file_path = output_obj_file_path + ".json"
    obj_file_path = output_obj_file_path
    data = open(input_msg_file_path, 'rb')
    data_text = msgpack.Unpacker(data, raw=False).unpack()

    # 1) convert the msg to json
    self.show_info("Output OpenVSLAM msg file to json : {}".format(json_file_path))
    with open(json_file_path, 'w') as json_file:
        json_file.write(json.dumps(data_text))

    # 2) generate the obj file for point cloud
    self.show_info("Output OpenVSLAM point cloud to: {}".format(obj_file_path))
    landmarks = data_text["landmarks"]
    with open(obj_file_path, 'w') as obj_file:
        for key in landmarks:
            landmark = landmarks[key]["pos_w"]
            point_txt = "v {} {} {} {}".\
                format(str(landmark[0]), str(
                    landmark[1]), str(landmark[2]), os.linesep)
            obj_file.write(point_txt)


def openvslam__msg_to_obj(msg_file_path):
    """
    parser the msg output the corresponding json and landmark (point cloud).
    output to the same folder of input msg file
    :param msg_file_path: the path of msg file
    """
    json_file_path = msg_file_path + ".json"
    obj_file_path = msg_file_path + ".obj"
    data = open(msg_file_path, 'rb')
    data_text = msgpack.Unpacker(data, raw=False).unpack()

    # 1) convert the msg to json
    with open(json_file_path, 'w') as json_file:
        json_file.write(json.dumps(data_text))

    # 2) generate the obj file for point cloud
    landmarks = data_text["landmarks"]
    with open(obj_file_path, 'w') as obj_file:
        for key in landmarks:
            landmark = landmarks[key]["pos_w"]
            point_txt = "v {} {} {} {}".\
                format(str(landmark[0]), str(
                    landmark[1]), str(landmark[2]), os.linesep)
            obj_file.write(point_txt)


def obj_2_openvslam_msg(input_obj_file_path, output_msg_file_path):
    """
    generate map.msg file
    """
    print("Generate {}..........".format(output_msg_file_path))
    # load_pointcloud_from_obj
    point_cloud_data = []
    obj_file_handle = open(input_obj_file_path, 'r')
    for line in obj_file_handle:
        split = line.split()
        #if blank line, skip
        if not len(split):
            continue
        if split[0] == "v":
            point_cloud_data.append([float(split[1]), float(split[2]), float(split[3])])

    obj_file_handle.close()

    # output to msg file		# output for msg
    landmarks = {}
    for idx in range(len(point_cloud_data)):
        landmarks[str(idx)] = {"pos_w": point_cloud_data[idx]}
    data_text = {"landmarks": landmarks}
    # pack and output
    msgpack_packer = msgpack.Packer(use_bin_type=True)
    mapmsg_file = open(output_msg_file_path, 'wb')
    mapmsg_file.write(msgpack_packer.pack(data_text))
    mapmsg_file.close()


def siggraph_2017_data_analysis():
    image_name = []
    image_info = {}
    with open('images.txt', 'r') as file:
        reader = csv.reader(file, delimiter=' ', quotechar='|')
        for row in reader:
            if row[0] == "#" or len(row) < 3 or len(row) > 20 :
                continue
            #print(row[0], row[9])
            image_name.append(row[9])
            image_info[row[9]] = row[0]

    counter = 1
    image_name.sort()
    #print(image_name)
    for idx, i in enumerate(image_name):
        print(idx, counter , i , image_info[i])
        counter = 3 + counter


def siggraph_2017_comput_camera_pose(root_dir):
    """
    transform Siggraph 2017 casual 3D dataset to Openvslam Format
    """
    camera_intrinsics = {}
    camera_proj_mats = {}
    camera_pose = {}
    camera_image_file = {}

    dest_image_dir = root_dir + "../../Input_op/"
    if not pathlib.Path(dest_image_dir).exists():
        pathlib.Path(dest_image_dir).mkdir()
    image_file_name_new_exp = r"pinhole-{:04d}.jpg"

    # 1) load camera intrinsic paramters & projection matrix
    print("1) load camera intrinsic paramters & projection matrix")
    for dir_item in pathlib.Path(root_dir).iterdir():
        if dir_item.suffix == ".csv" or  dir_item.suffix == ".obj" :
            continue

        img_idx = re.findall(r'\d+', dir_item.name)[0]
        #print(dir_item)

        if dir_item.suffix == ".jpg":
            camera_image_file[img_idx] = dir_item.name

        # load camera intrinsic parameter
        if dir_item.name.find(".jpg.camera.txt") != -1:
            with open(str(dir_item), "r") as file_handle:
                for line in file_handle:
                    line = line.split()
                    if line[0] == "#":
                        continue
                    # MODEL, WIDTH, HEIGHT, PARAMS[fx, fy, cx, cy]
                    camera_intrinsic = \
                        np.array([[float(line[3]), 0.0 , float(line[5])],\
                                 [0.0 , float(line[4]), float(line[6])],\
                                 [ 0.0 , 0.0 , 1.0]])
                    camera_intrinsics[img_idx] = camera_intrinsic

        # load camera r_t 
        if dir_item.name.find(".jpg.proj_matrix.txt") != -1:
            with open(str(dir_item), "r") as file_handle:
                params = []
                for line in file_handle:
                    line = line.split()
                    for param in line:
                        params.append(float(param))

            params_op = np.array(params, dtype=np.float64)
            params_op = params_op.reshape([3,4])
            camera_proj_mats[img_idx] = params_op
		
    # 2) comput the camera R_T
    print("2) comput the camera R_T matrix")
    for key in camera_intrinsics.keys():
        intrinsic_mat = camera_intrinsics[key]
        intrinsic_mat_inv = np.linalg.inv(intrinsic_mat)
        rt_mat = np.matmul(intrinsic_mat_inv ,camera_proj_mats[key])
        camera_pose[key] = rt_mat

    # 3) output files to csv & obj
    camera_pose_csv_file_path = root_dir + "camera_pose.csv"
    camera_pose_csv_file = open(camera_pose_csv_file_path, 'w')
    # camera_pose_csv_file.write("# idx, t_x, t_y, t_z, r_x, r_y, r_z \n")
    camera_pose_csv_file.write("# idx, filename, t_x, t_y, t_z, q_x, q_y, q_z, q_w \n")
    pose_csv_writer = csv.writer(camera_pose_csv_file, lineterminator="\n",
                    delimiter=' ', quoting=csv.QUOTE_MINIMAL)

    # output to obj for debug
    debug_obj_file_path = root_dir + "camera_pose.csv.obj"
    debug_obj_file = open(debug_obj_file_path, 'w')

    for key, value in camera_pose.items():
        if int(key) % 10 == 0:
            print(key)

        # copy and rename the images
        image_file_name_original = root_dir + camera_image_file[key]
        image_file_name_new = image_file_name_new_exp.format(int(key))
        image_file_name_new_path = dest_image_dir + image_file_name_new
        shutil.copyfile(image_file_name_original, image_file_name_new_path)

        # OpenVSLAM
        # # comput new r and T
        # r_mat = R.from_matrix(value[:3,:3]) # get R
        # r_mat_inv = np.linalg.inv(r_mat.as_matrix())
        # # get T
        # t_vec = value[:3,3]
        # t_vec = -np.matmul(r_mat_inv, t_vec)
        # #pose_csv_writer.writerow([key] + t_vec.tolist() + r_mat.as_euler('xyz', degrees=True).tolist())
        # # pose_csv_writer.writerow([int(key)] + [image_file_name_new] \
        # #             + t_vec.tolist() + R.from_matrix(r_mat).as_quat().tolist())
        # pose_csv_writer.writerow([int(key)] + [image_file_name_new] \
        #             + t_vec.tolist() + r_mat.as_quat().tolist())
        # debug_obj_file.write("v {} {} {}\n".format(t_vec[0], t_vec[1], t_vec[2]))

        # for COLMAP
        # comput new r and T
        r_mat = R.from_matrix(value[:3,:3]) # get R
        r_quat = r_mat.as_quat()
        # get T
        t_vec = value[:3,3]
        #pose_csv_writer.writerow([key] + t_vec.tolist() + r_mat.as_euler('xyz', degrees=True).tolist())
        # pose_csv_writer.writerow([int(key)] + [image_file_name_new] \
        #             + t_vec.tolist() + R.from_matrix(r_mat).as_quat().tolist())
        #  IMAGE_ID, QW, QX, QY, QZ, TX, TY, TZ, CAMERA_ID, NAME
        pose_csv_writer.writerow([int(key)] + [r_quat[3], r_quat[0], r_quat[1], r_quat[2]] 
        + t_vec.tolist() + [1]  + [image_file_name_new])
        # POINTS2D[] as (X, Y, POINT3D_ID)
        pose_csv_writer.writerow("0,0,0")
        debug_obj_file.write("v {} {} {}\n".format(t_vec[0], t_vec[1], t_vec[2]))


    camera_pose_csv_file.close()
    debug_obj_file.close()


def siggraph_2017_reszie_images(op_root_dir):
    """
    the british museum image's resolution is 15904 x 6737 
    resize the image to quarter 3976 x 1684 
    1) first crop to 15904 x 6736
    2) resize to 3976 x 1684
    """
    width_new = 3976
    height_new = 1684

    output_dir = op_root_dir + "Input_{}_{}/".format(width_new, height_new)
    if not pathlib.Path(output_dir).exists():
        pathlib.Path(output_dir).mkdir()
    image_dir_path = op_root_dir + "Input/"
    cameras_txt_file_path = op_root_dir + "Capture/openvslam/cameras.txt"

    # 1) crop image
    counter = 0
    for item in pathlib.Path(image_dir_path).iterdir():
        if not item.suffix == ".jpg":
            continue

        counter  = counter + 1
        if counter % 10 == 0:
            print(counter)

        # crop image
        image_data = Image.open(str(item))
        image_data = image_data.crop((0,0,15904,6736))

        # # resize image
        image_data = image_data.resize((width_new, height_new))

        image_data.save(output_dir + item.name, "JPEG", quality= 95)

    # 2) generate new camera image
    with open(cameras_txt_file_path, newline='') as csvfile:
         spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
         for row in spamreader:
            if row[0] == "#":
                continue
            print(', '.join(row))
            print("{} {} {} {} {} {} {}".format( \
                row[0], row[1], float(row[2])/4.0, float(row[3])/4.0,\
                float(row[4]) /4.0, float(row[5])/4.0, float(row[6])/4.0, float(row[7])/4.0))


def cheknew_peter_sig_2017_image_txt():
    """
    """
    def load_camera_pose(input_filepath, output_file_path):
        data_list = []
        with open(input_filepath) as fp:
            cnt = 0
            for line in fp:
                cnt = cnt + 1
                if cnt % 2 == 0 or line[0] == "#":
                    continue
                print("Line {}: {}".format(cnt,line))
                data_list.append(line.split(" "))

        def get_x(data):
            return float(data[5])
        data_list.sort(key=get_x)
        #print(data_list)
        with open(output_file_path, 'w') as f:
            for item in data_list:
                f.write("%s" % (" ".join(item)))

        return data_list

    # 0    1         2          
    #140 0.754744 -0.13975 -0.640955 -0.00278308 0.145118 0.0886817 -3.30874 1 ldr/DSC12464.JPG
    def find_corresponding(new_list, old_list):
        result_list = []
        result_corresponding_old = {}
        result_corresponding_new = {}
        result_no_coresponding = []
        for item_idx, item_old in enumerate(old_list):
            have_same_term = False

            for item_new in new_list:
                have_same_value = True
                for idx in range(1, 8):
                    if abs(float(item_old[idx]) - float(item_new[idx])) > 0.001:
                        have_same_value = False
            
                if have_same_value:
                    have_same_term = True
                    break

            if have_same_term:
                result_list.append(True)
                result_corresponding_old[item_idx] = item_old
                result_corresponding_new[item_idx] = item_new
            else:
                result_list.append(False)
                result_no_coresponding.append(item_old)

        # output result
        with open("d:/debug_result.csv", "w", newline="\n") as fh:
            fh.write("# old, new , there are {} same.\n".format(len(result_corresponding_new.keys())))
            for key in result_corresponding_new.keys(): 
                fh.write("{}{}\n".format(",".join(result_corresponding_old[key]),\
                     ",".join(result_corresponding_new[key])))

            fh.write("# ===== without corresponding==== \n")
            for line in result_no_coresponding:
                fh.write("{}".format(",".join(line)))

            
    output_file_path = 'images.txt.old.sorted'
    input_filepath = "D:/workdata/casual_3d_photography/british-museum/sparse/model/images.txt"
    data_old = load_camera_pose(input_filepath, output_file_path)

    output_file_path = 'images.txt.new.sorted'
    input_filepath = "d:/workdata/casual_3d_photography/british-museum/sparse/undistorted/camera_pose.csv"
    data_new = load_camera_pose(input_filepath, output_file_path)

    find_corresponding(data_new, data_old)

    

if __name__ == "__main__":

    print('Number of arguments:', len(sys.argv), 'arguments.')
    print('Argument List:', str(sys.argv))

    if sys.argv[1] == "0":
        # convert obj to msg file
        # root_dir = "D:/workdata/GT-Replica-workdata-3840x1920/room_0/Preprocessing/"
        # input_obj = root_dir + "map.msg_3_8K.obj"
        # output_msg = root_dir + "map.msg_3_8K.msg"
        root_dir = "D:/workdata/casual_3d_photography/british-museum/"
        input_obj = root_dir + "point_cloud_sparse.obj"
        output_msg = root_dir + "point_cloud_sparse.msg"
        obj_2_openvslam_msg(input_obj, output_msg)
        
    elif sys.argv[1] == "1":
        # convert obj to OmniPhotos coordinate system
        root_dir = "D:/workdata/GT-Replica-workdata-3840x1920/room_0/"
        input_obj = root_dir + "proxy_geometry_gt.obj"
        output_obj = root_dir + "spherefit-ground-truth.obj"
        camera_center_csv_file_path= root_dir + "circle_center.csv"
        op_json_file_path= "D:/workdata/GT-Replica/room_0/Cache/90-4k-2k-DIS/PreprocessingSetup-0090.json"
        generate_gt_proxy_geometry(input_obj, output_obj, op_json_file_path,  camera_center_csv_file_path)

    elif sys.argv[1] == "2":
        # generate mesked input images
        root_dir = "/mnt/sda1/workdata/KobeGarden6/"
        original_image_dir = root_dir + "image/"
        mask_image_dir = root_dir + "mask/"
        masked_image_dir = root_dir + "Input_masked/"
        blur_photographer(original_image_dir, mask_image_dir, masked_image_dir)

    elif sys.argv[1] == "3":
        root_dir = "/mnt/sda1/workdata/GT-Replica-workdata-cubemap/room_1/"
        replica_json_file_path = root_dir + "config.json"
        input_ov_traj_file = root_dir + "grid.csv"
        output_op_traj_file = root_dir + "Preprocessing/frame_trajectory_op.csv"
        # op_json_file_path= "D:/workdata/GT-Replica/room_0/Cache/90-4k-2k-DIS/PreprocessingSetup-0090.json"
        op_json_file_path= "/mnt/sda1/workdata/GT-Replica/room_1/Cache/90-4k-2k-DIS/PreprocessingSetup-0090.json"
        openvslam_traj_2_op(input_ov_traj_file, output_op_traj_file, \
                                op_json_file_path, replica_json_file_path)

    elif sys.argv[1] == "4":
        # compute peter's dataset camera pose
        root_dir = "D:/workdata/casual_3d_photography/british-museum/sparse/undistorted/"
        siggraph_2017_comput_camera_pose(root_dir)
        exit(0)

    elif sys.argv[1] == "5":
        root_dir = "D:/workdata/casual_3d_photography/british-museum_op/"
        siggraph_2017_reszie_images(root_dir)

    elif sys.argv[1] == "6":
        msg_file_path = "D:/workdata/GenoaCathedral/openvslam_result/GenoaCathedral_map.msg"
        openvslam__msg_to_obj(msg_file_path)

    elif sys.argv[1] == "7":
        cheknew_peter_sig_2017_image_txt()
