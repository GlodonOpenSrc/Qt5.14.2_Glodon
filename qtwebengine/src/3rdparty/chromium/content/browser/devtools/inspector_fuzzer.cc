// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "third_party/inspector_protocol/encoding/encoding.h"

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  using namespace inspector_protocol_encoding;

  if (size > 64 * 1024) {
    return 0;
  }

  span<uint8_t> fuzz{data, size};

  // We need to handle whatever the parser parses. So, we handle the parsed
  // stuff with another CBOR encoder, just because it's conveniently available.
  std::vector<uint8_t> encoded;
  Status status;
  std::unique_ptr<StreamingParserHandler> encoder =
      cbor::NewCBOREncoder(&encoded, &status);

  cbor::ParseCBOR(fuzz, encoder.get());

  return 0;
}