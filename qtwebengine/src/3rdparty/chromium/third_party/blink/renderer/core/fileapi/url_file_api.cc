// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fileapi/url_file_api.h"

#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fileapi/blob.h"
#include "third_party/blink/renderer/core/fileapi/public_url_manager.h"
#include "third_party/blink/renderer/core/url/dom_url.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"

namespace blink {

// static
String URLFileAPI::createObjectURL(ScriptState* script_state,
                                   Blob* blob,
                                   ExceptionState& exception_state) {
  DCHECK(blob);
  ExecutionContext* execution_context = ExecutionContext::From(script_state);
  DCHECK(execution_context);

  UseCounter::Count(execution_context, WebFeature::kCreateObjectURLBlob);
  return DOMURL::CreatePublicURL(execution_context, blob);
}

// static
void URLFileAPI::revokeObjectURL(ScriptState* script_state,
                                 const String& url_string) {
  ExecutionContext* execution_context = ExecutionContext::From(script_state);
  DCHECK(execution_context);

  KURL url(NullURL(), url_string);
  execution_context->RemoveURLFromMemoryCache(url);
  execution_context->GetPublicURLManager().Revoke(url);
}

}  // namespace blink
