// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ui_devtools/views/view_element.h"

#include "base/strings/utf_string_conversions.h"
#include "components/ui_devtools/Protocol.h"
#include "components/ui_devtools/ui_devtools_unittest_utils.h"
#include "ui/views/test/views_test_base.h"

namespace ui_devtools {

namespace {

// Returns a pair of the ClassProperties index and UIProperty index
std::pair<size_t, size_t> GetPropertyIndices(ui_devtools::ViewElement* element,
                                             const std::string& property_name) {
  std::vector<UIElement::ClassProperties> props =
      element->GetCustomPropertiesForMatchedStyle();
  for (size_t cp_index = 0; cp_index < props.size(); ++cp_index) {
    for (size_t uip_index = 0; uip_index < props[cp_index].properties_.size();
         ++uip_index) {
      if (props[cp_index].properties_[uip_index].name_ == property_name)
        return std::make_pair(cp_index, uip_index);
    }
  }
  DCHECK(false) << "Property " << property_name << " can not be found.";
  return std::make_pair(-1, -1);
}

void TestBooleanCustomPropertySetting(ui_devtools::ViewElement* element,
                                      const std::string& property_name,
                                      bool init_value) {
  std::pair<size_t, size_t> indices =
      GetPropertyIndices(element, property_name);
  std::string old_value(init_value ? "true" : "false");
  std::vector<UIElement::ClassProperties> props =
      element->GetCustomPropertiesForMatchedStyle();
  std::vector<UIElement::UIProperty> ui_props =
      props[indices.first].properties_;
  EXPECT_EQ(ui_props[indices.second].value_, old_value);

  // Check the property can be set accordingly.
  std::string new_value(init_value ? "false" : "true");
  std::string separator(":");
  element->SetPropertiesFromString(property_name + separator + new_value);
  props = element->GetCustomPropertiesForMatchedStyle();
  ui_props = props[indices.first].properties_;
  EXPECT_EQ(ui_props[indices.second].name_, property_name);
  EXPECT_EQ(ui_props[indices.second].value_, new_value);

  element->SetPropertiesFromString(property_name + separator + old_value);
  props = element->GetCustomPropertiesForMatchedStyle();
  ui_props = props[indices.first].properties_;
  EXPECT_EQ(ui_props[indices.second].name_, property_name);
  EXPECT_EQ(ui_props[indices.second].value_, old_value);
}

}  // namespace

using ::testing::_;

class NamedTestView : public views::View {
 public:
  METADATA_HEADER(NamedTestView);

  // For custom properties test.
  base::string16 GetTooltipText(const gfx::Point& p) const override {
    return base::ASCIIToUTF16("This is the tooltip");
  }

  int GetBoolProperty() const { return bool_property_; }
  void SetBoolProperty(bool bool_property) { bool_property_ = bool_property; }

 private:
  bool bool_property_ = false;
};

BEGIN_METADATA(NamedTestView)
METADATA_PARENT_CLASS(views::View)
ADD_PROPERTY_METADATA(NamedTestView, bool, BoolProperty)
END_METADATA()

class ViewElementTest : public views::ViewsTestBase {
 public:
  ViewElementTest() {}
  ~ViewElementTest() override {}

 protected:
  void SetUp() override {
    views::ViewsTestBase::SetUp();
    view_.reset(new NamedTestView);
    delegate_.reset(new testing::NiceMock<MockUIElementDelegate>);
    // |OnUIElementAdded| is called on element creation.
    EXPECT_CALL(*delegate_, OnUIElementAdded(_, _)).Times(1);
    element_.reset(new ViewElement(view_.get(), delegate_.get(), nullptr));
  }

  NamedTestView* view() { return view_.get(); }
  ViewElement* element() { return element_.get(); }
  MockUIElementDelegate* delegate() { return delegate_.get(); }

 private:
  std::unique_ptr<NamedTestView> view_;
  std::unique_ptr<ViewElement> element_;
  std::unique_ptr<MockUIElementDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(ViewElementTest);
};

TEST_F(ViewElementTest, SettingsBoundsOnViewCallsDelegate) {
  EXPECT_CALL(*delegate(), OnUIElementBoundsChanged(element())).Times(1);
  view()->SetBounds(1, 2, 3, 4);
}

TEST_F(ViewElementTest, AddingChildView) {
  // The first call is from the element being created, before it
  // gets parented to |element_|.
  EXPECT_CALL(*delegate(), OnUIElementAdded(nullptr, _)).Times(1);
  EXPECT_CALL(*delegate(), OnUIElementAdded(element(), _)).Times(1);
  views::View child_view;
  view()->AddChildView(&child_view);

  DCHECK_EQ(element()->children().size(), 1U);
  UIElement* child_element = element()->children()[0];

  EXPECT_CALL(*delegate(), OnUIElementRemoved(child_element)).Times(1);
  view()->RemoveChildView(&child_view);
}

TEST_F(ViewElementTest, SettingsBoundsOnElementSetsOnView) {
  DCHECK(view()->bounds() == gfx::Rect());

  element()->SetBounds(gfx::Rect(1, 2, 3, 4));
  EXPECT_EQ(view()->bounds(), gfx::Rect(1, 2, 3, 4));
}

TEST_F(ViewElementTest, SetPropertiesFromString) {
  static const char* kEnabledProperty = "Enabled";
  TestBooleanCustomPropertySetting(element(), kEnabledProperty, true);
  std::pair<size_t, size_t> indices =
      GetPropertyIndices(element(), kEnabledProperty);

  // Test setting a non-existent property has no effect.
  element()->SetPropertiesFromString("Enable:false");
  std::vector<UIElement::ClassProperties> props =
      element()->GetCustomPropertiesForMatchedStyle();
  std::vector<UIElement::UIProperty> ui_props =
      props[indices.first].properties_;

  EXPECT_EQ(ui_props[indices.second].name_, kEnabledProperty);
  EXPECT_EQ(ui_props[indices.second].value_, "true");

  // Test setting empty string for property value has no effect.
  element()->SetPropertiesFromString("Enabled:");
  props = element()->GetCustomPropertiesForMatchedStyle();
  ui_props = props[indices.first].properties_;
  EXPECT_EQ(ui_props[indices.second].name_, kEnabledProperty);
  EXPECT_EQ(ui_props[indices.second].value_, "true");

  // Ensure setting pure whitespace doesn't crash.
  ASSERT_NO_FATAL_FAILURE(element()->SetPropertiesFromString("   \n  "));
}

TEST_F(ViewElementTest, SettingVisibilityOnView) {
  TestBooleanCustomPropertySetting(element(), "Visible", true);
}

TEST_F(ViewElementTest, SettingSubclassBoolProperty) {
  TestBooleanCustomPropertySetting(element(), "BoolProperty", false);
}

TEST_F(ViewElementTest, GetBounds) {
  gfx::Rect bounds;

  view()->SetBounds(10, 20, 30, 40);
  element()->GetBounds(&bounds);
  EXPECT_EQ(bounds, gfx::Rect(10, 20, 30, 40));
}

TEST_F(ViewElementTest, GetAttributes) {
  std::vector<std::string> attrs = element()->GetAttributes();
  EXPECT_THAT(attrs,
              testing::ElementsAre("name", NamedTestView::kViewClassName));
}

TEST_F(ViewElementTest, GetCustomProperties) {
  std::vector<UIElement::ClassProperties> props =
      element()->GetCustomPropertiesForMatchedStyle();

  // The ClassProperties vector should be of size 2.
  DCHECK_EQ(props.size(), 2U);

  std::pair<size_t, size_t> indices =
      GetPropertyIndices(element(), "BoolProperty");
  // The BoolProperty property should be in the second ClassProperties object in
  // the vector.
  DCHECK_EQ(indices.first, 0U);

  indices = GetPropertyIndices(element(), "tooltip");
  // The tooltip property should be in the second ClassProperties object in the
  // vector.
  DCHECK_EQ(indices.first, 1U);

  std::vector<UIElement::UIProperty> ui_props =
      props[indices.first].properties_;

  EXPECT_EQ(ui_props[indices.second].name_, "tooltip");
  EXPECT_EQ(ui_props[indices.second].value_, "This is the tooltip");
}

TEST_F(ViewElementTest, CheckCustomProperties) {
  std::vector<UIElement::ClassProperties> props =
      element()->GetCustomPropertiesForMatchedStyle();
  DCHECK_GT(props.size(), 1U);
  DCHECK_GT(props[1].properties_.size(), 1U);

  // Check visibility information is passed in.
  bool is_visible_set = false;
  for (size_t i = 0; i < props[1].properties_.size(); ++i) {
    if (props[1].properties_[i].name_ == "Visible")
      is_visible_set = true;
  }
  EXPECT_TRUE(is_visible_set);
}

TEST_F(ViewElementTest, GetNodeWindowAndScreenBounds) {
  // For this to be meaningful, the view must be in
  // a widget.
  auto widget = std::make_unique<views::Widget>();
  views::Widget::InitParams params =
      CreateParams(views::Widget::InitParams::TYPE_WINDOW);
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  widget->Init(params);
  widget->Show();

  widget->GetContentsView()->AddChildView(view());
  gfx::Rect bounds(50, 60, 70, 80);
  view()->SetBoundsRect(bounds);

  std::pair<gfx::NativeWindow, gfx::Rect> window_and_bounds =
      element()->GetNodeWindowAndScreenBounds();
  EXPECT_EQ(window_and_bounds.first, widget->GetNativeWindow());
  EXPECT_EQ(window_and_bounds.second, view()->GetBoundsInScreen());

  view()->parent()->RemoveChildView(view());
}

}  // namespace ui_devtools
