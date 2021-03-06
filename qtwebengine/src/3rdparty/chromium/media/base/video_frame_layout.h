// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_VIDEO_FRAME_LAYOUT_H_
#define MEDIA_BASE_VIDEO_FRAME_LAYOUT_H_

#include <stddef.h>
#include <stdint.h>

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "base/optional.h"
#include "media/base/media_export.h"
#include "media/base/video_types.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_pixmap_handle.h"

namespace media {

// A class to describes how physical buffer is allocated for video frame.
// In stores format, coded size of the frame and size of physical buffers
// which can be used to allocate buffer(s) hardware expected.
// It also stores stride (bytes per line) and offset per color plane as Plane.
// stride is to calculate each color plane's size (note that a buffer may
// contains multiple color planes.)
// offset is to describe a start point of each plane from buffer's dmabuf fd.
// Note that it is copyable.
class MEDIA_EXPORT VideoFrameLayout {
 public:
  // Default alignment for buffers.
  // Note: This value is dependent on what's used by ffmpeg, do not change
  // without inspecting av_frame_get_buffer() first.
  static constexpr size_t kBufferAddressAlignment = 32;

  struct Plane {
    Plane() = default;
    Plane(int32_t stride, size_t offset, size_t size)
        : stride(stride), offset(offset), size(size) {}

    bool operator==(const Plane& rhs) const;
    bool operator!=(const Plane& rhs) const;

    // Strides in bytes of a plane. Note that stride can be negative if the
    // image layout is bottom-up.
    int32_t stride = 0;

    // Offset in bytes of a plane, which stands for the offset of a start point
    // of a color plane from a buffer fd.
    size_t offset = 0;

    // Size in bytes of a plane. This |size| bytes data must contain all the
    // data a decoder will access (e.g. visible area and padding).
    size_t size = 0;
  };

  // Factory functions.
  // |format| and |coded_size| must be specified.
  // |is_single_planar| is optional. It describes planes can be stored (although
  // not always) in multiple buffers. It is specified only in HW decoder code.
  // |planes| info is also optional but useful to represent the layout of a
  // video frame buffer correctly.
  // |buffer_addr_align| can be specified to request a specific buffer memory
  // alignment.
  // |modifier| is the additional information of |format|. It will become some
  // value else than gfx::NativePixmapHandle::kNoModifier when the underlying
  // buffer format is different from a standard |format| due to tiling.
  // The returned base::Optional will be base::nullopt if the configured values
  // are invalid.
  static base::Optional<VideoFrameLayout> Create(VideoPixelFormat format,
                                                 const gfx::Size& coded_size);

  // The size of |strides| must be NumPlanes(|format|). Planes' offset will be
  // 0.
  static base::Optional<VideoFrameLayout> CreateWithStrides(
      VideoPixelFormat format,
      const gfx::Size& coded_size,
      std::vector<int32_t> strides);

  // The size of |planes| must be NumPlanes(|format|).
  static base::Optional<VideoFrameLayout> CreateWithPlanes(
      VideoPixelFormat format,
      const gfx::Size& coded_size,
      std::vector<Plane> planes,
      size_t buffer_addr_align = kBufferAddressAlignment,
      uint64_t modifier = gfx::NativePixmapHandle::kNoModifier);

  // This constructor should be called for situations where the frames using
  // this format are backed by multiple physical buffers, instead of having each
  // plane at different offsets of the same buffer. Currently only used by V4L2.
  static base::Optional<VideoFrameLayout> CreateMultiPlanar(
      VideoPixelFormat format,
      const gfx::Size& coded_size,
      std::vector<Plane> planes,
      size_t buffer_addr_align = kBufferAddressAlignment,
      uint64_t modifier = gfx::NativePixmapHandle::kNoModifier);

  VideoFrameLayout() = delete;
  VideoFrameLayout(const VideoFrameLayout&);
  VideoFrameLayout(VideoFrameLayout&&);
  VideoFrameLayout& operator=(const VideoFrameLayout&);
  ~VideoFrameLayout();

  static size_t NumPlanes(VideoPixelFormat format);

  VideoPixelFormat format() const { return format_; }
  const gfx::Size& coded_size() const { return coded_size_; }

  // Returns number of planes. Note that num_planes >= num_buffers.
  size_t num_planes() const { return planes_.size(); }

  const std::vector<Plane>& planes() const { return planes_; }

  bool operator==(const VideoFrameLayout& rhs) const;
  bool operator!=(const VideoFrameLayout& rhs) const;

  // Return true when a format uses multiple backing buffers to store its
  // planes.
  bool is_multi_planar() const { return is_multi_planar_; }
  // Returns the required memory alignment for buffers.
  size_t buffer_addr_align() const { return buffer_addr_align_; }
  // Return the modifier of buffers.
  uint64_t modifier() const { return modifier_; }

 private:
  VideoFrameLayout(VideoPixelFormat format,
                   const gfx::Size& coded_size,
                   std::vector<Plane> planes,
                   bool is_multi_planar,
                   size_t buffer_addr_align,
                   uint64_t modifier);

  VideoPixelFormat format_;

  // Width and height of the video frame in pixels. This must include pixel
  // data for the whole image; i.e. for YUV formats with subsampled chroma
  // planes, in the case that the visible portion of the image does not line up
  // on a sample boundary, |coded_size_| must be rounded up appropriately and
  // the pixel data provided for the odd pixels.
  gfx::Size coded_size_;

  // Layout property for each color planes, e.g. stride and buffer offset.
  std::vector<Plane> planes_;

  // Set to true when a format uses multiple backing buffers to store its
  // planes. Used by code for V4L2 API at the moment.
  bool is_multi_planar_;

  // Memory address alignment of the buffers. This is only relevant when
  // allocating physical memory for the buffer, so it doesn't need to be
  // serialized when frames are passed through Mojo.
  size_t buffer_addr_align_;

  // Modifier of buffers. The modifier is retrieved from GBM library. This
  // can be a different value from kNoModifier only if the VideoFrame is created
  // by using NativePixmap.
  uint64_t modifier_;
};

// Outputs VideoFrameLayout::Plane to stream.
MEDIA_EXPORT std::ostream& operator<<(std::ostream& ostream,
                                      const VideoFrameLayout::Plane& plane);

// Outputs VideoFrameLayout to stream.
MEDIA_EXPORT std::ostream& operator<<(std::ostream& ostream,
                                      const VideoFrameLayout& layout);

}  // namespace media

#endif  // MEDIA_BASE_VIDEO_FRAME_LAYOUT_H_
