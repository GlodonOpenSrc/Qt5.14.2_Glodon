// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_android.h"

#include <memory>

#include "cc/layers/deadline_policy.h"
#include "cc/layers/layer.h"
#include "components/viz/common/surfaces/local_surface_id_allocation.h"
#include "content/browser/renderer_host/mock_render_widget_host.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/test/mock_render_widget_host_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/android/view_android.h"

namespace content {

class RenderWidgetHostViewAndroidTest : public testing::Test {
 public:
  RenderWidgetHostViewAndroidTest();
  ~RenderWidgetHostViewAndroidTest() override {}

  RenderWidgetHostViewAndroid* render_widget_host_view_android() {
    return render_widget_host_view_android_;
  }

  // Directly map to RenderWidgetHostViewAndroid methods.
  bool SynchronizeVisualProperties(
      const cc::DeadlinePolicy& deadline_policy,
      const base::Optional<viz::LocalSurfaceIdAllocation>&
          child_local_surface_id_allocation);
  void WasEvicted();

 protected:
  // testing::Test:
  void SetUp() override;
  void TearDown() override;

 private:
  std::unique_ptr<TestBrowserContext> browser_context_;
  MockRenderProcessHost* process_;  // Deleted automatically by the widget.
  std::unique_ptr<MockRenderWidgetHostDelegate> delegate_;
  scoped_refptr<cc::Layer> parent_layer_;
  scoped_refptr<cc::Layer> layer_;
  ui::ViewAndroid parent_view_;
  ui::ViewAndroid native_view_;
  std::unique_ptr<MockRenderWidgetHost> host_;
  RenderWidgetHostViewAndroid* render_widget_host_view_android_;

  TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostViewAndroidTest);
};

RenderWidgetHostViewAndroidTest::RenderWidgetHostViewAndroidTest()
    : parent_view_(ui::ViewAndroid::LayoutType::NORMAL),
      native_view_(ui::ViewAndroid::LayoutType::NORMAL) {}

bool RenderWidgetHostViewAndroidTest::SynchronizeVisualProperties(
    const cc::DeadlinePolicy& deadline_policy,
    const base::Optional<viz::LocalSurfaceIdAllocation>&
        child_local_surface_id_allocation) {
  return render_widget_host_view_android_->SynchronizeVisualProperties(
      deadline_policy, child_local_surface_id_allocation);
}

void RenderWidgetHostViewAndroidTest::WasEvicted() {
  render_widget_host_view_android_->WasEvicted();
}

void RenderWidgetHostViewAndroidTest::SetUp() {
  browser_context_.reset(new TestBrowserContext());
  delegate_.reset(new MockRenderWidgetHostDelegate());
  process_ = new MockRenderProcessHost(browser_context_.get());
  host_.reset(MockRenderWidgetHost::Create(delegate_.get(), process_,
                                           process_->GetNextRoutingID()));
  parent_layer_ = cc::Layer::Create();
  parent_view_.SetLayer(parent_layer_);
  layer_ = cc::Layer::Create();
  native_view_.SetLayer(layer_);
  parent_view_.AddChild(&native_view_);
  EXPECT_EQ(&parent_view_, native_view_.parent());
  render_widget_host_view_android_ =
      new RenderWidgetHostViewAndroid(host_.get(), &native_view_);
}

void RenderWidgetHostViewAndroidTest::TearDown() {
  render_widget_host_view_android_->Destroy();
  host_.reset();
  delegate_.reset();
  process_ = nullptr;
  browser_context_.reset();
}

// Tests that when a child responds to a Surface Synchronization message, while
// we are evicted, that we do not attempt to embed an invalid
// viz::LocalSurfaceId. This test should not crash.
TEST_F(RenderWidgetHostViewAndroidTest, NoSurfaceSynchronizationWhileEvicted) {
  // Android default host and views initialize as visible.
  RenderWidgetHostViewAndroid* rwhva = render_widget_host_view_android();
  EXPECT_TRUE(rwhva->IsShowing());
  const viz::LocalSurfaceIdAllocation initial_allocation =
      rwhva->GetLocalSurfaceIdAllocation();
  EXPECT_TRUE(initial_allocation.local_surface_id().is_valid());

  // Evicting while hidden should invalidate the current viz::LocalSurfaceId.
  rwhva->Hide();
  EXPECT_FALSE(rwhva->IsShowing());
  WasEvicted();
  EXPECT_FALSE(
      rwhva->GetLocalSurfaceIdAllocation().local_surface_id().is_valid());

  // When a child acknowledges a Surface Synchronization message, and has no new
  // properties to change, it responds with the original viz::LocalSurfaceId.
  // If we are evicted, we should not attempt to embed our invalid id. Nor
  // should we continue the synchronization process. This should not cause a
  // crash in DelegatedFrameHostAndroid.
  EXPECT_FALSE(SynchronizeVisualProperties(
      cc::DeadlinePolicy::UseDefaultDeadline(), initial_allocation));
}

// Tests insetting the Visual Viewport.
TEST_F(RenderWidgetHostViewAndroidTest, InsetVisualViewport) {
  // Android default viewport.
  RenderWidgetHostViewAndroid* rwhva = render_widget_host_view_android();
  int width = rwhva->GetViewBounds().width();
  int height = rwhva->GetViewBounds().height();
  gfx::Size full_open_size(width, height);
  EXPECT_EQ(full_open_size, rwhva->GetVisibleViewportSize());

  // Set up SurfaceId checking.
  const viz::LocalSurfaceIdAllocation& surface_id_allocation =
      rwhva->GetLocalSurfaceIdAllocation();
  viz::LocalSurfaceId original_surface =
      surface_id_allocation.local_surface_id();

  // Inset all around.
  gfx::Insets insets(2, 3, 4, 5);
  rwhva->SetInsets(insets);
  gfx::Size inset_size(width - insets.left() - insets.right(),
                       height - insets.top() - insets.bottom());
  EXPECT_EQ(inset_size, rwhva->GetVisibleViewportSize());
  viz::LocalSurfaceId inset_surface = surface_id_allocation.local_surface_id();
  EXPECT_TRUE(inset_surface.IsNewerThan(original_surface));

  // Reset insets; should go back to the original size as a new surface.
  rwhva->SetInsets(gfx::Insets());
  EXPECT_EQ(full_open_size, rwhva->GetVisibleViewportSize());
  EXPECT_TRUE(
      surface_id_allocation.local_surface_id().IsNewerThan(inset_surface));
}

}  // namespace content
