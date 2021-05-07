// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_ACCESSIBILITY_AX_EVENT_MANAGER_H_
#define UI_VIEWS_ACCESSIBILITY_AX_EVENT_MANAGER_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/views/views_export.h"

namespace views {

class AXEventObserver;
class View;

// AXEventManager allows observation of accessibility events for all views.
class VIEWS_EXPORT AXEventManager {
 public:
  AXEventManager();
  ~AXEventManager();

  // Returns the singleton instance.
  static AXEventManager* Get();

  void AddObserver(AXEventObserver* observer);
  void RemoveObserver(AXEventObserver* observer);

  // Notifies observers of an accessibility event. |view| must not be null.
  void NotifyViewEvent(views::View* view, ax::mojom::Event event_type);

 private:
  base::ObserverList<AXEventObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(AXEventManager);
};

}  // namespace views

#endif  // UI_VIEWS_ACCESSIBILITY_AX_EVENT_MANAGER_H_