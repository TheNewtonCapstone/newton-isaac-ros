/*
Copyright 2022 NVIDIA CORPORATION

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma once

namespace nvblox {

template <typename VoxelType>
typename VoxelBlock<VoxelType>::Ptr VoxelBlock<VoxelType>::allocate(
    MemoryType memory_type) {
  return allocateAsync(memory_type, CudaStreamOwning());
}

template <typename VoxelType>
typename VoxelBlock<VoxelType>::Ptr VoxelBlock<VoxelType>::allocateAsync(
    MemoryType memory_type, const CudaStream& cuda_stream) {
  Ptr voxel_block_ptr =
      make_unified_async<VoxelBlock>(memory_type, cuda_stream);
  initAsync(voxel_block_ptr.get(), memory_type, cuda_stream);

  return voxel_block_ptr;
}
template <typename VoxelType>
void VoxelBlock<VoxelType>::initAsync(VoxelBlock<VoxelType>* block_ptr,
                                      const MemoryType memory_type,
                                      const CudaStream& cuda_stream) {
  if (memory_type == MemoryType::kDevice) {
    setBlockBytesZeroOnGPUAsync(block_ptr, cuda_stream);
  } else {
    *block_ptr = VoxelBlock<VoxelType>();
  }
}

template <typename VoxelType>
const VoxelType& VoxelBlock<VoxelType>::operator()(const Index3D& idx) const {
  return voxels[idx.x()][idx.y()][idx.z()];
}

template <typename VoxelType>
VoxelType& VoxelBlock<VoxelType>::operator()(const Index3D& idx) {
  return voxels[idx.x()][idx.y()][idx.z()];
}

template <typename VoxelType>
typename VoxelBlock<VoxelType>::iterator VoxelBlock<VoxelType>::begin() {
  return iterator(&voxels[0][0][0]);
}

template <typename VoxelType>
typename VoxelBlock<VoxelType>::const_iterator VoxelBlock<VoxelType>::cbegin()
    const {
  return const_iterator(&voxels[0][0][0]);
}

template <typename VoxelType>
typename VoxelBlock<VoxelType>::iterator VoxelBlock<VoxelType>::end() {
  return iterator(&voxels[0][0][0] + kNumVoxels);
}

template <typename VoxelType>
typename VoxelBlock<VoxelType>::const_iterator VoxelBlock<VoxelType>::cend()
    const {
  return const_iterator(&voxels[0][0][0] + kNumVoxels);
}

// Initialization specialization for ColorVoxel which is initialized to gray
// with zero weight
template <>
inline void VoxelBlock<ColorVoxel>::initAsync(VoxelBlock<ColorVoxel>* block_ptr,
                                              const MemoryType memory_type,
                                              const CudaStream& cuda_stream) {
  if (memory_type == MemoryType::kDevice) {
    setColorBlockGrayOnGPUAsync(block_ptr, cuda_stream);
  } else {
    *block_ptr = VoxelBlock<VoxelType>();
  }
}

template <typename BlockType>
void setBlockBytesZeroOnGPUAsync(BlockType* block_device_ptr,
                                 const CudaStream& cuda_stream) {
  checkCudaErrors(
      cudaMemsetAsync(block_device_ptr, 0, sizeof(BlockType), cuda_stream));
}

template <typename VoxelType>
constexpr size_t sizeInBytes(const VoxelBlock<VoxelType>*) {
  return sizeof(VoxelBlock<VoxelType>);
}

}  // namespace nvblox
