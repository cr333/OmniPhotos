#!/bin/bash

# Output files:
# 1) map.msg: map information 
# 2) frame_trajectory.txt: camera pose information

echo "Start reconstructing camera pose."

PROGRAM_FILE="/mnt/sda1/workspace_linux/openvslam/build/run_camera_pose_reconstruction "
ORB_VOCAB="/mnt/sda1/workdata/openvslam_data/orb_vocab/orb_vocab.dbow2"

DATASET_DIR="/mnt/sda1/workdata/openvslam_data/openvslam_win32_version_test/"
OUTPUT_DIR="/mnt/sda1/workspace_linux/openvslam/result/"

DATASET_LIST="room_0 \
hotel_0"

for DATASET_NAME in $DATASET_LIST; do
    OUTPUT_MAP_DB="${OUTPUT_DIR}${DATASET_NAME}_map.msg"
    OUTPUT_MAP_DB_JSON="${OUTPUT_DIR}${DATASET_NAME}_map.json"
    OUTPUT_TRAJ="${OUTPUT_DIR}${DATASET_NAME}_traj.csv"

    CONFIG_FILE=${DATASET_DIR}${DATASET_NAME}"/config.yaml"
    INPUT_IMAGE_DIR=${DATASET_DIR}${DATASET_NAME}"/Input/"

    COMMON_PARAMETERS="--eval-log \
    --auto-term \
    --debug \
    --no-sleep \
    --frame-skip 1 \
    -v ${ORB_VOCAB} \
    -c ${CONFIG_FILE} \
    -i ${INPUT_IMAGE_DIR} \
    --map-db ${OUTPUT_MAP_DB}"

    #------------------
    echo "Step 1: Reconstructing the map"

    MAPPING_PARAMETERS="${COMMON_PARAMETERS} \
    -t map \
    --repeat-times 2"

    eval "${PROGRAM_FILE}${MAPPING_PARAMETERS}"

    #------------------
    echo "Step 2: Recontructing the camera pose"

     LOCATION_PARAMETERS="${COMMON_PARAMETERS} \
    -t trajectory \
    --trajectory_path ${OUTPUT_TRAJ}"

    eval "${PROGRAM_FILE} ${LOCATION_PARAMETERS}"

    #------------------
    # Convert the map msg to json & format the json
    eval "msgpack2json -d -i ${OUTPUT_MAP_DB} -o ${OUTPUT_MAP_DB_JSON}"

done
