// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_CPDFSDK_BAANNOTHANDLER_H_
#define FPDFSDK_CPDFSDK_BAANNOTHANDLER_H_

#include <memory>

#include "core/fxcrt/fx_coordinates.h"
#include "fpdfsdk/ipdfsdk_annothandler.h"

class CFFL_InteractiveFormFiller;
class CFX_Matrix;
class CFX_RenderDevice;
class CPDF_Annot;
class CPDFSDK_FormFillEnvironment;
class CPDFSDK_Annot;
class CPDFSDK_PageView;

#ifdef PDF_ENABLE_XFA
class CXFA_FFWidget;
#endif  // PDF_ENABLE_XFA

class CPDFSDK_BAAnnotHandler final : public IPDFSDK_AnnotHandler {
 public:
  CPDFSDK_BAAnnotHandler();
  ~CPDFSDK_BAAnnotHandler() override;

  bool CanAnswer(CPDFSDK_Annot* pAnnot) override;
  CPDFSDK_Annot* NewAnnot(CPDF_Annot* pAnnot, CPDFSDK_PageView* pPage) override;
#ifdef PDF_ENABLE_XFA
  CPDFSDK_Annot* NewAnnot(CXFA_FFWidget* hWidget,
                          CPDFSDK_PageView* pPage) override;
#endif  // PDF_ENABLE_XFA
  void ReleaseAnnot(std::unique_ptr<CPDFSDK_Annot> pAnnot) override;
  CFX_FloatRect GetViewBBox(CPDFSDK_PageView* pPageView,
                            CPDFSDK_Annot* pAnnot) override;
  WideString GetText(CPDFSDK_Annot* pAnnot) override;
  WideString GetSelectedText(CPDFSDK_Annot* pAnnot) override;
  void ReplaceSelection(CPDFSDK_Annot* pAnnot, const WideString& text) override;
  bool CanUndo(CPDFSDK_Annot* pAnnot) override;
  bool CanRedo(CPDFSDK_Annot* pAnnot) override;
  bool Undo(CPDFSDK_Annot* pAnnot) override;
  bool Redo(CPDFSDK_Annot* pAnnot) override;
  bool HitTest(CPDFSDK_PageView* pPageView,
               CPDFSDK_Annot* pAnnot,
               const CFX_PointF& point) override;
  void OnDraw(CPDFSDK_PageView* pPageView,
              CPDFSDK_Annot* pAnnot,
              CFX_RenderDevice* pDevice,
              const CFX_Matrix& mtUser2Device,
              bool bDrawAnnots) override;
  void OnLoad(CPDFSDK_Annot* pAnnot) override;

  void OnMouseEnter(CPDFSDK_PageView* pPageView,
                    ObservedPtr<CPDFSDK_Annot>* pAnnot,
                    uint32_t nFlag) override;
  void OnMouseExit(CPDFSDK_PageView* pPageView,
                   ObservedPtr<CPDFSDK_Annot>* pAnnot,
                   uint32_t nFlag) override;
  bool OnLButtonDown(CPDFSDK_PageView* pPageView,
                     ObservedPtr<CPDFSDK_Annot>* pAnnot,
                     uint32_t nFlags,
                     const CFX_PointF& point) override;
  bool OnLButtonUp(CPDFSDK_PageView* pPageView,
                   ObservedPtr<CPDFSDK_Annot>* pAnnot,
                   uint32_t nFlags,
                   const CFX_PointF& point) override;
  bool OnLButtonDblClk(CPDFSDK_PageView* pPageView,
                       ObservedPtr<CPDFSDK_Annot>* pAnnot,
                       uint32_t nFlags,
                       const CFX_PointF& point) override;
  bool OnMouseMove(CPDFSDK_PageView* pPageView,
                   ObservedPtr<CPDFSDK_Annot>* pAnnot,
                   uint32_t nFlags,
                   const CFX_PointF& point) override;
  bool OnMouseWheel(CPDFSDK_PageView* pPageView,
                    ObservedPtr<CPDFSDK_Annot>* pAnnot,
                    uint32_t nFlags,
                    short zDelta,
                    const CFX_PointF& point) override;
  bool OnRButtonDown(CPDFSDK_PageView* pPageView,
                     ObservedPtr<CPDFSDK_Annot>* pAnnot,
                     uint32_t nFlags,
                     const CFX_PointF& point) override;
  bool OnRButtonUp(CPDFSDK_PageView* pPageView,
                   ObservedPtr<CPDFSDK_Annot>* pAnnot,
                   uint32_t nFlags,
                   const CFX_PointF& point) override;
  bool OnRButtonDblClk(CPDFSDK_PageView* pPageView,
                       ObservedPtr<CPDFSDK_Annot>* pAnnot,
                       uint32_t nFlags,
                       const CFX_PointF& point) override;
  bool OnChar(CPDFSDK_Annot* pAnnot, uint32_t nChar, uint32_t nFlags) override;
  bool OnKeyDown(CPDFSDK_Annot* pAnnot, int nKeyCode, int nFlag) override;
  bool OnKeyUp(CPDFSDK_Annot* pAnnot, int nKeyCode, int nFlag) override;
  bool OnSetFocus(ObservedPtr<CPDFSDK_Annot>* pAnnot, uint32_t nFlag) override;
  bool OnKillFocus(ObservedPtr<CPDFSDK_Annot>* pAnnot, uint32_t nFlag) override;
  bool SetIndexSelected(ObservedPtr<CPDFSDK_Annot>* pAnnot,
                        int index,
                        bool selected) override;
  bool IsIndexSelected(ObservedPtr<CPDFSDK_Annot>* pAnnot, int index) override;
#ifdef PDF_ENABLE_XFA
  bool OnXFAChangedFocus(ObservedPtr<CPDFSDK_Annot>* pOldAnnot,
                         ObservedPtr<CPDFSDK_Annot>* pNewAnnot) override;
#endif  // PDF_ENABLE_XFA
};

#endif  // FPDFSDK_CPDFSDK_BAANNOTHANDLER_H_
