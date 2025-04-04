# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

from isaac_ros_launch_utils.all_types import *
import isaac_ros_launch_utils as lu

from nvblox_ros_python_utils.nvblox_launch_utils import NvbloxMode, NvbloxCamera, NvbloxPeopleSegmentation
from nvblox_ros_python_utils.nvblox_constants import NVBLOX_CONTAINER_NAME


def generate_launch_description() -> LaunchDescription:
    args = lu.ArgumentContainer()
    args.add_arg(
        'rosbag', 'None', description='Path to rosbag (running on sensor if not set).', cli=True)
    args.add_arg('rosbag_args', '',
                 description='Additional args for ros2 bag play.', cli=True)
    args.add_arg('log_level', 'info', choices=[
                 'debug', 'info', 'warn'], cli=True)
    args.add_arg('num_cameras', 1,
                 description='How many cameras to use.', cli=True)
    args.add_arg('camera_serial_numbers', '',
                 description='List of the serial no of the extra cameras. (comma separated)',
                 cli=True)
    args.add_arg(
        'multicam_urdf_path',
        lu.get_path('nvblox_examples_bringup',
                    'config/urdf/4_realsense_carter_example_calibration.urdf.xacro'),
        description='Path to a URDF file describing the camera rig extrinsics. Only used in multicam.',
        cli=True)
    args.add_arg(
        'mode',
        default=NvbloxMode.static,
        choices=NvbloxMode.names(),
        description='The nvblox mode.',
        cli=True)
    args.add_arg(
        'attach_to_container',
        'False',
        description='Add components to an existing component container.',
        cli=True)
    args.add_arg(
        'container_name',
        NVBLOX_CONTAINER_NAME,
        description='Name of the component container.')
    args.add_arg(
        'run_realsense',
        'True',
        description='Launch Realsense drivers')
    args.add_arg(
        'navigation',
        True,
        description='Whether to enable nav2 for navigation.',
        cli=True)
    actions = args.get_launch_actions()

    # Globally set use_sim_time if we're running from bag or sim
    actions.append(
        SetParameter('use_sim_time', True, condition=IfCondition(lu.is_valid(args.rosbag))))

    actions.append(
        lu.include(
            'nvblox_examples_bringup',
            'launch/navigation/nvblox_newton_navigation.launch.py',
            launch_arguments={
                'container_name': NVBLOX_CONTAINER_NAME,
                'mode': args.mode,
            },
            condition=IfCondition(lu.is_true(args.navigation))))

    # Single or Multi-realsense
    is_multi_cam = UnlessCondition(lu.is_equal(args.num_cameras, '1'))
    camera_mode = lu.if_else_substitution(
        lu.is_equal(args.num_cameras, '1'),
        str(NvbloxCamera.realsense),
        str(NvbloxCamera.multi_realsense)
    )
    # Only up to 4 Realsenses is supported.
    actions.append(
        lu.assert_condition(
            'Up to 4 cameras have been tested! num_cameras must be less than 5.',
            IfCondition(PythonExpression(['int("', args.num_cameras, '") > 4']))),
    )

    run_rs_driver = UnlessCondition(
        OrSubstitution(lu.is_valid(args.rosbag), lu.is_false(args.run_realsense)))
    
    # Realsense
    actions.append(
        lu.include(
            'nvblox_examples_bringup',
            'launch/sensors/realsense.launch.py',
            launch_arguments={
                'container_name': args.container_name,
                'camera_serial_numbers': args.camera_serial_numbers,
                'num_cameras': args.num_cameras,
            },
            condition=run_rs_driver))

    # Visual SLAM
    actions.append(
        lu.include(
            'nvblox_examples_bringup',
            'launch/perception/vslam.launch.py',
            launch_arguments={
                'container_name': args.container_name,
                'camera': camera_mode,
            },
            # Delay for 1 second to make sure that the static topics from the rosbag are published.
            delay=1.0,
        ))

    # Nvblox
    actions.append(
        lu.include(
            'nvblox_examples_bringup',
            'launch/perception/nvblox.launch.py',
            launch_arguments={
                'container_name': args.container_name,
                'mode': args.mode,
                'camera': camera_mode,
                'num_cameras': args.num_cameras,
            },
        ))
    
    # EKF node for mashing IMU and vSLAM
    actions.append(
        Node(
            package='robot_localization',
            executable='ekf_node',
            name='ekf_filter_node',   
            output='screen',
            parameters=[{
                'use_sim_time': False,
                'frequency': 30.0,
                'sensor_timeout': 0.1,
                'two_d_mode': False,
                'map_frame': 'map',
                'odom_frame': 'odom',
                'base_link_frame': 'camera0_link',
                'world_frame': 'odom',
                
                # Visual SLAM odometry input
                'odom0': '/visual_slam/tracking/odometry',
                'odom0_config': [True, True, False,  # x, y, z position
                                False, False, True,  # roll, pitch, yaw
                                False, False, False,   # x, y, z velocity
                                False, False, True,  # roll, pitch, yaw velocity
                                False, False, False], # x, y, z acceleration
                'odom0_queue_size': 10,
                'odom0_nodelay': True,
                'odom0_differential': False,
                'odom0_relative': False,
                
                # RealSense IMU input
                'imu0': '/imu_data',
                'imu0_config': [False, False, False,  # x, y, z position
                               True, True, True,      # roll, pitch, yaw
                               False, False, False,   # x, y, z velocity (current IMU does not have any)
                               True, True, True,      # roll, pitch, yaw velocity
                               True, True, True],     # x, y, z acceleration
                'imu0_queue_size': 10,
                'imu0_nodelay': True,
                'imu0_differential': False,
                'imu0_relative': False,
                'imu0_remove_gravitational_acceleration': True,
            }],
            remappings=[
                ('/odometry/filtered', '/odom'),
            ], 
        ),
    )

    # TF transforms for multi-realsense
    actions.append(
        lu.add_robot_description(robot_calibration_path=args.multicam_urdf_path,
                                 condition=is_multi_cam)
    )

    # Play ros2bag
    actions.append(
        lu.play_rosbag(
            bag_path=args.rosbag,
            additional_bag_play_args=args.rosbag_args,
            condition=IfCondition(lu.is_valid(args.rosbag))))
 
    # Container
    # NOTE: By default (attach_to_container:=False) we launch a container which all nodes are
    # added to, however, we expose the option to not launch a container, and instead attach to
    # an already running container. The reason for this is that when running live on multiple
    # realsenses we have experienced unreliability in the bringup of multiple realsense drivers.
    # To (partially) mitigate this issue the suggested workflow for multi-realsenses is to:
    # 1. Launch RS (cameras & splitter) and start a component_container
    # 2. Launch nvblox + cuvslam and attached to the above running component container

    actions.append(
        lu.component_container(
            NVBLOX_CONTAINER_NAME, condition=UnlessCondition(args.attach_to_container),
            log_level=args.log_level))

    return LaunchDescription(actions)