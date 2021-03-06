// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_MOCK_COMPOUND_RTCP_PARSER_CLIENT_H_
#define STREAMING_CAST_MOCK_COMPOUND_RTCP_PARSER_CLIENT_H_

#include "gmock/gmock.h"
#include "streaming/cast/compound_rtcp_parser.h"

namespace openscreen {
namespace cast_streaming {

class MockCompoundRtcpParserClient : public CompoundRtcpParser::Client {
 public:
  MOCK_METHOD1(OnReceiverReferenceTimeAdvanced,
               void(platform::Clock::time_point reference_time));
  MOCK_METHOD1(OnReceiverReport, void(const RtcpReportBlock& receiver_report));
  MOCK_METHOD0(OnReceiverIndicatesPictureLoss, void());
  MOCK_METHOD2(OnReceiverCheckpoint,
               void(FrameId frame_id, std::chrono::milliseconds playout_delay));
  MOCK_METHOD1(OnReceiverHasFrames, void(std::vector<FrameId> acks));
  MOCK_METHOD1(OnReceiverIsMissingPackets, void(std::vector<PacketNack> nacks));
};

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_MOCK_COMPOUND_RTCP_PARSER_CLIENT_H_
