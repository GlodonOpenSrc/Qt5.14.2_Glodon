// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_GPU_CLIENT_H_
#define CONTENT_PUBLIC_BROWSER_GPU_CLIENT_H_

#include <memory>

#include "components/viz/host/gpu_client.h"
#include "content/common/content_export.h"
#include "services/viz/public/interfaces/gpu.mojom.h"

namespace content {

CONTENT_EXPORT
std::unique_ptr<viz::GpuClient, base::OnTaskRunnerDeleter> CreateGpuClient(
    viz::mojom::GpuRequest request,
    viz::GpuClient::ConnectionErrorHandlerClosure connection_error_handler,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner);

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_GPU_CLIENT_H_
