// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/cfx_unicodeencodingex.h"

#include <memory>

#include "core/fxge/cfx_font.h"
#include "core/fxge/fx_font.h"
#include "core/fxge/fx_freetype.h"
#include "third_party/base/ptr_util.h"

namespace {

const uint32_t g_EncodingID[] = {
    FXFM_ENCODING_MS_SYMBOL,     FXFM_ENCODING_UNICODE,
    FXFM_ENCODING_MS_SJIS,       FXFM_ENCODING_MS_GB2312,
    FXFM_ENCODING_MS_BIG5,       FXFM_ENCODING_MS_WANSUNG,
    FXFM_ENCODING_MS_JOHAB,      FXFM_ENCODING_ADOBE_STANDARD,
    FXFM_ENCODING_ADOBE_EXPERT,  FXFM_ENCODING_ADOBE_CUSTOM,
    FXFM_ENCODING_ADOBE_LATIN_1, FXFM_ENCODING_OLD_LATIN_2,
    FXFM_ENCODING_APPLE_ROMAN,
};

std::unique_ptr<CFX_UnicodeEncodingEx> FXFM_CreateFontEncoding(
    CFX_Font* pFont,
    uint32_t nEncodingID) {
  if (FXFT_Select_Charmap(pFont->GetFaceRec(), nEncodingID))
    return nullptr;
  return pdfium::MakeUnique<CFX_UnicodeEncodingEx>(pFont, nEncodingID);
}

}  // namespace

CFX_UnicodeEncodingEx::CFX_UnicodeEncodingEx(CFX_Font* pFont,
                                             uint32_t EncodingID)
    : CFX_UnicodeEncoding(pFont), m_nEncodingID(EncodingID) {}

CFX_UnicodeEncodingEx::~CFX_UnicodeEncodingEx() {}

uint32_t CFX_UnicodeEncodingEx::GlyphFromCharCode(uint32_t charcode) {
  FXFT_FaceRec* face = m_pFont->GetFaceRec();
  FT_UInt nIndex = FT_Get_Char_Index(face, charcode);
  if (nIndex > 0)
    return nIndex;
  int nmaps = FXFT_Get_Face_CharmapCount(face);
  int m = 0;
  while (m < nmaps) {
    uint32_t nEncodingID =
        FXFT_Get_Charmap_Encoding(FXFT_Get_Face_Charmaps(face)[m++]);
    if (m_nEncodingID == nEncodingID)
      continue;
    int error = FXFT_Select_Charmap(face, nEncodingID);
    if (error)
      continue;
    nIndex = FT_Get_Char_Index(face, charcode);
    if (nIndex > 0) {
      m_nEncodingID = nEncodingID;
      return nIndex;
    }
  }
  FXFT_Select_Charmap(face, m_nEncodingID);
  return 0;
}

uint32_t CFX_UnicodeEncodingEx::CharCodeFromUnicode(wchar_t Unicode) const {
  if (m_nEncodingID == FXFM_ENCODING_UNICODE ||
      m_nEncodingID == FXFM_ENCODING_MS_SYMBOL) {
    return Unicode;
  }
  FXFT_FaceRec* face = m_pFont->GetFaceRec();
  int nmaps = FXFT_Get_Face_CharmapCount(face);
  for (int i = 0; i < nmaps; i++) {
    int nEncodingID =
        FXFT_Get_Charmap_Encoding(FXFT_Get_Face_Charmaps(face)[i]);
    if (nEncodingID == FXFM_ENCODING_UNICODE ||
        nEncodingID == FXFM_ENCODING_MS_SYMBOL) {
      return Unicode;
    }
  }
  return kInvalidCharCode;
}

std::unique_ptr<CFX_UnicodeEncodingEx> FX_CreateFontEncodingEx(
    CFX_Font* pFont) {
  if (!pFont || !pFont->GetFaceRec())
    return nullptr;

  for (uint32_t id : g_EncodingID) {
    auto pFontEncoding = FXFM_CreateFontEncoding(pFont, id);
    if (pFontEncoding)
      return pFontEncoding;
  }
  return nullptr;
}
