// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/public/cpp/testing_json_parser.h"

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace data_decoder {
namespace {

const char kTestJson[] = "{\"key\":2}";

class TestingJsonParserTest : public testing::Test {
 public:
  void Parse(const std::string& input) {
    base::RunLoop run_loop;
    SafeJsonParser::Parse(
        /* connector=*/nullptr, input,
        base::BindOnce(&SuccessCallback, base::Unretained(this),
                       run_loop.QuitClosure()),
        base::BindOnce(&ErrorCallback, base::Unretained(this),
                       run_loop.QuitClosure()));
    run_loop.Run();
  }

  bool did_success() const { return did_success_; }
  bool did_error() const { return did_error_; }

 private:
  static void SuccessCallback(TestingJsonParserTest* test,
                              base::Closure quit_closure,
                              base::Value value) {
    test->did_success_ = true;
    quit_closure.Run();

    ASSERT_TRUE(value.is_dict());
    base::Optional<int> key_value = value.FindIntKey("key");
    ASSERT_TRUE(key_value.has_value());
    EXPECT_EQ(2, *key_value);
  }

  static void ErrorCallback(TestingJsonParserTest* test,
                            base::Closure quit_closure,
                            const std::string& error) {
    test->did_error_ = true;
    quit_closure.Run();

    EXPECT_FALSE(error.empty());
  }

  base::test::ScopedTaskEnvironment scoped_task_environment;
  TestingJsonParser::ScopedFactoryOverride factory_override_;
  bool did_success_ = false;
  bool did_error_ = false;
};

TEST_F(TestingJsonParserTest, QuitLoopInSuccessCallback) {
  Parse(kTestJson);
  EXPECT_TRUE(did_success());
  EXPECT_FALSE(did_error());
}

TEST_F(TestingJsonParserTest, QuitLoopInErrorCallback) {
  Parse(&kTestJson[1]);
  EXPECT_FALSE(did_success());
  EXPECT_TRUE(did_error());
}

}  // namespace
}  // namespace data_decoder
