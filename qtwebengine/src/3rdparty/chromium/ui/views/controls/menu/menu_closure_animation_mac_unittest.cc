// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/menu/menu_closure_animation_mac.h"

#include "base/test/bind_test_util.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/menu_test_utils.h"

TEST(MenuClosureAnimationMacTest, DestructCancelsCleanly) {
  views::test::DisableMenuClosureAnimations();
  base::test::ScopedTaskEnvironment environment;

  bool called = false;
  auto animation = std::make_unique<views::MenuClosureAnimationMac>(
      nullptr, nullptr, base::BindLambdaForTesting([&]() { called = true; }));

  animation->Start();
  animation.reset();

  // If the animation callback runs after the animation is destroyed, this line
  // may crash; if not, |called| will get set to true and fail the test below.
  views::test::WaitForMenuClosureAnimation();

  EXPECT_FALSE(called);
}
