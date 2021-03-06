// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/media_session/public/cpp/util.h"

#include "services/media_session/public/mojom/constants.mojom.h"

namespace {

constexpr base::TimeDelta kDefaultSeekTime =
    base::TimeDelta::FromSeconds(media_session::mojom::kDefaultSeekTimeSeconds);

}  // namespace

namespace media_session {

void PerformMediaSessionAction(
    mojom::MediaSessionAction action,
    const mojom::MediaControllerPtr& media_controller_ptr) {
  switch (action) {
    case mojom::MediaSessionAction::kPreviousTrack:
      media_controller_ptr->PreviousTrack();
      break;
    case mojom::MediaSessionAction::kSeekBackward:
      media_controller_ptr->Seek(kDefaultSeekTime * -1);
      break;
    case mojom::MediaSessionAction::kPlay:
      media_controller_ptr->Resume();
      break;
    case mojom::MediaSessionAction::kPause:
      media_controller_ptr->Suspend();
      break;
    case mojom::MediaSessionAction::kSeekForward:
      media_controller_ptr->Seek(kDefaultSeekTime);
      break;
    case mojom::MediaSessionAction::kNextTrack:
      media_controller_ptr->NextTrack();
      break;
    case mojom::MediaSessionAction::kStop:
      media_controller_ptr->Stop();
      break;
    case mojom::MediaSessionAction::kSkipAd:
    case mojom::MediaSessionAction::kSeekTo:
    case mojom::MediaSessionAction::kScrubTo:
      break;
  }
}

}  // namespace media_session
