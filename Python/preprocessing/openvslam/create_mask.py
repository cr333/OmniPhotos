import csv

import numpy as np
from sklearn import linear_model, datasets
from PIL import Image
from matplotlib import pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from skimage.measure import LineModelND, ransac, CircleModel
from scipy.spatial.transform import Rotation as R
from envmap import EnvironmentMap


# the distance from up body to the center of circle (camera stick)
people2camera_meter = None
# the stick radius unit is meter
radius_meter = None

def spherical2dcm(spherical_coordinate, dcm):
    """
    convert spherical coordinate to DCM (Direction Cosine Matrix)
    :param spherical_coordinate: the spherical coordinate Euler angles [phi, theta]
    :param dcm: the rotation matrix (Direction Cosine Matrix), is 3x3 matrix
    :return:
    """
    dcm[:] = R.from_euler("xyz", [spherical_coordinate[1],
                                  spherical_coordinate[0], 0], degrees=True).as_matrix()


def rotate_image(data, rotation):
    """ 
    rotation images with skylibs 
    :param data: image data will be rotated, dimension is 4 [ x , width, height, 3] 
    :return : weather rotated the images 
    """
    if [0.0, 0.0] == rotation:
        return False

    rotation_matrix = np.zeros([3, 3])
    spherical2dcm(rotation, rotation_matrix)
    envmap = EnvironmentMap(data, format_='latlong')
    new_image = envmap.rotate("DCM", rotation_matrix).data
    data[:] = new_image.astype(np.uint8)
    return True


def rotate_image_fast(data, rotation):
    """ 
    rotation panoramic image with drift matrix 
    :param data: image data will be rotated, dimension is 4 [ x , width, height, 3] 
    :return : weather rotated images 
    """
    if [0.0, 0.0] == rotation:
        return False

    phi_roll_numb = rotation[0] / 360.0 * np.shape(data)[1]
    if phi_roll_numb == int(phi_roll_numb) and rotation[1] == 0.0:
        # do not need to interpolate, use numpy roll operation
        data[:] = np.roll(data, int(phi_roll_numb), axis=2)
        return True

    # interpolate (rotate) with skylibs
    return rotate_image(data, rotation)


def estimate_photographer_position_ransac_circle(position_list):
    """
    """
    # AX + By + C = z
    # (x-h)^2 + (y-k)^2 = r^2
    x = position_list[300:900, 0]
    y = position_list[300:900, 1]
    z = position_list[300:900, 2]

    # A*x^2 + B*x + C*y^2 + D*y + E*z^2 + F = z
    X = np.stack(x*x, x, y*y, y, z*z)
    Y = z

    ransac = linear_model.RANSACRegressor()
    ransac.fit(X, Y)
    inlier_mask = ransac.inlier_mask_
    outlier_mask = np.logical_not(inlier_mask)

    # Compare estimated coefficients
    print("Estimated coefficients (linear regression, RANSAC):")
    print(ransac.estimator_.coef_, ransac.estimator_.intercept_)

    xx, yy = np.meshgrid(np.linspace(X[:, 0].min(), X[:, 0].max(
    ), 20), np.linspace(X[:, 1].min(), X[:, 1].max(), 20))
    zz = xx * xx * ransac.estimator_.coef_[0] \
        + xx * ransac.estimator_.coef_[1] \
        + yy * yy * ransac.estimator_.coef_[2] \
        + yy * ransac.estimator_.coef_[3] \
        - ransac.estimator_.intercept_

    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    ax.plot_surface(xx, yy, zz, color=(0.3, 0.3, 0.3, 0.5))
    ax.scatter(X[inlier_mask][:, 0], X[inlier_mask][:, 1],
               Y[inlier_mask], color='yellowgreen', marker='.', label='Inliers')
    ax.scatter(X[outlier_mask][:, 0], X[outlier_mask][:, 1],
               Y[outlier_mask], color='gold', marker='x', label='Outliers')

    ax.set_xlabel('X Label')
    ax.set_ylabel('Y Label')
    ax.set_zlabel('Z Label')

    plt.show()


def estimate_photographer_position_ransac_plane(position_list, plot_result = False):
    """
    # AX + By + C = z
    3D plane function
    """
    # get rid of the start and end unstable pose
    start_idx = int(np.shape(position_list)[0] * 0.15)
    end_idx = int(np.shape(position_list)[0] * 0.85)

    X = position_list[start_idx:end_idx, :2]
    y = position_list[start_idx:end_idx, 2]

    # X = position_list[:, :2]
    # y = position_list[:, 2]

    ransac = linear_model.RANSACRegressor(stop_probability = 0.80)
    ransac.fit(X, y)
    inlier_mask = ransac.inlier_mask_
    outlier_mask = np.logical_not(inlier_mask)

    # Compare estimated coefficients
    print("Estimated coefficients (linear regression, RANSAC):")
    print(ransac.estimator_.coef_, ransac.estimator_.intercept_)

    A = ransac.estimator_.coef_[0]
    B = ransac.estimator_.coef_[1]
    C = ransac.estimator_.intercept_

    if plot_result:
        xx, yy = np.meshgrid(np.linspace(X[:, 0].min(), X[:, 0].max(), 20), np.linspace(X[:, 1].min(), X[:, 1].max(), 20))
        zz = xx * A + yy * B + C

        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')
        ax.plot_surface(xx, yy, zz, color=(0.3, 0.3, 0.3, 0.5))
        ax.scatter(X[inlier_mask][:, 0], X[inlier_mask][:, 1], y[inlier_mask], color='yellowgreen', marker='.', label='Inliers')
        ax.scatter(X[outlier_mask][:, 0], X[outlier_mask][:, 1], y[outlier_mask], color='gold', marker='x', label='Outliers')
        ax.scatter(position_list[:start_idx, 0], position_list[:start_idx, 1], position_list[:start_idx,2], color='blue', marker='>', label='Start')
        ax.scatter(position_list[end_idx:, 0], position_list[end_idx:, 1], position_list[end_idx:,2], color='blue', marker='>', label='End')

        ax.set_xlabel('X Label')
        ax.set_ylabel('Y Label')
        ax.set_zlabel('Z Label')

        plt.show()

    return [A/1.0, B/1.0, -1.0]


def estimate_photographer_position(traj_list, plot_result = False):
    """
    estimate the center of photographer 3D position in the 
    """
    # start and end index of frames
    tranlation_array = traj_list[:,1:4]
    start_idx = int(np.shape(tranlation_array)[0] * 0.35)
    end_idx = int(np.shape(tranlation_array)[0] * 0.95)

    # get the center of trajectory
    center_circle = np.average(tranlation_array[start_idx: end_idx], axis = 0)

    # get the radius of circle
    radius_vector = tranlation_array[start_idx: end_idx] - center_circle
    radius = np.average(np.sqrt(radius_vector[:,0] * radius_vector[:,0] \
                     + radius_vector[:,1] * radius_vector[:,1] \
                     + radius_vector[:,2] * radius_vector[:,2]))

    # get the upright vector, openvslam up is -Y
    up_vector = estimate_photographer_position_ransac_plane(traj_list[:, 1:4], True)
    if up_vector[1] < 0:
        up_vector = [-up_vector[0], -up_vector[1], -up_vector[2]]

    # estimate the persion position, with real stick length

    people2camera = radius / radius_meter * people2camera_meter 
    people2camera_vec = up_vector / np.linalg.norm(up_vector) * people2camera

    ptgpr_position = center_circle + people2camera_vec

    if plot_result:
        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')

        ax.scatter(tranlation_array[:start_idx, 0], tranlation_array[:start_idx, 1], tranlation_array[:start_idx,2], color='blue', marker='.', label='Camera_start')
        ax.scatter(tranlation_array[start_idx:end_idx, 0], tranlation_array[start_idx:end_idx, 1], tranlation_array[start_idx:end_idx,2], color='yellowgreen', marker='.', label='Camera_used')
        ax.scatter(tranlation_array[end_idx:, 0], tranlation_array[end_idx:, 1], tranlation_array[end_idx:,2], color='blue', marker='.', label='Camera_end')

        ax.scatter(ptgpr_position[0], ptgpr_position[ 1], ptgpr_position[2], color='blue', marker='x', label='People')
        ax.set_xlabel('X Label')
        ax.set_ylabel('Y Label')
        ax.set_zlabel('Z Label')
        plt.show()

    return ptgpr_position


def get_spherical_coord(src_trans, src_rot, tar_trans):
    """
    Right hand coordinate system.
    get the target theta & phi relation to source camera.
    The theta, phi as https://developers.google.com/vr/jump/rendering-ods-content.pdf

    @param src_trans: XYZ
    @param src_rot: rotation quaternion XYZ
    @return theta, phi  
    """
    openvslam_coord_transform = np.array((-1.0, -1.0, 1.0))
    src_trans = openvslam_coord_transform * src_trans
    tar_trans = openvslam_coord_transform * tar_trans

    src_rot_mat_inv = np.linalg.inv(R.from_quat(src_rot).as_matrix())
    src2tar_trans = np.dot(src_rot_mat_inv, tar_trans - src_trans)
    x = src2tar_trans[0] 
    y = src2tar_trans[1] 
    z = src2tar_trans[2]

    radius = np.sqrt(np.sum(src2tar_trans * src2tar_trans))
    # transform to OpenVSLAM coordinate
    theta = np.arctan2(z,x) * 180 / np.pi
    phi = np.arcsin(y/radius) * 180 / np.pi
    return theta, phi


def create_mask(initial_mask_file_path, traj_file_path, mask_output_path):
    """
    OpenVSLAM use right hand coordinate: \
        up (0, -1, 0), forward (0, 0, 1), left (-1, 0, 0)

    @param initial_mask_file_path: the original openvslam output csv file, corresponding the first frame 

    https://github.com/xdspacelab/openvslam/blob/5a0b1a5f52b4d29b699624052c9d5dc4417d9882/src/openvslam/io/trajectory_io.cc#L148
    << timestamp << trans_wc(0) << " " << trans_wc(1) << " " << trans_wc(2) << " " 
    << quat_wc.x() << " " << quat_wc.y() << " " << quat_wc.z() << " " << quat_wc.w()
    """
    initial_mask_file = Image.open(initial_mask_file_path)
    initial_mask = initial_mask_file.convert("L")  # convert image to black and white

    # warp initial mask base on the camera pose
    traj_file = open(traj_file_path)
    traj_csv_handle = csv.reader(traj_file, delimiter=' ', quoting=csv.QUOTE_NONNUMERIC)
    traj_list = [traj_csv_item for traj_csv_item in traj_csv_handle]
    transformation_array = np.asarray(traj_list)

    # the people position
    ptgpr_position = estimate_photographer_position(transformation_array, True)
    theta_init, phi_init = \
        get_spherical_coord(transformation_array[0, 1:4], transformation_array[0, 4:8], ptgpr_position)

    for idx in range(len(traj_list)):
        if idx % 10 == 0:
            print("generate the {}th frame mask".format(idx))

        term = traj_list[idx]
        # compute the relative rotation from initial fot current frame
        translation = transformation_array[idx, 1:4]
        rotation_quat = transformation_array[idx, 4:8]

        # ration the mask
        theta_cur, phi_cur = get_spherical_coord(translation, rotation_quat, ptgpr_position)

        rotation = [theta_cur - theta_init , phi_cur - phi_init]
        # print(rotation)
        # rotation
        if idx % 1 == 0:
            # output mask
            mask = np.copy(initial_mask)[..., np.newaxis]
            rotate_image_fast(mask, rotation)

            im = Image.fromarray(mask[..., 0])
            im.save(mask_output_path + r"{:04d}.jpg".format(idx))
            # plt.imshow(mask, interpolation='nearest')
            # plt.show()


if __name__ == "__main__":
    people2camera_meter = 0.1
    radius_meter = 0.65
    traj_file_path = "D:/workdata/KobeGarden6/openvslam_result_Apr_23/KobeGarden6_traj.csv"
    initial_mask_file_path = "D:/workdata/KobeGarden6/mask.png"
    mask_output_path = "D:/workdata/KobeGarden6/mask/"

    create_mask(initial_mask_file_path, traj_file_path, mask_output_path)
