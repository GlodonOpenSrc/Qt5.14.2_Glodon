// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_DISPATCHER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_DISPATCHER_H_

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "device/gamepad/public/mojom/gamepad.mojom-blink.h"
#include "third_party/blink/public/platform/web_gamepad_listener.h"
#include "third_party/blink/renderer/core/frame/platform_event_dispatcher.h"

namespace device {
class Gamepad;
class Gamepads;
}  // namespace device

namespace blink {

class GamepadSharedMemoryReader;

class GamepadDispatcher final
    : public GarbageCollectedFinalized<GamepadDispatcher>,
      public PlatformEventDispatcher,
      public WebGamepadListener {
  USING_GARBAGE_COLLECTED_MIXIN(GamepadDispatcher);

 public:
  explicit GamepadDispatcher(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~GamepadDispatcher() override;

  void SampleGamepads(device::Gamepads&);

  void PlayVibrationEffectOnce(uint32_t pad_index,
                               device::mojom::blink::GamepadHapticEffectType,
                               device::mojom::blink::GamepadEffectParametersPtr,
                               device::mojom::blink::GamepadHapticsManager::
                                   PlayVibrationEffectOnceCallback);
  void ResetVibrationActuator(uint32_t pad_index,
                              device::mojom::blink::GamepadHapticsManager::
                                  ResetVibrationActuatorCallback);

  void Trace(blink::Visitor*) override;

 private:
  void InitializeHaptics();

  // WebGamepadListener
  void DidConnectGamepad(uint32_t index, const device::Gamepad&) override;
  void DidDisconnectGamepad(uint32_t index, const device::Gamepad&) override;
  void ButtonOrAxisDidChange(uint32_t index, const device::Gamepad&) override;

  // PlatformEventDispatcher
  void StartListening(LocalFrame* frame) override;
  void StopListening() override;

  void DispatchDidConnectOrDisconnectGamepad(uint32_t index,
                                             const device::Gamepad&,
                                             bool connected);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<GamepadSharedMemoryReader> reader_;
  device::mojom::blink::GamepadHapticsManagerPtr gamepad_haptics_manager_;
};

}  // namespace blink

#endif
