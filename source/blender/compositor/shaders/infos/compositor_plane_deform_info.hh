/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "gpu_shader_create_info.hh"

GPU_SHADER_CREATE_INFO(compositor_plane_deform_mask)
LOCAL_GROUP_SIZE(16, 16)
PUSH_CONSTANT(float4x4, homography_matrix)
IMAGE(0, SFLOAT_16, write, image2D, mask_img)
COMPUTE_SOURCE("compositor_plane_deform_mask.glsl")
DO_STATIC_COMPILATION()
GPU_SHADER_CREATE_END()

GPU_SHADER_CREATE_INFO(compositor_plane_deform_shared)
LOCAL_GROUP_SIZE(16, 16)
PUSH_CONSTANT(float4x4, homography_matrix)
SAMPLER(0, sampler2D, input_tx)
SAMPLER(1, sampler2D, mask_tx)
IMAGE(0, SFLOAT_16_16_16_16, write, image2D, output_img)
GPU_SHADER_CREATE_END()

GPU_SHADER_CREATE_INFO(compositor_plane_deform)
ADDITIONAL_INFO(compositor_plane_deform_shared)
DEFINE_VALUE("SAMPLER_FUNCTION", "texture")
COMPUTE_SOURCE("compositor_plane_deform.glsl")
DO_STATIC_COMPILATION()
GPU_SHADER_CREATE_END()

GPU_SHADER_CREATE_INFO(compositor_plane_deform_bicubic)
ADDITIONAL_INFO(compositor_plane_deform_shared)
DEFINE_VALUE("SAMPLER_FUNCTION", "texture_bicubic")
COMPUTE_SOURCE("compositor_plane_deform.glsl")
DO_STATIC_COMPILATION()
GPU_SHADER_CREATE_END()

GPU_SHADER_CREATE_INFO(compositor_plane_deform_anisotropic)
ADDITIONAL_INFO(compositor_plane_deform_shared)
COMPUTE_SOURCE("compositor_plane_deform_anisotropic.glsl")
DO_STATIC_COMPILATION()
GPU_SHADER_CREATE_END()

GPU_SHADER_CREATE_INFO(compositor_plane_deform_motion_blur_mask)
LOCAL_GROUP_SIZE(16, 16)
PUSH_CONSTANT(int, number_of_motion_blur_samples)
UNIFORM_BUF(0, float4x4, homography_matrices[64])
IMAGE(0, SFLOAT_16, write, image2D, mask_img)
COMPUTE_SOURCE("compositor_plane_deform_motion_blur_mask.glsl")
DO_STATIC_COMPILATION()
GPU_SHADER_CREATE_END()

GPU_SHADER_CREATE_INFO(compositor_plane_deform_motion_blur)
LOCAL_GROUP_SIZE(16, 16)
PUSH_CONSTANT(int, number_of_motion_blur_samples)
UNIFORM_BUF(0, float4x4, homography_matrices[64])
SAMPLER(0, sampler2D, input_tx)
SAMPLER(1, sampler2D, mask_tx)
IMAGE(0, SFLOAT_16_16_16_16, write, image2D, output_img)
COMPUTE_SOURCE("compositor_plane_deform_motion_blur.glsl")
DO_STATIC_COMPILATION()
GPU_SHADER_CREATE_END()
