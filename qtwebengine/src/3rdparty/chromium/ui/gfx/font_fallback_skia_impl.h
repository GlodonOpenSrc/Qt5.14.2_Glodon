// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_FONT_FALLBACK_SKIA_IMPL_H_
#define UI_GFX_FONT_FALLBACK_SKIA_IMPL_H_

#include <string>

#include "base/strings/string_piece.h"
#include "ui/gfx/font.h"

namespace gfx {

std::string GetFallbackFontFamilyNameSkia(const Font& template_font,
                                          const std::string& locale,
                                          base::StringPiece16 text);
}

#endif  // UI_GFX_FONT_FALLBACK_SKIA_IMPL_H_