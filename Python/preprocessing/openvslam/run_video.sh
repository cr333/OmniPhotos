#!/bin/bash

# Output files:
# 1) map.msg: map information 
# 2) frame_trajectory.txt: camera pose information

echo "Start reconstructing camera pose."

DATASET_LIST="BeijingBeihai4 \
BeijingBeihai4_00"

PROGRAM_FILE="/mnt/sda1/workspace_linux/openvslam/build/run_camera_pose_reconstruction_video "
ORB_VOCAB="/mnt/sda1/workdata/openvslam_data/orb_vocab/orb_vocab.dbow2"

DATASET_ROOT_DIR="/mnt/sda1/workdata/openvslam_data/openvslam_win32_version_test/"

OUTPUT_DIR="/mnt/sda1/workspace_linux/openvslam/result/"

for DATASET_NAME in $DATASET_LIST; do
    DATASET_DIR="${DATASET_ROOT_DIR}${DATASET_NAME}/"
    CONFIG_FILE=${DATASET_DIR}"config.yaml"
    MASK_PATH=${DATASET_DIR}"mask/"

    for VIDEO_FILENAME in ${DATASET_DIR}*-flowstate.mp4; do
        echo ${VIDEO_FILENAME}
    done

    #VIDEO_FILENAME="${DATASET_DIR}VID_20190610_181806_00_047-flowstate-lock.mp4"

    INPUT_VIDEO_PATH=${VIDEO_FILENAME}
    OUTPUT_MAP_DB="${OUTPUT_DIR}${DATASET_NAME}_map.msg"
    OUTPUT_MAP_DB_JSON="${OUTPUT_DIR}${DATASET_NAME}_map.json"
    OUTPUT_TRAJ="${OUTPUT_DIR}${DATASET_NAME}_traj.csv"

    COMMON_PARAMETERS="--eval-log \
    --auto-term \
    --debug \
    --no-sleep \
    --frame-skip 1 \
    -v ${ORB_VOCAB} \
    -c ${CONFIG_FILE} \
    -i ${INPUT_VIDEO_PATH} \
    --downsample_scalar 1.0 \
    --map-db ${OUTPUT_MAP_DB}"

    #------------------
    echo "Step 1: Reconstructing the map"

    MAPPING_PARAMETERS="${COMMON_PARAMETERS} \
    -t slam \
    --repeat-times 2 \
    --mask ${MASK_PATH}"

    eval "${PROGRAM_FILE}${MAPPING_PARAMETERS}"

    #------------------
    echo "Step 2: Reconstructing the camera pose"

    LOCATION_PARAMETERS="${COMMON_PARAMETERS} \
    -t localization \
    --trajectory_path ${OUTPUT_TRAJ}"

    eval "${PROGRAM_FILE} ${LOCATION_PARAMETERS}"

    #------------------
    # Convert the map msg to json & format the json
    eval "msgpack2json -d -i ${OUTPUT_MAP_DB} -o ${OUTPUT_MAP_DB_JSON}"

done
