
/*
 * Copyright 2011 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef SkDrawLooper_DEFINED
#define SkDrawLooper_DEFINED

#include "include/core/SkBlurTypes.h"
#include "include/core/SkColor.h"
#include "include/core/SkFlattenable.h"
#include "include/core/SkPoint.h"

class  SkArenaAlloc;
class  SkCanvas;
class  SkPaint;
struct SkRect;
class  SkString;

/** \class SkDrawLooper
    Subclasses of SkDrawLooper can be attached to a SkPaint. Where they are,
    and something is drawn to a canvas with that paint, the looper subclass will
    be called, allowing it to modify the canvas and/or paint for that draw call.
    More than that, via the next() method, the looper can modify the draw to be
    invoked multiple times (hence the name loop-er), allow it to perform effects
    like shadows or frame/fills, that require more than one pass.
*/
class SK_API SkDrawLooper : public SkFlattenable {
public:
    /**
     *  Holds state during a draw. Users call next() until it returns false.
     *
     *  Subclasses of SkDrawLooper should create a subclass of this object to
     *  hold state specific to their subclass.
     */
    class SK_API Context {
    public:
        Context() {}
        virtual ~Context() {}

        /**
         *  Called in a loop on objects returned by SkDrawLooper::createContext().
         *  Each time true is returned, the object is drawn (possibly with a modified
         *  canvas and/or paint). When false is finally returned, drawing for the object
         *  stops.
         *
         *  On each call, the paint will be in its original state, but the
         *  canvas will be as it was following the previous call to next() or
         *  createContext().
         *
         *  The implementation must ensure that, when next() finally returns
         *  false, the canvas has been restored to the state it was
         *  initially, before createContext() was first called.
         */
        virtual bool next(SkCanvas* canvas, SkPaint* paint) = 0;

    private:
        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;
    };

    /**
     *  Called right before something is being drawn. Returns a Context
     *  whose next() method should be called until it returns false.
     */
    virtual Context* makeContext(SkCanvas*, SkArenaAlloc*) const = 0;

    /**
     * The fast bounds functions are used to enable the paint to be culled early
     * in the drawing pipeline. If a subclass can support this feature it must
     * return true for the canComputeFastBounds() function.  If that function
     * returns false then computeFastBounds behavior is undefined otherwise it
     * is expected to have the following behavior. Given the parent paint and
     * the parent's bounding rect the subclass must fill in and return the
     * storage rect, where the storage rect is with the union of the src rect
     * and the looper's bounding rect.
     */
    bool canComputeFastBounds(const SkPaint& paint) const;
    void computeFastBounds(const SkPaint& paint, const SkRect& src, SkRect* dst) const;

    struct BlurShadowRec {
        SkScalar        fSigma;
        SkVector        fOffset;
        SkColor         fColor;
        SkBlurStyle     fStyle;
    };
    /**
     *  If this looper can be interpreted as having two layers, such that
     *      1. The first layer (bottom most) just has a blur and translate
     *      2. The second layer has no modifications to either paint or canvas
     *      3. No other layers.
     *  then return true, and if not null, fill out the BlurShadowRec).
     *
     *  If any of the above are not met, return false and ignore the BlurShadowRec parameter.
     */
    virtual bool asABlurShadow(BlurShadowRec*) const;

    static SkFlattenable::Type GetFlattenableType() {
        return kSkDrawLooper_Type;
    }

    SkFlattenable::Type getFlattenableType() const override {
        return kSkDrawLooper_Type;
    }

    static sk_sp<SkDrawLooper> Deserialize(const void* data, size_t size,
                                          const SkDeserialProcs* procs = nullptr) {
        return sk_sp<SkDrawLooper>(static_cast<SkDrawLooper*>(
                                  SkFlattenable::Deserialize(
                                  kSkDrawLooper_Type, data, size, procs).release()));
    }

    void apply(SkCanvas* canvas, const SkPaint& paint,
               std::function<void(SkCanvas*, const SkPaint&)>);

protected:
    SkDrawLooper() {}

private:
    typedef SkFlattenable INHERITED;
};

#endif
