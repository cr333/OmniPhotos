from matplotlib import pyplot as plt
import numpy as np
from skimage.feature import peak_local_max
from scipy import ndimage as ndi

def plot_heatmap(point_dicts, n_points, show=True, error="summed_errors",dot_coords=None,save_to=None):
    """

    :param point_dicts: listof(dicts)
    :param n_points: number of points in the original dataset (in the .traj file). Usually ~1k
    :param show: boolean.
    :param error: flatness_error, endpoint_error, perimeter_error
    :param dot_coords: np.array. shape=(n,2). array of coordinates to show as red dots on heatmap.
    :return: None
    """
    arr = np.zeros((n_points, n_points))
    for dct in point_dicts:
        arr[dct["interval"][0], dct["interval"][1]] = dct[error]
    plt.figure()
    plt.title(error)
    plt.imshow(arr)
    if dot_coords is not None:
        plt.plot(dot_coords[:, 1], dot_coords[:, 0], 'r.')
    plt.colorbar()
    if show:
        plt.show()
    if save_to:
        plt.savefig(save_to)
        return

def plot_points(list, show=True, save_to=None):
    fig = plt.figure()
    ax = fig.gca(projection='3d')
    """
    set all the coordinates in seperate lists
    and exclude the endpoints
    """
    xx = [arr[0] for arr in list[1:-1]]
    yy = [arr[1] for arr in list[1:-1]]
    zz = [arr[2] for arr in list[1:-1]]
    xx_end = [list[0][0], list[-1][0]]
    yy_end = [list[0][1], list[-1][1]]
    zz_end = [list[0][2], list[-1][2]]

    """
    plot the previously excluded endpoints in 
    a different colour.
    """
    ax.scatter(xx_end, yy_end, zz_end, color='r')

    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    ax.scatter(xx, yy, zz)

    """
    Set the axis limits so that the points
    are centered and the axes are correctly 
    scaled.
    """
    maxval = max(max(xx), max(yy), max(zz))

    points = np.array(list)
    centroid = np.mean(points,axis=0)
    ax.set_xlim(centroid[0] + maxval, centroid[0] - maxval)
    ax.set_ylim(centroid[1] + maxval, centroid[1] - maxval)
    ax.set_zlim(centroid[2] + maxval, centroid[2] - maxval)
    if save_to:
        plt.savefig(save_to)
        return

    if show:
        plt.show()

def plot_minima(im,show=True,save_to=None):

    # image_max is the dilation of im with a 20*20 structuring element
    # It is used within peak_local_max function
    image_max = ndi.maximum_filter(im, size=20, mode='constant')

    # Comparison between image_max and im to find the coordinates of local maxima
    coordinates = peak_local_max(im, min_distance=10,threshold_rel=0.1)

    # display results
    fig, axes = plt.subplots(1, 3, figsize=(8, 3), sharex=True, sharey=True)
    ax = axes.ravel()
    ax[0].imshow(im, cmap=plt.cm.gray)
    ax[0].axis('off')
    ax[0].set_title('Original')

    ax[1].imshow(image_max, cmap=plt.cm.gray)
    ax[1].axis('off')
    ax[1].set_title('Maximum filter')

    ax[2].imshow(im, cmap=plt.cm.gray)
    ax[2].autoscale(False)
    ax[2].plot(coordinates[:, 1], coordinates[:, 0], 'r.')
    ax[2].axis('off')
    ax[2].set_title('Peak local max')

    fig.tight_layout()
    if show:
        plt.show()
    if save_to:
        plt.savefig(save_to)