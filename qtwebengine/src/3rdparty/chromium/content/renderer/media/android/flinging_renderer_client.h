// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_ANDROID_FLINGING_RENDERER_CLIENT_H_
#define CONTENT_RENDERER_MEDIA_ANDROID_FLINGING_RENDERER_CLIENT_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "content/common/content_export.h"
#include "media/base/media_resource.h"
#include "media/base/renderer.h"
#include "media/base/renderer_client.h"
#include "media/base/video_renderer_sink.h"
#include "media/mojo/clients/mojo_renderer.h"
#include "media/mojo/clients/mojo_renderer_wrapper.h"
#include "media/mojo/interfaces/renderer_extensions.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

// FlingingRendererClient lives in Renderer process and mirrors a
// FlingingRenderer living in the Browser process.
class CONTENT_EXPORT FlingingRendererClient
    : public media::mojom::FlingingRendererClientExtension,
      public media::MojoRendererWrapper {
 public:
  using ClientExtentionRequest =
      media::mojom::FlingingRendererClientExtensionRequest;

  FlingingRendererClient(
      ClientExtentionRequest client_extension_request,
      scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
      std::unique_ptr<media::MojoRenderer> mojo_renderer,
      media::RemotePlayStateChangeCB remote_play_state_change_cb);

  ~FlingingRendererClient() override;

  // media::MojoRendererWrapper overrides.
  void Initialize(media::MediaResource* media_resource,
                  media::RendererClient* client,
                  const media::PipelineStatusCB& init_cb) override;

  // media::mojom::FlingingRendererClientExtension implementation
  void OnRemotePlayStateChange(media::MediaStatus::State state) override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;

  media::RendererClient* client_;

  media::RemotePlayStateChangeCB remote_play_state_change_cb_;

  // Used temporarily, to delay binding to |client_extension_binding_| until we
  // are on the right sequence, when Initialize() is called.
  ClientExtentionRequest delayed_bind_client_extension_request_;

  mojo::Binding<FlingingRendererClientExtension> client_extension_binding_;

  DISALLOW_COPY_AND_ASSIGN(FlingingRendererClient);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_ANDROID_FLINGING_RENDERER_CLIENT_H_
