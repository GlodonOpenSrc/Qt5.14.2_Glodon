/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) 2009 Adam Barth. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/loader/http_refresh_scheduler.h"

#include <memory>
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/events/current_input_event.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/loader/frame_load_request.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

static ClientNavigationReason ToReason(
    Document::HttpRefreshType http_refresh_type) {
  switch (http_refresh_type) {
    case Document::HttpRefreshType::kHttpRefreshFromHeader:
      return ClientNavigationReason::kHttpHeaderRefresh;
    case Document::HttpRefreshType::kHttpRefreshFromMetaTag:
      return ClientNavigationReason::kMetaTagRefresh;
    default:
      break;
  }
  NOTREACHED();
  return ClientNavigationReason::kMetaTagRefresh;
}

HttpRefreshScheduler::HttpRefreshScheduler(Document* document)
    : document_(document) {}

bool HttpRefreshScheduler::IsScheduledWithin(double interval) const {
  return refresh_ && refresh_->delay <= interval;
}

void HttpRefreshScheduler::Schedule(
    double delay,
    const KURL& url,
    Document::HttpRefreshType http_refresh_type) {
  DCHECK(document_->GetFrame());
  if (!document_->GetFrame()->IsNavigationAllowed())
    return;
  if (delay < 0 || delay > INT_MAX / 1000)
    return;
  if (url.IsEmpty())
    return;
  if (refresh_ && refresh_->delay < delay)
    return;

  base::TimeTicks timestamp;
  if (const WebInputEvent* input_event = CurrentInputEvent::Get())
    timestamp = input_event->TimeStamp();

  Cancel();
  refresh_ = std::make_unique<ScheduledHttpRefresh>(
      delay, url, ToReason(http_refresh_type), timestamp);
  MaybeStartTimer();
}

void HttpRefreshScheduler::NavigateTask() {
  DCHECK(document_->GetFrame());
  std::unique_ptr<ScheduledHttpRefresh> refresh(refresh_.release());

  FrameLoadRequest request(document_, ResourceRequest(refresh->url));
  request.SetInputStartTime(refresh->input_timestamp);
  request.SetClientRedirectReason(refresh->reason);

  // We want a new back/forward list item if the refresh timeout is > 1 second.
  WebFrameLoadType load_type = WebFrameLoadType::kStandard;
  if (EqualIgnoringFragmentIdentifier(document_->Url(), refresh->url)) {
    request.GetResourceRequest().SetCacheMode(
        mojom::FetchCacheMode::kValidateCache);
    load_type = WebFrameLoadType::kReload;
  } else if (refresh->delay <= 1) {
    load_type = WebFrameLoadType::kReplaceCurrentItem;
  }

  document_->GetFrame()->Loader().StartNavigation(request, load_type);
  probe::FrameClearedScheduledNavigation(document_->GetFrame());
}

void HttpRefreshScheduler::MaybeStartTimer() {
  if (!refresh_)
    return;
  if (navigate_task_handle_.IsActive())
    return;
  if (!document_->LoadEventFinished())
    return;

  // wrapWeakPersistent(this) is safe because a posted task is canceled when the
  // task handle is destroyed on the dtor of this HttpRefreshScheduler.
  navigate_task_handle_ = PostDelayedCancellableTask(
      *document_->GetTaskRunner(TaskType::kInternalLoading), FROM_HERE,
      WTF::Bind(&HttpRefreshScheduler::NavigateTask, WrapWeakPersistent(this)),
      base::TimeDelta::FromSecondsD(refresh_->delay));

  probe::FrameScheduledNavigation(document_->GetFrame(), refresh_->url,
                                  refresh_->delay, refresh_->reason);
}

void HttpRefreshScheduler::Cancel() {
  if (navigate_task_handle_.IsActive()) {
    probe::FrameClearedScheduledNavigation(document_->GetFrame());
  }
  navigate_task_handle_.Cancel();
  refresh_.reset();
}

void HttpRefreshScheduler::Trace(blink::Visitor* visitor) {
  visitor->Trace(document_);
}

}  // namespace blink
