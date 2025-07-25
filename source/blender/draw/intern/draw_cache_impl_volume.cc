/* SPDX-FileCopyrightText: 2017 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup draw
 *
 * \brief Volume API for render engines
 */

#include <cstring>

#include "MEM_guardedalloc.h"

#include "BLI_listbase.h"
#include "BLI_math_matrix.hh"
#include "BLI_string.h"
#include "BLI_utildefines.h"

#include "DNA_scene_types.h"
#include "DNA_volume_types.h"

#include "BKE_global.hh"
#include "BKE_volume.hh"
#include "BKE_volume_grid_fwd.hh"
#include "BKE_volume_render.hh"

#include "GPU_attribute_convert.hh"
#include "GPU_batch.hh"
#include "GPU_capabilities.hh"
#include "GPU_texture.hh"

#include "DRW_render.hh"

#include "draw_cache.hh"      /* own include */
#include "draw_cache_impl.hh" /* own include */

namespace blender::draw {

static void volume_batch_cache_clear(Volume *volume);

/* ---------------------------------------------------------------------- */
/* Volume gpu::Batch Cache */

struct VolumeBatchCache {
  /* 3D textures */
  ListBase grids;

  /* Wireframe */
  struct {
    gpu::VertBuf *pos_nor_in_order;
    gpu::Batch *batch;
  } face_wire;

  /* Surface for selection */
  gpu::Batch *selection_surface;

  /* settings to determine if cache is invalid */
  bool is_dirty;
};

/* gpu::Batch cache management. */

static bool volume_batch_cache_valid(Volume *volume)
{
  VolumeBatchCache *cache = static_cast<VolumeBatchCache *>(volume->batch_cache);
  return (cache && cache->is_dirty == false);
}

static void volume_batch_cache_init(Volume *volume)
{
  VolumeBatchCache *cache = static_cast<VolumeBatchCache *>(volume->batch_cache);

  if (!cache) {
    volume->batch_cache = cache = MEM_callocN<VolumeBatchCache>(__func__);
  }
  else {
    memset(cache, 0, sizeof(*cache));
  }

  cache->is_dirty = false;
}

void DRW_volume_batch_cache_validate(Volume *volume)
{
  if (!volume_batch_cache_valid(volume)) {
    volume_batch_cache_clear(volume);
    volume_batch_cache_init(volume);
  }
}

static VolumeBatchCache *volume_batch_cache_get(Volume *volume)
{
  DRW_volume_batch_cache_validate(volume);
  return static_cast<VolumeBatchCache *>(volume->batch_cache);
}

void DRW_volume_batch_cache_dirty_tag(Volume *volume, int mode)
{
  VolumeBatchCache *cache = static_cast<VolumeBatchCache *>(volume->batch_cache);
  if (cache == nullptr) {
    return;
  }
  switch (mode) {
    case BKE_VOLUME_BATCH_DIRTY_ALL:
      cache->is_dirty = true;
      break;
    default:
      BLI_assert(0);
  }
}

static void volume_batch_cache_clear(Volume *volume)
{
  VolumeBatchCache *cache = static_cast<VolumeBatchCache *>(volume->batch_cache);
  if (!cache) {
    return;
  }

  LISTBASE_FOREACH (DRWVolumeGrid *, grid, &cache->grids) {
    MEM_SAFE_FREE(grid->name);
    GPU_TEXTURE_FREE_SAFE(grid->texture);
  }
  BLI_freelistN(&cache->grids);

  GPU_VERTBUF_DISCARD_SAFE(cache->face_wire.pos_nor_in_order);
  GPU_BATCH_DISCARD_SAFE(cache->face_wire.batch);
  GPU_BATCH_DISCARD_SAFE(cache->selection_surface);
}

void DRW_volume_batch_cache_free(Volume *volume)
{
  volume_batch_cache_clear(volume);
  MEM_SAFE_FREE(volume->batch_cache);
}
struct VolumeWireframeUserData {
  Volume *volume;
  Scene *scene;
};

static void drw_volume_wireframe_cb(
    void *userdata, const float (*verts)[3], const int (*edges)[2], int totvert, int totedge)
{
  VolumeWireframeUserData *data = static_cast<VolumeWireframeUserData *>(userdata);
  Scene *scene = data->scene;
  Volume *volume = data->volume;
  VolumeBatchCache *cache = static_cast<VolumeBatchCache *>(volume->batch_cache);
  const bool do_hq_normals = (scene->r.perf_flag & SCE_PERF_HQ_NORMALS) != 0 ||
                             GPU_use_hq_normals_workaround();

  /* Create vertex buffer. */
  static struct {
    uint pos_id, nor_id;
    uint pos_hq_id, nor_hq_id;
  } attr_id;

  static const GPUVertFormat format = [&]() {
    GPUVertFormat format{};
    attr_id.pos_id = GPU_vertformat_attr_add(&format, "pos", gpu::VertAttrType::SFLOAT_32_32_32);
    attr_id.nor_id = GPU_vertformat_attr_add(
        &format, "nor", blender::gpu::VertAttrType::SNORM_10_10_10_2);
    return format;
  }();

  static const GPUVertFormat format_hq = [&]() {
    GPUVertFormat format{};
    attr_id.pos_hq_id = GPU_vertformat_attr_add(
        &format, "pos", gpu::VertAttrType::SFLOAT_32_32_32);
    attr_id.nor_hq_id = GPU_vertformat_attr_add(
        &format, "nor", blender::gpu::VertAttrType::SNORM_16_16_16_16);
    return format;
  }();

  uint pos_id = do_hq_normals ? attr_id.pos_hq_id : attr_id.pos_id;
  uint nor_id = do_hq_normals ? attr_id.nor_hq_id : attr_id.nor_id;

  cache->face_wire.pos_nor_in_order = GPU_vertbuf_create_with_format(do_hq_normals ? format_hq :
                                                                                     format);
  GPU_vertbuf_data_alloc(*cache->face_wire.pos_nor_in_order, totvert);
  GPU_vertbuf_attr_fill(cache->face_wire.pos_nor_in_order, pos_id, verts);
  const float3 normal(1.0f, 0.0f, 0.0f);
  if (do_hq_normals) {
    const gpu::PackedNormal packed_normal = gpu::convert_normal<gpu::PackedNormal>(normal);
    GPU_vertbuf_attr_fill_stride(cache->face_wire.pos_nor_in_order, nor_id, 0, &packed_normal);
  }
  else {
    const short4 packed_normal = gpu::convert_normal<short4>(normal);
    GPU_vertbuf_attr_fill_stride(cache->face_wire.pos_nor_in_order, nor_id, 0, &packed_normal);
  }

  /* Create wiredata. */
  gpu::VertBuf *vbo_wiredata = GPU_vertbuf_calloc();
  DRW_vertbuf_create_wiredata(vbo_wiredata, totvert);

  if (volume->display.wireframe_type == VOLUME_WIREFRAME_POINTS) {
    /* Create batch. */
    cache->face_wire.batch = GPU_batch_create(
        GPU_PRIM_POINTS, cache->face_wire.pos_nor_in_order, nullptr);
  }
  else {
    /* Create edge index buffer. */
    GPUIndexBufBuilder elb;
    GPU_indexbuf_init(&elb, GPU_PRIM_LINES, totedge, totvert);
    for (int i = 0; i < totedge; i++) {
      GPU_indexbuf_add_line_verts(&elb, edges[i][0], edges[i][1]);
    }
    gpu::IndexBuf *ibo = GPU_indexbuf_build(&elb);

    /* Create batch. */
    cache->face_wire.batch = GPU_batch_create_ex(
        GPU_PRIM_LINES, cache->face_wire.pos_nor_in_order, ibo, GPU_BATCH_OWNS_INDEX);
  }

  GPU_batch_vertbuf_add(cache->face_wire.batch, vbo_wiredata, true);
}

gpu::Batch *DRW_volume_batch_cache_get_wireframes_face(Volume *volume)
{
  if (volume->display.wireframe_type == VOLUME_WIREFRAME_NONE) {
    return nullptr;
  }

  VolumeBatchCache *cache = volume_batch_cache_get(volume);

  if (cache->face_wire.batch == nullptr) {
    const bke::VolumeGridData *volume_grid = BKE_volume_grid_active_get_for_read(volume);
    if (volume_grid == nullptr) {
      return nullptr;
    }

    /* Create wireframe from OpenVDB tree. */
    const DRWContext *draw_ctx = DRW_context_get();
    VolumeWireframeUserData userdata;
    userdata.volume = volume;
    userdata.scene = draw_ctx->scene;
    BKE_volume_grid_wireframe(volume, volume_grid, drw_volume_wireframe_cb, &userdata);
  }

  return cache->face_wire.batch;
}

static void drw_volume_selection_surface_cb(
    void *userdata, float (*verts)[3], int (*tris)[3], int totvert, int tottris)
{
  Volume *volume = static_cast<Volume *>(userdata);
  VolumeBatchCache *cache = static_cast<VolumeBatchCache *>(volume->batch_cache);

  static uint pos_id;
  static const GPUVertFormat format = [&]() {
    GPUVertFormat format{};
    pos_id = GPU_vertformat_attr_add(&format, "pos", gpu::VertAttrType::SFLOAT_32_32_32);
    return format;
  }();

  /* Create vertex buffer. */
  gpu::VertBuf *vbo_surface = GPU_vertbuf_create_with_format(format);
  GPU_vertbuf_data_alloc(*vbo_surface, totvert);
  GPU_vertbuf_attr_fill(vbo_surface, pos_id, verts);

  /* Create index buffer. */
  GPUIndexBufBuilder elb;
  GPU_indexbuf_init(&elb, GPU_PRIM_TRIS, tottris, totvert);
  for (int i = 0; i < tottris; i++) {
    GPU_indexbuf_add_tri_verts(&elb, UNPACK3(tris[i]));
  }
  gpu::IndexBuf *ibo_surface = GPU_indexbuf_build(&elb);

  cache->selection_surface = GPU_batch_create_ex(
      GPU_PRIM_TRIS, vbo_surface, ibo_surface, GPU_BATCH_OWNS_VBO | GPU_BATCH_OWNS_INDEX);
}

gpu::Batch *DRW_volume_batch_cache_get_selection_surface(Volume *volume)
{
  VolumeBatchCache *cache = volume_batch_cache_get(volume);
  if (cache->selection_surface == nullptr) {
    const bke::VolumeGridData *volume_grid = BKE_volume_grid_active_get_for_read(volume);
    if (volume_grid == nullptr) {
      return nullptr;
    }
    BKE_volume_grid_selection_surface(
        volume, volume_grid, drw_volume_selection_surface_cb, volume);
  }
  return cache->selection_surface;
}

static DRWVolumeGrid *volume_grid_cache_get(const Volume *volume,
                                            const bke::VolumeGridData *grid,
                                            VolumeBatchCache *cache)
{
  const std::string name = bke::volume_grid::get_name(*grid);

  /* Return cached grid. */
  LISTBASE_FOREACH (DRWVolumeGrid *, cache_grid, &cache->grids) {
    if (cache_grid->name == name) {
      return cache_grid;
    }
  }

  /* Allocate new grid. */
  DRWVolumeGrid *cache_grid = MEM_callocN<DRWVolumeGrid>(__func__);
  cache_grid->name = BLI_strdup(name.c_str());
  BLI_addtail(&cache->grids, cache_grid);

  /* TODO: can we load this earlier, avoid accessing the global and take
   * advantage of dependency graph multi-threading? */
  BKE_volume_load(volume, G.main);

  /* Test if we support textures with the number of channels. */
  size_t channels = bke::volume_grid::get_channels_num(bke::volume_grid::get_type(*grid));
  if (!ELEM(channels, 1, 3)) {
    return cache_grid;
  }

  DenseFloatVolumeGrid dense_grid;
  if (BKE_volume_grid_dense_floats(volume, grid, &dense_grid)) {
    cache_grid->texture_to_object = float4x4(dense_grid.texture_to_object);
    cache_grid->object_to_texture = math::invert(cache_grid->texture_to_object);

    /* Create GPU texture. */
    blender::gpu::TextureFormat format = (channels == 3) ?
                                             blender::gpu::TextureFormat::SFLOAT_16_16_16 :
                                             blender::gpu::TextureFormat::SFLOAT_16;
    cache_grid->texture = GPU_texture_create_3d("volume_grid",
                                                UNPACK3(dense_grid.resolution),
                                                1,
                                                format,
                                                GPU_TEXTURE_USAGE_SHADER_READ,
                                                dense_grid.voxels);
    /* The texture can be null if the resolution along one axis is larger than
     * GL_MAX_3D_TEXTURE_SIZE. */
    if (cache_grid->texture != nullptr) {
      GPU_texture_swizzle_set(cache_grid->texture, (channels == 3) ? "rgb1" : "rrr1");
      GPU_texture_extend_mode(cache_grid->texture, GPU_SAMPLER_EXTEND_MODE_CLAMP_TO_BORDER);
      BKE_volume_dense_float_grid_clear(&dense_grid);
    }
    else {
      MEM_freeN(dense_grid.voxels);
      printf("Error: Could not allocate 3D texture for volume.\n");
    }
  }

  return cache_grid;
}

DRWVolumeGrid *DRW_volume_batch_cache_get_grid(Volume *volume,
                                               const bke::VolumeGridData *volume_grid)
{
  VolumeBatchCache *cache = volume_batch_cache_get(volume);
  DRWVolumeGrid *grid = volume_grid_cache_get(volume, volume_grid, cache);
  return (grid->texture != nullptr) ? grid : nullptr;
}

}  // namespace blender::draw
