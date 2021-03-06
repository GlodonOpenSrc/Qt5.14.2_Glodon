/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm/gm.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkShader.h"
#include "include/core/SkSize.h"
#include "include/core/SkString.h"
#include "include/core/SkTileMode.h"
#include "include/core/SkTypes.h"
#include "include/effects/SkColorMatrixFilter.h"
#include "include/effects/SkGradientShader.h"

static sk_sp<SkShader> make_shader(const SkRect& bounds) {
    const SkPoint pts[] = {
        { bounds.left(), bounds.top() },
        { bounds.right(), bounds.bottom() },
    };
    const SkColor colors[] = {
        SK_ColorRED, SK_ColorGREEN, SK_ColorBLUE, SK_ColorBLACK,
        SK_ColorCYAN, SK_ColorMAGENTA, SK_ColorYELLOW,
    };
    return SkGradientShader::MakeLinear(pts, colors, nullptr, SK_ARRAY_COUNT(colors),
                                        SkTileMode::kClamp);
}

typedef void (*InstallPaint)(SkPaint*, uint32_t, uint32_t);

static void install_nothing(SkPaint* paint, uint32_t, uint32_t) {
    paint->setColorFilter(nullptr);
}

static void install_lighting(SkPaint* paint, uint32_t mul, uint32_t add) {
    paint->setColorFilter(SkColorMatrixFilter::MakeLightingFilter(mul, add));
}

class ColorFiltersGM : public skiagm::GM {
    SkString onShortName() override { return SkString("lightingcolorfilter"); }

    SkISize onISize() override { return {620, 430}; }

    void onDraw(SkCanvas* canvas) override {
        SkRect r = {0, 0, 600, 50};

        SkPaint paint;
        paint.setShader(make_shader(r));

        const struct {
            InstallPaint    fProc;
            uint32_t        fData0, fData1;
        } rec[] = {
            { install_nothing, 0, 0 },
            { install_lighting, 0xFF0000, 0 },
            { install_lighting, 0x00FF00, 0 },
            { install_lighting, 0x0000FF, 0 },
            { install_lighting, 0x000000, 0xFF0000 },
            { install_lighting, 0x000000, 0x00FF00 },
            { install_lighting, 0x000000, 0x0000FF },
        };

        canvas->translate(10, 10);
        for (size_t i = 0; i < SK_ARRAY_COUNT(rec); ++i) {
            rec[i].fProc(&paint, rec[i].fData0, rec[i].fData1);
            canvas->drawRect(r, paint);
            canvas->translate(0, r.height() + 10);
        }
    }
};

DEF_GM(return new ColorFiltersGM;)
