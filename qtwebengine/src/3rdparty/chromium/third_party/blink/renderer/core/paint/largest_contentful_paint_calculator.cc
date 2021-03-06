// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/largest_contentful_paint_calculator.h"

#include "third_party/blink/renderer/core/paint/image_element_timing.h"

namespace blink {

namespace {
bool HasLargestTextChanged(const std::unique_ptr<TextRecord>& a,
                           const base::WeakPtr<TextRecord> b) {
  if (!a && !b)
    return false;
  if (!a && b)
    return true;
  if (a && !b)
    return true;
  return a->node_id != b->node_id || a->first_size != b->first_size ||
         a->paint_time != b->paint_time;
}

bool HasLargestImageChanged(const std::unique_ptr<ImageRecord>& a,
                            const ImageRecord* b) {
  if (!a && !b)
    return false;
  if (!a && b)
    return true;
  if (a && !b)
    return true;
  return a->node_id != b->node_id || a->first_size != b->first_size ||
         a->paint_time != b->paint_time || a->load_time != b->load_time;
}
}  // namespace

LargestContentfulPaintCalculator::LargestContentfulPaintCalculator(
    WindowPerformance* window_performance)
    : window_performance_(window_performance) {}

void LargestContentfulPaintCalculator::OnLargestImageUpdated(
    const ImageRecord* largest_image) {
  largest_image_.reset();
  if (largest_image) {
    largest_image_ = std::make_unique<ImageRecord>();
    largest_image_->node_id = largest_image->node_id;
    largest_image_->first_size = largest_image->first_size;
    largest_image_->paint_time = largest_image->paint_time;
    largest_image_->cached_image = largest_image->cached_image;
    largest_image_->load_time = largest_image->load_time;
  }
}

void LargestContentfulPaintCalculator::OnLargestTextUpdated(
    base::WeakPtr<TextRecord> largest_text) {
  largest_text_.reset();
  if (largest_text) {
    largest_text_ = std::make_unique<TextRecord>(
        largest_text->node_id, largest_text->first_size, FloatRect());
    largest_text_->paint_time = largest_text->paint_time;
  }
}

void LargestContentfulPaintCalculator::UpdateLargestContentPaintIfNeeded(
    base::Optional<base::WeakPtr<TextRecord>> largest_text,
    base::Optional<const ImageRecord*> largest_image) {
  bool image_has_changed = false;
  bool text_has_changed = false;
  if (largest_image.has_value()) {
    image_has_changed = HasLargestImageChanged(largest_image_, *largest_image);
    OnLargestImageUpdated(*largest_image);
  }
  if (largest_text.has_value()) {
    text_has_changed = HasLargestTextChanged(largest_text_, *largest_text);
    OnLargestTextUpdated(*largest_text);
  }
  // If |largest_image| does not have value, the detector may have been
  // destroyed. In this case, keep using its last candidate for comparison with
  // the text candidate. The same for |largest_text|.
  if ((!largest_image.has_value() || !image_has_changed) &&
      (!largest_text.has_value() || !text_has_changed))
    return;

  if (!largest_text_ && !largest_image_)
    return;
  if (LargestTextSize() > LargestImageSize()) {
    if (largest_text_->paint_time > base::TimeTicks())
      UpdateLargestContentfulPaint(LargestContentType::kText);
  } else {
    if (largest_image_->paint_time > base::TimeTicks())
      UpdateLargestContentfulPaint(LargestContentType::kImage);
  }
}

void LargestContentfulPaintCalculator::UpdateLargestContentfulPaint(
    LargestContentType type) {
  DCHECK(window_performance_);
  DCHECK(type != LargestContentType::kUnknown);
  last_type_ = type;
  if (type == LargestContentType::kImage) {
    DCHECK(largest_image_);
    const ImageResourceContent* cached_image = largest_image_->cached_image;
    Node* image_node = DOMNodeIds::NodeForId(largest_image_->node_id);

    // |cached_image| is a weak pointer, so it may be null. This can only happen
    // if the image has been removed, which means that the largest image is not
    // up-to-date. This can happen when this method call came from
    // OnLargestTextUpdated(). It is safe to ignore the image in this case: the
    // correct largest content should be identified on the next call to
    // OnLargestImageUpdated().
    // For similar reasons, |image_node| may be null and it is safe to ignore
    // the |largest_image_| content in this case as well.
    if (!cached_image || !image_node)
      return;

    const KURL& url = cached_image->Url();
    auto* document = window_performance_->GetExecutionContext();
    bool expose_paint_time_to_api = true;
    if (!url.ProtocolIsData() &&
        (!document || !Performance::PassesTimingAllowCheck(
                          cached_image->GetResponse(),
                          *document->GetSecurityOrigin(), document))) {
      expose_paint_time_to_api = false;
    }
    const String& image_url =
        url.ProtocolIsData()
            ? url.GetString().Left(ImageElementTiming::kInlineImageMaxChars)
            : url.GetString();
    // Do not expose element attribution from shadow trees.
    Element* image_element =
        image_node->IsInShadowTree() ? nullptr : To<Element>(image_node);
    const AtomicString& image_id =
        image_element ? image_element->GetIdAttribute() : AtomicString();
    window_performance_->OnLargestContentfulPaintUpdated(
        expose_paint_time_to_api ? largest_image_->paint_time
                                 : base::TimeTicks(),
        largest_image_->first_size, largest_image_->load_time, image_id,
        image_url, image_element);
  } else {
    DCHECK(largest_text_);
    Node* text_node = DOMNodeIds::NodeForId(largest_text_->node_id);
    // |text_node| could be null and |largest_text_| should be ignored in this
    // case.
    if (!text_node)
      return;

    // Do not expose element attribution from shadow trees.
    Element* text_element =
        text_node->IsInShadowTree() ? nullptr : To<Element>(text_node);
    const AtomicString& text_id =
        text_element ? text_element->GetIdAttribute() : AtomicString();
    window_performance_->OnLargestContentfulPaintUpdated(
        largest_text_->paint_time, largest_text_->first_size, base::TimeTicks(),
        text_id, g_empty_string, text_element);
  }
}

void LargestContentfulPaintCalculator::Trace(Visitor* visitor) {
  visitor->Trace(window_performance_);
}

}  // namespace blink
