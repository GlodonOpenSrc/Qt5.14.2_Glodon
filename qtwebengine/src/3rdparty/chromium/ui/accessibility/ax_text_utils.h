// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_AX_TEXT_UTILS_H_
#define UI_ACCESSIBILITY_AX_TEXT_UTILS_H_

#include <stddef.h>

#include <vector>

#include "base/strings/string16.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_export.h"
#include "ui/accessibility/ax_text_boundary.h"

namespace ui {

// A direction when searching for the next boundary.
enum TextBoundaryDirection {
  // Search forwards for the next boundary past the starting position.
  FORWARDS_DIRECTION,
  // Search backwards for the previous boundary before the starting position.
  BACKWARDS_DIRECTION
};

// Convenience method needed to implement platform-specific text
// accessibility APIs like IAccessible2. Search forwards or backwards
// (depending on |direction|) from the given |start_offset| until the
// given boundary is found, and return the offset of that boundary,
// using the vector of line break character offsets in |line_breaks|.
AX_EXPORT size_t FindAccessibleTextBoundary(const base::string16& text,
                                            const std::vector<int>& line_breaks,
                                            AXTextBoundary boundary,
                                            size_t start_offset,
                                            TextBoundaryDirection direction,
                                            ax::mojom::TextAffinity affinity);

// Returns a string ID that corresponds to the name of the given action.
AX_EXPORT base::string16 ActionVerbToLocalizedString(
    const ax::mojom::DefaultActionVerb action_verb);

// Returns the non-localized string representation of a supported action.
// Some APIs on Linux and Windows need to return non-localized action names.
AX_EXPORT base::string16 ActionVerbToUnlocalizedString(
    const ax::mojom::DefaultActionVerb action_verb);

// Returns indices of all word starts in |text|.
AX_EXPORT std::vector<int> GetWordStartOffsets(const base::string16& text);
// Returns indices of all word ends in |text|.
AX_EXPORT std::vector<int> GetWordEndOffsets(const base::string16& text);

}  // namespace ui

#endif  // UI_ACCESSIBILITY_AX_TEXT_UTILS_H_
