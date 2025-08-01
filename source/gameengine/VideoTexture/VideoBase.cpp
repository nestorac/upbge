/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2007 The Zdeno Ash Miklas. */

/** \file gameengine/VideoTexture/VideoBase.cpp
 *  \ingroup bgevideotex
 */

#if defined WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  include <windows.h>
#endif

#include "VideoBase.h"

#include <epoxy/gl.h>

#include "FilterSource.h"

// VideoBase implementation

// initialize image data
void VideoBase::init(short width, short height)
{
  // save original sizes
  m_orgSize[0] = width;
  m_orgSize[1] = height;
  // call base class initialization
  ImageBase::init(width, height);
}

// process video frame
void VideoBase::process(BYTE *sample)
{
  // if scale was changed
  if (m_scaleChange)
    // reset image
    init(m_orgSize[0], m_orgSize[1]);
  // if image is allocated and is able to store new image
  if (m_image != nullptr && !m_avail) {
    // filters used
    // convert video format to image
    switch (m_format) {
      case RGBA32: {
        FilterRGBA32 filtRGBA;
        // use filter object for format to convert image
        filterImage(filtRGBA, sample, m_orgSize);
        // finish
        break;
      }
      case RGB24: {
        FilterRGB24 filtRGB;
        // use filter object for format to convert image
        filterImage(filtRGB, sample, m_orgSize);
        // finish
        break;
      }
      case YV12: {
        // use filter object for format to convert image
        FilterYV12 filtYUV;
        filtYUV.setBuffs(sample, m_orgSize);
        filterImage(filtYUV, sample, m_orgSize);
        // finish
        break;
      }
      case None:
        break; /* assert? */
    }
  }
}

// python functions

// exceptions for video source initialization
ExceptionID SourceVideoEmpty, SourceVideoCreation;
ExpDesc SourceVideoEmptyDesc(SourceVideoEmpty, "Source Video is empty");
ExpDesc SourceVideoCreationDesc(SourceVideoCreation, "SourceVideo object was not created");

// open video source
void Video_open(VideoBase *self, char *file, short captureID)
{
  // if file is empty, throw exception
  if (file == nullptr)
    THRWEXCP(SourceVideoEmpty, S_OK);

  // open video file or capture device
  if (captureID >= 0)
    self->openCam(file, captureID);
  else
    self->openFile(file);
}

// play video
PyObject *Video_play(PyImage *self)
{
  if (getVideo(self)->play())
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

// pause video
PyObject *Video_pause(PyImage *self)
{
  if (getVideo(self)->pause())
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

PyObject *Video_stop(PyImage *self)
{
  if (getVideo(self)->stop())
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

// get status
PyObject *Video_getStatus(PyImage *self, void *closure)
{
  return Py_BuildValue("h", getVideo(self)->getStatus());
}

// refresh video
PyObject *Video_refresh(PyImage *self, PyObject *args)
{
  Py_buffer buffer;
  char *mode = nullptr;
  blender::gpu::TextureFormat format = blender::gpu::TextureFormat::UNORM_8_8_8_8;
  double ts = -1.0;

  memset(&buffer, 0, sizeof(buffer));
  if (PyArg_ParseTuple(args, "|s*sd:refresh", &buffer, &mode, &ts)) {
    if (buffer.buf) {
      // a target buffer is provided, verify its format
      if (buffer.readonly) {
        PyErr_SetString(PyExc_TypeError, "Buffers passed in argument must be writable");
      }
      else if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        PyErr_SetString(PyExc_TypeError,
                        "Buffers passed in argument must be contiguous in memory");
      }
      else if (((intptr_t)buffer.buf & 3) != 0) {
        PyErr_SetString(PyExc_TypeError,
                        "Buffers passed in argument must be aligned to 4 bytes boundary");
      }
      else {
        // ready to get the image into our buffer
        try {
          if (mode == nullptr || !strcmp(mode, "RGBA"))
            format = blender::gpu::TextureFormat::UNORM_8_8_8_8;
          else
            THRWEXCP(InvalidImageMode, S_OK);

          if (!self->m_image->loadImage((unsigned int *)buffer.buf, buffer.len, ts)) {
            PyErr_SetString(PyExc_TypeError,
                            "Could not load the buffer, perhaps size is not compatible");
          }
        }
        catch (Exception &exp) {
          exp.report();
        }
      }
      PyBuffer_Release(&buffer);
      if (PyErr_Occurred())
        return nullptr;
    }
  }
  else {
    return nullptr;
  }
  getVideo(self)->refresh();
  return Video_getStatus(self, nullptr);
}

// get range
PyObject *Video_getRange(PyImage *self, void *closure)
{
  return Py_BuildValue("[ff]", getVideo(self)->getRange()[0], getVideo(self)->getRange()[1]);
}

// set range
int Video_setRange(PyImage *self, PyObject *value, void *closure)
{
  // check validity of parameter
  if (value == nullptr || !PySequence_Check(value) || PySequence_Size(value) != 2 ||
      /* XXX - this is incorrect if the sequence is not a list/tuple! */
      !PyFloat_Check(PySequence_Fast_GET_ITEM(value, 0)) ||
      !PyFloat_Check(PySequence_Fast_GET_ITEM(value, 1))) {
    PyErr_SetString(PyExc_TypeError, "The value must be a sequence of 2 float");
    return -1;
  }
  // set range
  getVideo(self)->setRange(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(value, 0)),
                           PyFloat_AsDouble(PySequence_Fast_GET_ITEM(value, 1)));
  // success
  return 0;
}

// get repeat
PyObject *Video_getRepeat(PyImage *self, void *closure)
{
  return Py_BuildValue("h", getVideo(self)->getRepeat());
}

// set repeat
int Video_setRepeat(PyImage *self, PyObject *value, void *closure)
{
  // check validity of parameter
  if (value == nullptr || !PyLong_Check(value)) {
    PyErr_SetString(PyExc_TypeError, "The value must be an int");
    return -1;
  }
  // set repeat
  getVideo(self)->setRepeat(int(PyLong_AsLong(value)));
  // success
  return 0;
}

// get frame rate
PyObject *Video_getFrameRate(PyImage *self, void *closure)
{
  return Py_BuildValue("f", double(getVideo(self)->getFrameRate()));
}

// set frame rate
int Video_setFrameRate(PyImage *self, PyObject *value, void *closure)
{
  // check validity of parameter
  if (value == nullptr || !PyFloat_Check(value)) {
    PyErr_SetString(PyExc_TypeError, "The value must be a float");
    return -1;
  }
  // set repeat
  getVideo(self)->setFrameRate(float(PyFloat_AsDouble(value)));
  // success
  return 0;
}
