// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_IMAGE_DIFF_IMAGE_DIFF_PNG_H_
#define TESTING_IMAGE_DIFF_IMAGE_DIFF_PNG_H_

#include <stdlib.h>  // for size_t.

#include <vector>

namespace image_diff_png {

// Decode a PNG into an RGBA pixel array.
std::vector<unsigned char> DecodePNG(const unsigned char* input,
                                     size_t input_size,
                                     int* width,
                                     int* height);

// Encode a BGR pixel array into a PNG.
std::vector<unsigned char> EncodeBGRPNG(const unsigned char* input,
                                        int width,
                                        int height,
                                        int row_byte_width);

// Encode an RGBA pixel array into a PNG.
std::vector<unsigned char> EncodeRGBAPNG(const unsigned char* input,
                                         int width,
                                         int height,
                                         int row_byte_width);

// Encode an BGRA pixel array into a PNG.
std::vector<unsigned char> EncodeBGRAPNG(const unsigned char* input,
                                         int width,
                                         int height,
                                         int row_byte_width,
                                         bool discard_transparency);

// Encode a grayscale pixel array into a PNG.
std::vector<unsigned char> EncodeGrayPNG(const unsigned char* input,
                                         int width,
                                         int height,
                                         int row_byte_width);

}  // namespace image_diff_png

#endif  // TESTING_IMAGE_DIFF_IMAGE_DIFF_PNG_H_
