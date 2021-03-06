// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_LAYOUT_INTERPOLATING_LAYOUT_MANAGER_H_
#define UI_VIEWS_LAYOUT_INTERPOLATING_LAYOUT_MANAGER_H_

#include <map>
#include <memory>
#include <utility>

#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/layout/layout_manager_base.h"

namespace views {

// Layout which interpolates between multiple embedded LayoutManagerBase
// layouts.
//
// An InterpolatingLayoutManager has a default layout, which applies at the
// smallest layout size along the layout's major axis (defined by |orientation|)
// and additional layouts, which phase in at some larger size. If only the
// default layout is set, the layout is functionally equivalent to the default
// layout.
//
// An example:
//
//   InterpolatingLayoutManager* e =
//       new InterpolatingLayoutManager(LayoutOrientation::kHorizontal);
//   e->AddLayout(std::make_unique<CompactLayout>());
//   e->AddLayout(std::make_unique<NormalLayout>(), {50, 0});
//   e->AddLayout(std::make_unique<SpaciousLayout>(), {100, 50});
//
// Now as the view expands, the different layouts are used:
//
// 0              50            100            150
// |   Compact    |    Normal    | Norm <~> Spa |  Spacious ->
//
// In the range from 100 to 150 (exclusive), an interpolation of the Normal and
// Spacious layouts is used. When interpolation happens in this way, the
// visibility of views is the conjunction of the visibilities in each layout, so
// if either layout hides a view then the interpolated layout also hides it.
// Since this can produce some unwanted visual results, we recommend making sure
// that over the interpolation range, visibility matches up between the layouts
// on either side.
//
// Note that behavior when interpolation ranges overlap is undefined, but will
// be guaranteed to at least be the result of mixing two adjacent layouts that
// fall over the range in a way that is not completely irrational.
class VIEWS_EXPORT InterpolatingLayoutManager : public LayoutManagerBase {
 public:
  InterpolatingLayoutManager();
  ~InterpolatingLayoutManager() override;

  InterpolatingLayoutManager& SetOrientation(LayoutOrientation orientation);
  LayoutOrientation orientation() const { return orientation_; }

  // Adds a layout which starts and finished phasing in at |start_interpolation|
  // and |end_interpolation|, respectively. Currently, having more than one
  // layout's interpolation range overlapping results in undefined behavior.
  //
  // This object retains ownership of the layout engine, but the method returns
  // a typed raw pointer to the added layout engine.
  template <class T>
  T* AddLayout(std::unique_ptr<T> layout_manager,
               const Span& interpolation_range = Span()) {
    T* const temp = layout_manager.get();
    AddLayoutInternal(std::move(layout_manager), interpolation_range);
    return temp;
  }

  // Specifies which layout is default (i.e. will be used for determining
  // preferred layout size). If you do not set this, the largest layout will be
  // used.
  void SetDefaultLayout(LayoutManagerBase* default_layout);

  // LayoutManagerBase:
  gfx::Size GetPreferredSize(const View* host) const override;
  gfx::Size GetMinimumSize(const View* host) const override;
  int GetPreferredHeightForWidth(const View* host, int width) const override;
  void InvalidateLayout() override;
  void SetChildViewIgnoredByLayout(View* child_view, bool ignored) override;

 protected:
  // LayoutManagerBase:
  void Installed(View* host_view) override;
  void ViewAdded(View* host_view, View* child_view) override;
  void ViewRemoved(View* host_view, View* child_view) override;
  void ViewVisibilitySet(View* host, View* view, bool visible) override;
  ProposedLayout CalculateProposedLayout(
      const SizeBounds& size_bounds) const override;

 private:
  // Describes an interpolation between two layouts as a pointer to each and
  // a percentage of distance between them to interpolate linearly to.
  struct LayoutInterpolation {
    LayoutManagerBase* first = nullptr;
    LayoutManagerBase* second = nullptr;

    // The closer this number is to zero, the more of |first| is used; the
    // closer to 1.0f, the more of |second|. If the value is 0, |second| may be
    // null.
    float percent_second = 0.0f;
  };

  void AddLayoutInternal(std::unique_ptr<LayoutManagerBase> layout,
                         const Span& interpolation_range);

  // Given a set of size bounds and the current layout's orientation, returns
  // a LayoutInterpolation providing the two layouts to interpolate between.
  // If only one layout applies, only |right| is set and |percent| is set to 1.
  LayoutInterpolation GetInterpolation(const SizeBounds& bounds) const;

  // Returns the default layout, or the largest layout if the default has not
  // been set.
  const LayoutManagerBase* GetDefaultLayout() const;

  // Returns the smallest layout; useful for calculating minimum layout size.
  const LayoutManagerBase* GetSmallestLayout() const;

  LayoutOrientation orientation_ = LayoutOrientation::kHorizontal;

  // Maps from interpolation range to embedded layout.
  std::map<Span, std::unique_ptr<LayoutManagerBase>> embedded_layouts_;
  LayoutManagerBase* default_layout_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(InterpolatingLayoutManager);
};

}  // namespace views

#endif  // UI_VIEWS_LAYOUT_INTERPOLATING_LAYOUT_MANAGER_H_
