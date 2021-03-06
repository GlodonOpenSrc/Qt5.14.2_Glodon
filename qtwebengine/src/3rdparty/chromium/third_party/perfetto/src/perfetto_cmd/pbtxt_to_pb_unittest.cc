/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/perfetto_cmd/pbtxt_to_pb.h"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "perfetto/config/trace_config.pb.h"

namespace perfetto {
namespace {

using ::testing::StrictMock;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::google::protobuf::io::ZeroCopyInputStream;
using ::google::protobuf::io::ArrayInputStream;

class MockErrorReporter : public ErrorReporter {
 public:
  MockErrorReporter() {}
  ~MockErrorReporter() = default;
  MOCK_METHOD4(AddError,
               void(size_t line,
                    size_t column_start,
                    size_t column_end,
                    const std::string& message));
};

protos::TraceConfig ToProto(const std::string& input) {
  StrictMock<MockErrorReporter> reporter;
  std::vector<uint8_t> output = PbtxtToPb(input, &reporter);
  EXPECT_FALSE(output.empty());
  protos::TraceConfig config;
  config.ParseFromArray(output.data(), static_cast<int>(output.size()));
  return config;
}

void ToErrors(const std::string& input, MockErrorReporter* reporter) {
  std::vector<uint8_t> output = PbtxtToPb(input, reporter);
}

TEST(PbtxtToPb, OneField) {
  protos::TraceConfig config = ToProto(R"(
    duration_ms: 1234
  )");
  EXPECT_EQ(config.duration_ms(), 1234);
}

TEST(PbtxtToPb, TwoFields) {
  protos::TraceConfig config = ToProto(R"(
    duration_ms: 1234
    file_write_period_ms: 5678
  )");
  EXPECT_EQ(config.duration_ms(), 1234);
  EXPECT_EQ(config.file_write_period_ms(), 5678);
}

TEST(PbtxtToPb, Semicolons) {
  protos::TraceConfig config = ToProto(R"(
    duration_ms: 1234;
    file_write_period_ms: 5678;
  )");
  EXPECT_EQ(config.duration_ms(), 1234);
  EXPECT_EQ(config.file_write_period_ms(), 5678);
}

TEST(PbtxtToPb, NestedMessage) {
  protos::TraceConfig config = ToProto(R"(
    buffers: {
      size_kb: 123
    }
  )");
  ASSERT_EQ(config.buffers().size(), 1);
  EXPECT_EQ(config.buffers().Get(0).size_kb(), 123);
}

TEST(PbtxtToPb, SplitNested) {
  protos::TraceConfig config = ToProto(R"(
    buffers: {
      size_kb: 1
    }
    duration_ms: 1000;
    buffers: {
      size_kb: 2
    }
  )");
  ASSERT_EQ(config.buffers().size(), 2);
  EXPECT_EQ(config.buffers().Get(0).size_kb(), 1);
  EXPECT_EQ(config.buffers().Get(1).size_kb(), 2);
  EXPECT_EQ(config.duration_ms(), 1000);
}

TEST(PbtxtToPb, MultipleNestedMessage) {
  protos::TraceConfig config = ToProto(R"(
    buffers: {
      size_kb: 1
    }
    buffers: {
      size_kb: 2
    }
  )");
  ASSERT_EQ(config.buffers().size(), 2);
  EXPECT_EQ(config.buffers().Get(0).size_kb(), 1);
  EXPECT_EQ(config.buffers().Get(1).size_kb(), 2);
}

TEST(PbtxtToPb, NestedMessageCrossFile) {
  protos::TraceConfig config = ToProto(R"(
data_sources {
  config {
    ftrace_config {
      drain_period_ms: 42
    }
  }
}
  )");
  ASSERT_EQ(
      config.data_sources().Get(0).config().ftrace_config().drain_period_ms(),
      42);
}

TEST(PbtxtToPb, Booleans) {
  protos::TraceConfig config = ToProto(R"(
    write_into_file: false; deferred_start: true;
  )");
  EXPECT_EQ(config.write_into_file(), false);
  EXPECT_EQ(config.deferred_start(), true);
}

TEST(PbtxtToPb, Comments) {
  protos::TraceConfig config = ToProto(R"(
    write_into_file: false # deferred_start: true;
    buffers# 1
    # 2
    :# 3
    # 4
    {# 5
    # 6
    fill_policy# 7
    # 8
    :# 9
    # 10
    RING_BUFFER# 11
    # 12
    ;# 13
    # 14
    } # 15
    # 16
  )");
  EXPECT_EQ(config.write_into_file(), false);
  EXPECT_EQ(config.deferred_start(), false);
}

TEST(PbtxtToPb, Enums) {
  protos::TraceConfig config = ToProto(R"(
    buffers: {
      fill_policy: RING_BUFFER
    }
  )");
  EXPECT_EQ(config.buffers().Get(0).fill_policy(),
            protos::TraceConfig::BufferConfig::RING_BUFFER);
}

TEST(PbtxtToPb, AllFieldTypes) {
  protos::TraceConfig config = ToProto(R"(
data_sources {
  config {
    for_testing {
      dummy_fields {
        field_uint32: 1;
        field_uint64: 2;
        field_int32: 3;
        field_int64: 4;
        field_fixed64: 5;
        field_sfixed64: 6;
        field_fixed32: 7;
        field_sfixed32: 8;
        field_double: 9;
        field_float: 10;
        field_sint64: 11;
        field_sint32: 12;
        field_string: "13";
        field_bytes: "14";
      }
    }
  }
}
  )");
  const auto& fields =
      config.data_sources().Get(0).config().for_testing().dummy_fields();
  ASSERT_EQ(fields.field_uint32(), 1);
  ASSERT_EQ(fields.field_uint64(), 2);
  ASSERT_EQ(fields.field_int32(), 3);
  ASSERT_EQ(fields.field_int64(), 4);
  ASSERT_EQ(fields.field_fixed64(), 5);
  ASSERT_EQ(fields.field_sfixed64(), 6);
  ASSERT_EQ(fields.field_fixed32(), 7);
  ASSERT_EQ(fields.field_sfixed32(), 8);
  ASSERT_EQ(fields.field_double(), 9);
  ASSERT_EQ(fields.field_float(), 10);
  ASSERT_EQ(fields.field_sint64(), 11);
  ASSERT_EQ(fields.field_sint32(), 12);
  ASSERT_EQ(fields.field_string(), "13");
  ASSERT_EQ(fields.field_bytes(), "14");
}

TEST(PbtxtToPb, NegativeNumbers) {
  protos::TraceConfig config = ToProto(R"(
data_sources {
  config {
    for_testing {
      dummy_fields {
        field_int32: -1;
        field_int64: -2;
        field_fixed64: -3;
        field_sfixed64: -4;
        field_fixed32: -5;
        field_sfixed32: -6;
        field_double: -7;
        field_float: -8;
        field_sint64: -9;
        field_sint32: -10;
      }
    }
  }
}
  )");
  const auto& fields =
      config.data_sources().Get(0).config().for_testing().dummy_fields();
  ASSERT_EQ(fields.field_int32(), -1);
  ASSERT_EQ(fields.field_int64(), -2);
  ASSERT_EQ(fields.field_fixed64(), -3);
  ASSERT_EQ(fields.field_sfixed64(), -4);
  ASSERT_EQ(fields.field_fixed32(), -5);
  ASSERT_EQ(fields.field_sfixed32(), -6);
  ASSERT_EQ(fields.field_double(), -7);
  ASSERT_EQ(fields.field_float(), -8);
  ASSERT_EQ(fields.field_sint64(), -9);
  ASSERT_EQ(fields.field_sint32(), -10);
}

TEST(PbtxtToPb, EofEndsNumeric) {
  protos::TraceConfig config = ToProto(R"(duration_ms: 1234)");
  EXPECT_EQ(config.duration_ms(), 1234);
}

TEST(PbtxtToPb, EofEndsIdentifier) {
  protos::TraceConfig config = ToProto(R"(enable_extra_guardrails: true)");
  EXPECT_EQ(config.enable_extra_guardrails(), true);
}

TEST(PbtxtToPb, ExampleConfig) {
  protos::TraceConfig config = ToProto(R"(
buffers {
  size_kb: 100024
  fill_policy: RING_BUFFER
}

data_sources {
  config {
    name: "linux.ftrace"
    target_buffer: 0
    ftrace_config {
      buffer_size_kb: 512 # 4 (page size) * 128
      drain_period_ms: 200
      ftrace_events: "binder_lock"
      ftrace_events: "binder_locked"
      atrace_categories: "gfx"
    }
  }
}

data_sources {
  config {
    name: "linux.process_stats"
    target_buffer: 0
  }
}

data_sources {
  config {
    name: "linux.inode_file_map"
    target_buffer: 0
    inode_file_config {
      scan_delay_ms: 1000
      scan_interval_ms: 1000
      scan_batch_size: 500
      mount_point_mapping: {
        mountpoint: "/data"
        scan_roots: "/data/app"
      }
    }
  }
}

producers {
  producer_name: "perfetto.traced_probes"
  shm_size_kb: 4096
  page_size_kb: 4
}

duration_ms: 10000
)");
  EXPECT_EQ(config.duration_ms(), 10000);
  EXPECT_EQ(config.buffers().Get(0).size_kb(), 100024);
  EXPECT_EQ(config.data_sources().Get(0).config().name(), "linux.ftrace");
  EXPECT_EQ(config.data_sources().Get(0).config().target_buffer(), 0);
  EXPECT_EQ(config.producers().Get(0).producer_name(),
            "perfetto.traced_probes");
}

TEST(PbtxtToPb, Strings) {
  protos::TraceConfig config = ToProto(R"(
data_sources {
  config {
    ftrace_config {
      ftrace_events: "binder_lock"
      ftrace_events: "foo/bar"
      ftrace_events: "foo\\bar"
      ftrace_events: "newline\nnewline"
      ftrace_events: "\"quoted\""
      ftrace_events: "\a\b\f\n\r\t\v\\\'\"\?"
    }
  }
}
)");
  auto events =
      config.data_sources().Get(0).config().ftrace_config().ftrace_events();
  EXPECT_THAT(events, Contains("binder_lock"));
  EXPECT_THAT(events, Contains("foo/bar"));
  EXPECT_THAT(events, Contains("foo\\bar"));
  EXPECT_THAT(events, Contains("newline\nnewline"));
  EXPECT_THAT(events, Contains("\"quoted\""));
  EXPECT_THAT(events, Contains("\a\b\f\n\r\t\v\\\'\"\?"));
}

TEST(PbtxtToPb, UnknownField) {
  MockErrorReporter reporter;
  EXPECT_CALL(reporter,
              AddError(2, 5, 11,
                       "No field named \"not_a_label\" in proto TraceConfig"));
  ToErrors(R"(
    not_a_label: false
  )",
           &reporter);
}

TEST(PbtxtToPb, UnknownNestedField) {
  MockErrorReporter reporter;
  EXPECT_CALL(
      reporter,
      AddError(
          4, 5, 16,
          "No field named \"not_a_field_name\" in proto DataSourceConfig"));
  ToErrors(R"(
data_sources {
  config {
    not_a_field_name {
    }
  }
}
  )",
           &reporter);
}

TEST(PbtxtToPb, BadBoolean) {
  MockErrorReporter reporter;
  EXPECT_CALL(reporter, AddError(2, 22, 3,
                                 "Expected 'true' or 'false' for boolean field "
                                 "write_into_file in proto TraceConfig instead "
                                 "saw 'foo'"));
  ToErrors(R"(
    write_into_file: foo;
  )",
           &reporter);
}

TEST(PbtxtToPb, MissingBoolean) {
  MockErrorReporter reporter;
  EXPECT_CALL(reporter, AddError(3, 3, 0, "Unexpected end of input"));
  ToErrors(R"(
    write_into_file:
  )",
           &reporter);
}

TEST(PbtxtToPb, RootProtoMustNotEndWithBrace) {
  MockErrorReporter reporter;
  EXPECT_CALL(reporter, AddError(2, 5, 0, "Unmatched closing brace"));
  ToErrors(R"(
    }
  )",
           &reporter);
}

TEST(PbtxtToPb, SawNonRepeatedFieldTwice) {
  MockErrorReporter reporter;
  EXPECT_CALL(
      reporter,
      AddError(3, 5, 15,
               "Saw non-repeating field 'write_into_file' more than once"));
  ToErrors(R"(
    write_into_file: true;
    write_into_file: true;
  )",
           &reporter);
}

TEST(PbtxtToPb, WrongTypeBoolean) {
  MockErrorReporter reporter;
  EXPECT_CALL(reporter,
              AddError(2, 18, 4,
                       "Expected value of type uint32 for field duration_ms in "
                       "proto TraceConfig instead saw 'true'"));
  ToErrors(R"(
    duration_ms: true;
  )",
           &reporter);
}

TEST(PbtxtToPb, WrongTypeNumber) {
  MockErrorReporter reporter;
  EXPECT_CALL(reporter,
              AddError(2, 14, 3,
                       "Expected value of type message for field buffers in "
                       "proto TraceConfig instead saw '100'"));
  ToErrors(R"(
    buffers: 100;
  )",
           &reporter);
}

TEST(PbtxtToPb, NestedMessageDidNotTerminate) {
  MockErrorReporter reporter;
  EXPECT_CALL(reporter, AddError(2, 15, 0, "Nested message not closed"));
  ToErrors(R"(
    buffers: {)",
           &reporter);
}

TEST(PbtxtToPb, BadEscape) {
  MockErrorReporter reporter;
  EXPECT_CALL(reporter, AddError(5, 23, 2,
                                 "Unknown string escape in ftrace_events in "
                                 "proto FtraceConfig: '\\p'"));
  ToErrors(R"(
data_sources {
  config {
    ftrace_config {
      ftrace_events: "\p"
    }
  }
})",
           &reporter);
}

// TODO(hjd): Add these tests.
// TEST(PbtxtToPb, WrongTypeString)
// TEST(PbtxtToPb, OverflowOnIntegers)
// TEST(PbtxtToPb, NegativeNumbersForUnsignedInt)
// TEST(PbtxtToPb, UnterminatedString) {
// TEST(PbtxtToPb, NumberIsEof)
// TEST(PbtxtToPb, OneOf)

}  // namespace
}  // namespace perfetto
