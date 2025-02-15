// SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
// Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef NVBLOX_ROS__CONVERSIONS__OCCUPANCY_CONVERSIONS_HPP_
#define NVBLOX_ROS__CONVERSIONS__OCCUPANCY_CONVERSIONS_HPP_

#include <nvblox/nvblox.h>
#include <vector>
#include <string>

namespace nvblox
{
namespace conversions
{

constexpr uint8_t kOccupancyGridPngUnknownValue = 205;
constexpr uint8_t kOccupancyGridPngFreeValue = 254;
constexpr uint8_t kOccupancyGridPngOccupiedValue = 0;

/// Save the given occupancy grid to png file specified by png_path.
///
/// @param png_path           Path to the png file
/// @param free_thresh        Threshold at which the occupancy grid is considered free
/// @param occupied_thresh    Threshold at which the occupancy grid is considered occuppied
/// @param height             Height of the occupancy grid
/// @param width              Width of the occupancy grid
/// @param occupancy_grid     The occupancy grid to save as png
void saveOccupancyGridAsPng(
  const std::string png_path, const float free_thresh,
  const float occupied_thresh, const size_t height, const size_t width,
  const std::vector<signed char> & occupancy_grid);


/// Save the given occupancy grid metadata to yaml_path.
///
/// @param yaml_path          Path of yaml file to be saved
/// @param image_name         Name of the image file this yaml file refers to
/// @param voxel_size         Size of each voxel
/// @param origin_x           Origin of the png file (X axis)
/// @param origin_y           Origin of the png file (Y axis)
/// @param free_thresh        Threshold at which the occupancy grid is considered free
/// @param occupied_thresh    Threshold at which the occupancy grid is considered occuppied
void saveOccupancyGridYaml(
  const std::string yaml_path, const std::string image_name,
  const float voxel_size, const float origin_x, const float origin_y,
  const float free_thresh, const float occupied_thresh);

}  // namespace conversions
}  // namespace nvblox

#endif  // NVBLOX_ROS__CONVERSIONS__OCCUPANCY_CONVERSIONS_HPP_
