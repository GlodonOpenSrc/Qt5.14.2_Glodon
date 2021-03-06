// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_REF_COUNTED_VIDEO_SOURCE_PROVIDER_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_REF_COUNTED_VIDEO_SOURCE_PROVIDER_H_

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "services/video_capture/public/mojom/device_factory_provider.mojom.h"
#include "services/video_capture/public/mojom/video_source_provider.mojom.h"

namespace content {

// Enables ref-counted shared ownership of a
// video_capture::mojom::DeviceFactoryPtr.
// Since instances of this class do not guarantee that the connection stays open
// for its entire lifetime, clients must verify that the connection is bound
// before using it.
class CONTENT_EXPORT RefCountedVideoSourceProvider
    : public base::RefCounted<RefCountedVideoSourceProvider> {
 public:
  RefCountedVideoSourceProvider(
      video_capture::mojom::VideoSourceProviderPtr source_provider,
      video_capture::mojom::DeviceFactoryProviderPtr device_factory_provider,
      base::OnceClosure destruction_cb);

  base::WeakPtr<RefCountedVideoSourceProvider> GetWeakPtr();

  const video_capture::mojom::VideoSourceProviderPtr& source_provider() {
    return source_provider_;
  }

  void ShutdownServiceAsap();
  void SetRetryCount(int32_t count);
  void ReleaseProviderForTesting();

 private:
  friend class base::RefCounted<RefCountedVideoSourceProvider>;
  ~RefCountedVideoSourceProvider();

  video_capture::mojom::VideoSourceProviderPtr source_provider_;
  video_capture::mojom::DeviceFactoryProviderPtr device_factory_provider_;
  base::OnceClosure destruction_cb_;
  base::WeakPtrFactory<RefCountedVideoSourceProvider> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(RefCountedVideoSourceProvider);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_REF_COUNTED_VIDEO_SOURCE_PROVIDER_H_
