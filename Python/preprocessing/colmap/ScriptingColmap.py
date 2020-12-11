#!/usr/bin/env python
import os
import re
import subprocess
import tempfile

python_dir = os.path.dirname(os.path.realpath(__file__))

# ---- Support functions -----------------------------------------------------------------------------------------------

def override_ini_parameters(ini, params):
    if not params:
        return ini  # nothing to do

    for (key, value) in params.items():
        if '.' in key:  # Update a parameter in a section.
            section, setting = key.split('.')
            section_text_old = re.findall(r'(\[%s\][^\[]+)' % section, ini, re.M)[0]
            section_text_new = re.sub(setting + '=.*', setting + '=' + str(value), section_text_old)
            ini = ini.replace(section_text_old, section_text_new)
        elif key + '=' in ini:  # Update an existing parameter
            ini = re.sub(key + '=.*', key + '=' + str(value), ini)
        else:  # Add a parameter.
            ini = key + '=' + str(value) + '\n' + ini

    return ini

ini_file_list = [] # save the each step *.ini files

def save_ini(contents, ini_save_path, add_project_ini = True):
    # If there is no path given, generate a temporary one.
    if ini_save_path is None:
        with tempfile.NamedTemporaryFile() as f:
            ini_save_path = f.name

    if add_project_ini:
        ini_file_list.append(ini_save_path)

    # Write contents to ini file.
    with open(ini_save_path, 'w') as f:
        f.write(contents)

    return ini_save_path

def reduce_ini(output_path):
    """
    collect all ini file in ini_file_list and output to project.ini file
    :param output_path: the final ini file absolute path
    """
    with open(output_path, "w") as output_file_handle:
        for mapped_ini_file_term in ini_file_list:
            with open(mapped_ini_file_term, "r") as input_file_handle:
                output_file_handle.write(input_file_handle.read())

# ---- COLMAP functions ------------------------------------------------------------------------------------------------

def check_version(colmap_bin):
    """
    check the version of COLMAP. The corresponding relationship between commit_id and official version is:
    - 3.6-dev.3 <----> 0dce1db

    the script just test on COLMAP 3.6-dev.3
    """
    output_str = subprocess.getoutput(colmap_bin + " --help")
    commit_number = re.findall(r'(Commit [a-z0-9]*)', output_str)[0][7:]
    version_list = ["0dce1db"]
    if not commit_number in version_list:
        msg = "!!!!!!!!!!warning!!!!!!!!!!! \n"
        msg = msg + "The COLMAP is build with commit number : {}".format(commit_number) + "\n"
        msg = msg + "now just support {}".format(version_list) + "\n"
        msg = msg + "please download latest version of COLMAP from https://github.com/colmap/colmap/releases"
        print(msg)
        #raise Exception(msg)

def show_model(colmap_bin, import_path, database_path, image_path):
    """
    :param colmap_bin: 
    :param import_path: the directory of camera.bin(.txt) etc.
    :param database_path: path of *.db file
    :param image_path: the directory of images
    """
    # Call COLMAP.
    colmap_command = [colmap_bin, "gui", "--import_path", import_path, "--database_path", database_path, "--image_path", image_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)


def feature_extractor(colmap_bin, database_path, image_path, ini_save_path=None, params=None):

    # Find and read template INI.
    input_ini_file = os.path.join(python_dir, 'template_inis', 'colmap_feature_extraction.ini')
    with open(input_ini_file, 'r') as f:
        colmap_ini = f.read()

    # Update some parameters, if requested.
    colmap_ini = override_ini_parameters(colmap_ini, params)

    # Add database and image directory to the INI.
    # NB. Forward slashes in paths are expected by the GUI.
    colmap_ini = override_ini_parameters(colmap_ini, {
        'database_path': database_path.replace('\\', '/'),
        'image_path': image_path.replace('\\', '/')
    })

    # Save INI file.
    ini_save_path = save_ini(colmap_ini, ini_save_path)

    # Call COLMAP.
    colmap_command = [colmap_bin, "feature_extractor", "--project_path", ini_save_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)


def exhaustive_matcher(colmap_bin, database_path, ini_save_path=None, params=None):

    # Find and read template INI.
    input_ini_file = os.path.join(python_dir, 'template_inis', 'colmap_exhaustive_matcher.ini')
    with open(input_ini_file, 'r') as f:
        colmap_ini = f.read()

    # Update some parameters, if requested.
    colmap_ini = override_ini_parameters(colmap_ini, params)

    # Add database and image directory to the INI.
    # NB. Forward slashes in paths are expected by the GUI.
    colmap_ini = override_ini_parameters(colmap_ini, {
        'database_path': database_path.replace('\\', '/')
    })

    # Save INI file.
    ini_save_path = save_ini(colmap_ini, ini_save_path)

    # Call COLMAP.
    colmap_command = [colmap_bin, "exhaustive_matcher", "--project_path", ini_save_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)


def vocab_tree_matcher(colmap_bin, database_path, ini_save_path=None, params=None):

    # Find and read template INI.
    input_ini_file = os.path.join(python_dir, 'template_inis', 'colmap_vocab_tree_matcher.ini')
    with open(input_ini_file, 'r') as f:
        colmap_ini = f.read()

    # Update some parameters, if requested.
    colmap_ini = override_ini_parameters(colmap_ini, params)

    # Add database and image directory to the INI.
    # NB. Forward slashes in paths are expected by the GUI.
    colmap_ini = override_ini_parameters(colmap_ini, {
        'database_path': database_path.replace('\\', '/')
    })

    # Save INI file.
    ini_save_path = save_ini(colmap_ini, ini_save_path)

    # Call COLMAP.
    colmap_command = [colmap_bin, "vocab_tree_matcher", "--project_path", ini_save_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)



def mapper(colmap_bin, database_path, image_path, output_path, input_path, image_list_path, ini_save_path=None, params=None):
    """
    :param database_path: the file path to *.db
    :param image_path: the directory of image file
    :param input_path: the folder contain cameras.bin/txt, images.bin/txt & point3D.bin/txt files, if do not have those file set to ""
    :param output_path: 
    :param image_list_path: if it empty, load it from database *.db file
    """
    # Find and read template INI.
    input_ini_file = os.path.join(python_dir, 'template_inis', 'colmap_mapper.ini')
    with open(input_ini_file, 'r') as f:
        colmap_ini = f.read()

    if not os.path.exists(output_path):
        os.makedirs(output_path, exist_ok=True)

    # Update some parameters, if requested.
    colmap_ini = override_ini_parameters(colmap_ini, params)

    # Add database and image directory to the INI.
    # NB. Forward slashes in paths are expected by the GUI.
    colmap_ini = override_ini_parameters(colmap_ini, {
        'database_path': database_path.replace('\\', '/'),
        'image_path': image_path.replace('\\', '/'),
        'output_path': output_path.replace('\\', '/'),
        'input_path': input_path.replace('\\', '/'),
        'image_list_path': image_list_path.replace('\\', '/')
    })

    # Save INI file.
    ini_save_path = save_ini(colmap_ini, ini_save_path)

    # Call COLMAP.
    colmap_command = [colmap_bin, "mapper", "--project_path", ini_save_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)


def bundle_adjuster(colmap_bin, input_path, output_path, ini_save_path=None, params=None):
    # Find and read template INI.
    input_ini_file = os.path.join(python_dir, 'template_inis', 'colmap_bundle_adjuster.ini')
    with open(input_ini_file, 'r') as f:
        colmap_ini = f.read()

    if not os.path.exists(output_path):
        os.makedirs(output_path, exist_ok=True)

    # Update some parameters, if requested.
    colmap_ini = override_ini_parameters(colmap_ini, params)

    # Add database and image directory to the INI.
    # NB. Forward slashes in paths are expected by the GUI.
    colmap_ini = override_ini_parameters(colmap_ini, {
        'input_path': input_path.replace('\\', '/'),
        'output_path': output_path.replace('\\', '/')
    })

    # Save INI file.
    ini_save_path = save_ini(colmap_ini, ini_save_path)

    # Call COLMAP.
    colmap_command = [colmap_bin, "bundle_adjuster", "--project_path", ini_save_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)

def model_converter(colmap_bin, input_path, output_path, output_type='TXT', ini_save_path=None, params=None):
    """
    Convert the model file from *.bin to *.txt/ply...
    :param input_path: the input directory path of *.bin file  
    :param output_path: the path of output directory
    :param output_type: the output file type, option list is {BIN, TXT, NVM, Bundler, VRML, PLY}
    """
    # Find and read template INI.
    input_ini_file = os.path.join(python_dir, 'template_inis', 'colmap_model_converter.ini')
    with open(input_ini_file, 'r') as f:
        colmap_ini = f.read()

    if not os.path.exists(output_path):
        os.makedirs(output_path, exist_ok=True)

    # Update some parameters, if requested.
    colmap_ini = override_ini_parameters(colmap_ini, params)

    # Add database and image directory to the INI.
    # NB. Forward slashes in paths are expected by the GUI.
    colmap_ini = override_ini_parameters(colmap_ini, {
        'input_path': input_path.replace('\\', '/'),
        'output_path': output_path.replace('\\', '/'),
        'output_type': output_type
    })

    # Save INI file.
    ini_save_path = save_ini(colmap_ini, ini_save_path, add_project_ini = False)

    # Call COLMAP.
    colmap_command = [colmap_bin, "model_converter", "--project_path", ini_save_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)

def model_converter_bin2ply(colmap_bin, input_path, output_path):
    """
    extract the 3D point cloud in the *.bin files, and export to PLY file
    :param colmap_bin:
    :param input_path: the path of folder containing *.bin files
    :param output_path: the absolute path of output *.ply file
    :return:
    """
    # Call COLMAP.
    colmap_command = [colmap_bin, "model_converter", \
                      "--input_path", input_path, \
                      "--output_path", output_path, \
                      "--output_type", "PLY"]
    # print(colmap_command)
    if 0 != subprocess.check_call(colmap_command):
        msg = "convert bin to ply error! bin {}, ply output {}".format(input_path, output_path)
        print(msg)
        raise Exception(msg)


def point_triangulator(colmap_bin, database_path, image_path, import_path, export_path, ini_save_path=None, params=None):

    # Find and read template INI.
    input_ini_file = os.path.join(python_dir, 'template_inis', 'colmap_point_triangulator.ini')
    with open(input_ini_file, 'r') as f:
        colmap_ini = f.read()

    if not os.path.exists(export_path):
        os.makedirs(export_path, exist_ok=True)

    # Update some parameters, if requested.
    colmap_ini = override_ini_parameters(colmap_ini, params)

    # Add database and image directory to the INI.
    # NB. Forward slashes in paths are expected by the GUI.
    colmap_ini = override_ini_parameters(colmap_ini, {
        'database_path': database_path.replace('\\', '/'),
        'image_path': image_path.replace('\\', '/'),
        'import_path': import_path.replace('\\', '/'),
        'export_path': export_path.replace('\\', '/')
    })

    # Save INI file.
    ini_save_path = save_ini(colmap_ini, ini_save_path)

    # Call COLMAP.
    colmap_command = [colmap_bin, "point_triangulator", "--project_path", ini_save_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)


def image_registrator(colmap_bin, database_path, input_path, output_path, ini_save_path=None, params=None):

    # Find and read template INI.
    input_ini_file = os.path.join(python_dir, 'template_inis', 'colmap_image_registrator.ini')
    with open(input_ini_file, 'r') as f:
        colmap_ini = f.read()

    if not os.path.exists(output_path):
        os.makedirs(output_path, exist_ok=True)

    # Update some parameters, if requested.
    colmap_ini = override_ini_parameters(colmap_ini, params)

    # Add database and image directory to the INI.
    # NB. Forward slashes in paths are expected by the GUI.
    colmap_ini = override_ini_parameters(colmap_ini, {
        'database_path': database_path.replace('\\', '/'),
        'input_path': input_path.replace('\\', '/'),
        'output_path': output_path.replace('\\', '/')
    })

    # Save INI file.
    ini_save_path = save_ini(colmap_ini, ini_save_path)

    # Call COLMAP.
    colmap_command = [colmap_bin, "image_registrator", "--project_path", ini_save_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)

def image_undistorter(colmap_bin, image_path, input_path, output_path, ini_save_path=None, params=None):

    # Find and read template INI.
    input_ini_file = os.path.join(python_dir, 'template_inis', 'colmap_image_undistorter.ini')
    with open(input_ini_file, 'r') as f:
        colmap_ini = f.read()

    if not os.path.exists(output_path):
        os.makedirs(output_path, exist_ok=True)

    # Update some parameters, if requested.
    colmap_ini = override_ini_parameters(colmap_ini, params)

    # Add database and image directory to the INI.
    # NB. Forward slashes in paths are expected by the GUI.
    colmap_ini = override_ini_parameters(colmap_ini, {
        'image_path': image_path.replace('\\', '/'),
        'input_path': input_path.replace('\\', '/'),
        'output_path': output_path.replace('\\', '/')
    })

    # Save INI file.
    ini_save_path = save_ini(colmap_ini, ini_save_path)

    # Call COLMAP.
    colmap_command = [colmap_bin, "image_undistorter", "--project_path", ini_save_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)


def patch_match_stereo(colmap_bin, workspace_path, ini_save_path=None, params=None):

    # Find and read template INI.
    input_ini_file = os.path.join(python_dir, 'template_inis', 'colmap_patch_match_stereo.ini')
    with open(input_ini_file, 'r') as f:
        colmap_ini = f.read()

    if not os.path.exists(workspace_path):
        os.makedirs(workspace_path, exist_ok=True)

    # Update some parameters, if requested.
    colmap_ini = override_ini_parameters(colmap_ini, params)

    # Add database and image directory to the INI.
    # NB. Forward slashes in paths are expected by the GUI.
    colmap_ini = override_ini_parameters(colmap_ini, {
        'workspace_path': workspace_path.replace('\\', '/')
    })

    # Save INI file.
    ini_save_path = save_ini(colmap_ini, ini_save_path)

    # Call COLMAP.
    colmap_command = [colmap_bin, "patch_match_stereo", "--project_path", ini_save_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)

def stereo_fusion(colmap_bin, workspace_path, output_path, ini_save_path=None, params=None):

    # Find and read template INI.
    input_ini_file = os.path.join(python_dir, 'template_inis', 'colmap_stereo_fusion.ini')
    with open(input_ini_file, 'r') as f:
        colmap_ini = f.read()

    if not os.path.exists(workspace_path):
        os.makedirs(workspace_path, exist_ok=True)

    # Update some parameters, if requested.
    colmap_ini = override_ini_parameters(colmap_ini, params)

    # Add database and image directory to the INI.
    # NB. Forward slashes in paths are expected by the GUI.
    colmap_ini = override_ini_parameters(colmap_ini, {
        'workspace_path': workspace_path.replace('\\', '/'),
        'output_path': output_path.replace('\\', '/')
    })

    # Save INI file.
    ini_save_path = save_ini(colmap_ini, ini_save_path)

    # Call COLMAP.
    colmap_command = [colmap_bin, "stereo_fusion", "--project_path", ini_save_path]
    # print(colmap_command)
    subprocess.check_call(colmap_command)
