/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup pythonintern
 */

#include <Python.h>

#include "../generic/python_compat.hh" /* IWYU pragma: keep. */

#include "BLI_utildefines.h"

#include "bpy_app_build_options.hh"

static PyTypeObject BlenderAppBuildOptionsType;

static PyStructSequence_Field app_builtopts_info_fields[] = {
    /* names mostly follow CMake options, lowercase, after `WITH_` */
    {"bullet", nullptr},
    {"codec_avi", nullptr},
    {"codec_ffmpeg", nullptr},
    {"codec_sndfile", nullptr},
    {"compositor_cpu", nullptr},
    {"cycles", nullptr},
    {"cycles_osl", nullptr},
    {"freestyle", nullptr},
    {"gameengine", nullptr},
    {"image_cineon", nullptr},
    {"image_dds", nullptr},
    {"image_hdr", nullptr},
    {"image_openexr", nullptr},
    {"image_openjpeg", nullptr},
    {"image_tiff", nullptr},
    {"image_webp", nullptr},
    {"input_ndof", nullptr},
    {"audaspace", nullptr},
    {"international", nullptr},
    {"openal", nullptr},
    {"opensubdiv", nullptr},
    {"sdl", nullptr},
    {"coreaudio", nullptr},
    {"jack", nullptr},
    {"pulseaudio", nullptr},
    {"wasapi", nullptr},
    {"libmv", nullptr},
    {"mod_oceansim", nullptr},
    {"mod_remesh", nullptr},
    {"player", nullptr},
    {"io_wavefront_obj", nullptr},
    {"io_ply", nullptr},
    {"io_stl", nullptr},
    {"io_fbx", nullptr},
    {"io_gpencil", nullptr},
    {"opencolorio", nullptr},
    {"openmp", nullptr},
    {"openvdb", nullptr},
    {"alembic", nullptr},
    {"usd", nullptr},
    {"fluid", nullptr},
    {"xr_openxr", nullptr},
    {"potrace", nullptr},
    {"pugixml", nullptr},
    {"haru", nullptr},
    {"experimental_features", nullptr},
    /* Sentinel (this line prevents `clang-format` wrapping into columns). */
    {nullptr},
};

static PyStructSequence_Desc app_builtopts_info_desc = {
    /*name*/ "bpy.app.build_options",
    /*doc*/ "This module contains information about options blender is built with",
    /*fields*/ app_builtopts_info_fields,
    /*n_in_sequence*/ ARRAY_SIZE(app_builtopts_info_fields) - 1,
};

static PyObject *make_builtopts_info()
{
  PyObject *builtopts_info;
  int pos = 0;

  builtopts_info = PyStructSequence_New(&BlenderAppBuildOptionsType);
  if (builtopts_info == nullptr) {
    return nullptr;
  }

#define SetObjIncref(item) \
  PyStructSequence_SET_ITEM(builtopts_info, pos++, (Py_IncRef(item), item))

#ifdef WITH_BULLET
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

  /* AVI */
  SetObjIncref(Py_False);

#ifdef WITH_FFMPEG
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_SNDFILE
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

  /* Compositor. */
  SetObjIncref(Py_True);

#ifdef WITH_CYCLES
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_CYCLES_OSL
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_FREESTYLE
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_GAMEENGINE
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_IMAGE_CINEON
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

  /* DDS */
  SetObjIncref(Py_True);

  /* HDR */
  SetObjIncref(Py_True);

#ifdef WITH_IMAGE_OPENEXR
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_IMAGE_OPENJPEG
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

  /* TIFF */
  SetObjIncref(Py_True);

#ifdef WITH_IMAGE_WEBP
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_INPUT_NDOF
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_AUDASPACE
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_INTERNATIONAL
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_OPENAL
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_OPENSUBDIV
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_SDL
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_COREAUDIO
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_JACK
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_PULSEAUDIO
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_WASAPI
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_LIBMV
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_OCEANSIM
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_MOD_REMESH
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_IO_WAVEFRONT_OBJ
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_IO_PLY
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_IO_STL
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_IO_FBX
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_IO_GREASE_PENCIL
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_OPENCOLORIO
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_PLAYER
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef _OPENMP
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_OPENVDB
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_ALEMBIC
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_USD
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_FLUID
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_XR_OPENXR
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_POTRACE
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_PUGIXML
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_HARU
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#ifdef WITH_EXPERIMENTAL_FEATURES
  SetObjIncref(Py_True);
#else
  SetObjIncref(Py_False);
#endif

#undef SetObjIncref

  return builtopts_info;
}

PyObject *BPY_app_build_options_struct()
{
  PyObject *ret;

  PyStructSequence_InitType(&BlenderAppBuildOptionsType, &app_builtopts_info_desc);

  ret = make_builtopts_info();

  /* prevent user from creating new instances */
  BlenderAppBuildOptionsType.tp_init = nullptr;
  BlenderAppBuildOptionsType.tp_new = nullptr;
  /* Without this we can't do `set(sys.modules)` #29635. */
  BlenderAppBuildOptionsType.tp_hash = (hashfunc)Py_HashPointer;

  return ret;
}
