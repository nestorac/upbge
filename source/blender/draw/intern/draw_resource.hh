/* SPDX-FileCopyrightText: 2022 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

/** \file
 * \ingroup draw
 *
 * Component / Object level resources like object attributes, matrices, visibility etc...
 * Each of them are reference by resource index (#ResourceHandle).
 */

#include "BLI_bounds.hh"
#include "BLI_math_base.h"
#include "BLI_math_matrix.hh"

#include "BKE_curve.hh"
#include "BKE_duplilist.hh"
#include "BKE_mesh.h"
#include "BKE_object.hh"
#include "BKE_volume.hh"
#include "BLI_hash.h"
#include "DNA_curve_types.h"
#include "DNA_layer_types.h"
#include "DNA_meta_types.h"
#include "DNA_object_types.h"

#include "DRW_render.hh"
#include "draw_handle.hh"
#include "draw_shader_shared.hh"

/* -------------------------------------------------------------------- */
/** \name ObjectMatrices
 * \{ */

inline void ObjectMatrices::sync(const Object &object)
{
  model = object.object_to_world();
  model_inverse = object.world_to_object();
}

inline void ObjectMatrices::sync(const float4x4 &model_matrix)
{
  model = model_matrix;
  model_inverse = blender::math::invert(model_matrix);
}

inline std::ostream &operator<<(std::ostream &stream, const ObjectMatrices &matrices)
{
  stream << "ObjectMatrices(" << std::endl;
  stream << "model=" << matrices.model << ", " << std::endl;
  stream << "model_inverse=" << matrices.model_inverse << ")" << std::endl;
  return stream;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name ObjectInfos
 * \{ */

inline void ObjectInfos::sync()
{
  object_attrs_len = 0;
  object_attrs_offset = 0;

  flag = eObjectInfoFlag::OBJECT_NO_INFO;
}

inline void ObjectInfos::sync(const blender::draw::ObjectRef ref,
                              bool is_active_object,
                              bool is_active_edit_mode)
{
  object_attrs_len = 0;
  object_attrs_offset = 0;
  light_and_shadow_set_membership = 0;

  LightLinking *light_linking = ref.light_linking();
  if (light_linking) {
    light_and_shadow_set_membership |= light_linking->runtime.receiver_light_set;
    light_and_shadow_set_membership |= light_linking->runtime.blocker_shadow_set << 8;
  }

  bool is_holdout = (ref.object->base_flag & BASE_HOLDOUT) ||
                    (ref.object->visibility_flag & OB_HOLDOUT);

  ob_color = ref.object->color;
  index = ref.object->index;
  SET_FLAG_FROM_TEST(flag, is_active_object, eObjectInfoFlag::OBJECT_ACTIVE);
  SET_FLAG_FROM_TEST(
      flag, ref.object->base_flag & BASE_SELECTED, eObjectInfoFlag::OBJECT_SELECTED);
  SET_FLAG_FROM_TEST(
      flag, ref.object->base_flag & BASE_FROM_DUPLI, eObjectInfoFlag::OBJECT_FROM_DUPLI);
  SET_FLAG_FROM_TEST(
      flag, ref.object->base_flag & BASE_FROM_SET, eObjectInfoFlag::OBJECT_FROM_SET);
  SET_FLAG_FROM_TEST(
      flag, ref.object->transflag & OB_NEG_SCALE, eObjectInfoFlag::OBJECT_NEGATIVE_SCALE);
  SET_FLAG_FROM_TEST(flag, is_holdout, eObjectInfoFlag::OBJECT_HOLDOUT);
  SET_FLAG_FROM_TEST(flag, is_active_edit_mode, eObjectInfoFlag::OBJECT_ACTIVE_EDIT_MODE);

  if (ref.object->shadow_terminator_normal_offset > 0.0f) {
    using namespace blender::math;
    shadow_terminator_geometry_offset = ref.object->shadow_terminator_geometry_offset;
    shadow_terminator_normal_offset = ref.object->shadow_terminator_normal_offset *
                                      reduce_max(to_scale(ref.object->object_to_world()));
  }
  else {
    shadow_terminator_geometry_offset = 0.0f;
    shadow_terminator_normal_offset = 0.0f;
  }

  random = ref.random();

  if (ref.object->data == nullptr) {
    orco_add = float3(0.0f);
    orco_mul = float3(1.0f);
    return;
  }

  switch (GS(reinterpret_cast<ID *>(ref.object->data)->name)) {
    case ID_VO: {
      std::optional<const blender::Bounds<float3>> bounds = BKE_volume_min_max(
          &DRW_object_get_data_for_drawing<const Volume>(*ref.object));
      if (bounds) {
        orco_add = blender::math::midpoint(bounds->min, bounds->max);
        orco_mul = (bounds->max - bounds->min) * 0.5f;
      }
      else {
        orco_add = float3(0.0f);
        orco_mul = float3(1.0f);
      }
      break;
    }
    case ID_ME: {
      BKE_mesh_texspace_get(
          &DRW_object_get_data_for_drawing<Mesh>(*ref.object), orco_add, orco_mul);
      break;
    }
    case ID_CU_LEGACY: {
      Curve &cu = DRW_object_get_data_for_drawing<Curve>(*ref.object);
      BKE_curve_texspace_ensure(&cu);
      orco_add = cu.texspace_location;
      orco_mul = cu.texspace_size;
      break;
    }
    case ID_MB: {
      MetaBall &mb = DRW_object_get_data_for_drawing<MetaBall>(*ref.object);
      orco_add = mb.texspace_location;
      orco_mul = mb.texspace_size;
      break;
    }
    default:
      orco_add = float3(0.0f);
      orco_mul = float3(1.0f);
      break;
  }
}

inline std::ostream &operator<<(std::ostream &stream, const ObjectInfos &infos)
{
  stream << "ObjectInfos(";
  if (infos.flag == eObjectInfoFlag::OBJECT_NO_INFO) {
    stream << "skipped)" << std::endl;
    return stream;
  }
  stream << "orco_add=" << infos.orco_add << ", ";
  stream << "orco_mul=" << infos.orco_mul << ", ";
  stream << "ob_color=" << infos.ob_color << ", ";
  stream << "index=" << infos.index << ", ";
  stream << "random=" << infos.random << ", ";
  stream << "flag=" << infos.flag << ")" << std::endl;
  return stream;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name ObjectBounds
 * \{ */

inline void ObjectBounds::sync()
{
#ifndef NDEBUG
  /* Initialize to NaN for easier debugging of uninitialized data usage. */
  bounding_corners[0] = float4(NAN_FLT);
  bounding_corners[1] = float4(NAN_FLT);
  bounding_corners[2] = float4(NAN_FLT);
  bounding_corners[3] = float4(NAN_FLT);
  bounding_sphere = float4(NAN_FLT);
#endif
  bounding_sphere.w = -1.0f; /* Disable test. */
}

inline void ObjectBounds::sync(const Object &ob, float inflate_bounds)
{
  const std::optional<blender::Bounds<float3>> bounds = BKE_object_boundbox_get(&ob);
  if (!bounds) {
#ifndef NDEBUG
    /* Initialize to NaN for easier debugging of uninitialized data usage. */
    bounding_corners[0] = float4(NAN_FLT);
    bounding_corners[1] = float4(NAN_FLT);
    bounding_corners[2] = float4(NAN_FLT);
    bounding_corners[3] = float4(NAN_FLT);
    bounding_sphere = float4(NAN_FLT);
#endif
    bounding_sphere.w = -1.0f; /* Disable test. */
    return;
  }
  const std::array<float3, 8> corners = blender::bounds::corners(*bounds);
  *reinterpret_cast<float3 *>(&bounding_corners[0]) = corners[0];
  *reinterpret_cast<float3 *>(&bounding_corners[1]) = corners[4];
  *reinterpret_cast<float3 *>(&bounding_corners[2]) = corners[3];
  *reinterpret_cast<float3 *>(&bounding_corners[3]) = corners[1];
  bounding_sphere.w = 0.0f; /* Enable test. */

  if (inflate_bounds != 0.0f) {
    BLI_assert(inflate_bounds >= 0.0f);
    float p = inflate_bounds;
    float n = -inflate_bounds;
    bounding_corners[0] += float4(n, n, n, 0.0f);
    bounding_corners[1] += float4(p, n, n, 0.0f);
    bounding_corners[2] += float4(n, p, n, 0.0f);
    bounding_corners[3] += float4(n, n, p, 0.0f);
  }
}

inline void ObjectBounds::sync(const float3 &center, const float3 &size)
{
  *reinterpret_cast<float3 *>(&bounding_corners[0]) = center - size;
  *reinterpret_cast<float3 *>(&bounding_corners[1]) = center + float3(+size.x, -size.y, -size.z);
  *reinterpret_cast<float3 *>(&bounding_corners[2]) = center + float3(-size.x, +size.y, -size.z);
  *reinterpret_cast<float3 *>(&bounding_corners[3]) = center + float3(-size.x, -size.y, +size.z);
  bounding_sphere.w = 0.0; /* Enable test. */
}

inline std::ostream &operator<<(std::ostream &stream, const ObjectBounds &bounds)
{
  stream << "ObjectBounds(";
  if (bounds.bounding_sphere.w == -1.0f) {
    stream << "skipped)" << std::endl;
    return stream;
  }
  stream << std::endl;
  stream << ".bounding_corners[0]"
         << *reinterpret_cast<const float3 *>(&bounds.bounding_corners[0]) << std::endl;
  stream << ".bounding_corners[1]"
         << *reinterpret_cast<const float3 *>(&bounds.bounding_corners[1]) << std::endl;
  stream << ".bounding_corners[2]"
         << *reinterpret_cast<const float3 *>(&bounds.bounding_corners[2]) << std::endl;
  stream << ".bounding_corners[3]"
         << *reinterpret_cast<const float3 *>(&bounds.bounding_corners[3]) << std::endl;
  stream << ".sphere=(pos=" << float3(bounds.bounding_sphere)
         << ", rad=" << bounds.bounding_sphere.w << std::endl;
  stream << ")" << std::endl;
  return stream;
}

/** \} */
