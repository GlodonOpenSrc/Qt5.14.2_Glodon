// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UI_DEVTOOLS_VIEWS_PAGE_AGENT_VIEWS_H_
#define COMPONENTS_UI_DEVTOOLS_VIEWS_PAGE_AGENT_VIEWS_H_

#include "components/ui_devtools/page_agent.h"

namespace ui_devtools {

class PageAgentViews : public PageAgent {
 public:
  explicit PageAgentViews(DOMAgent* dom_agent);
  ~PageAgentViews() override;

  // PageAgent:
  protocol::Response disable() override;
  protocol::Response reload(protocol::Maybe<bool> bypass_cache) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PageAgentViews);
};

}  // namespace ui_devtools

#endif  // COMPONENTS_UI_DEVTOOLS_VIEWS_PAGE_AGENT_VIEWS_H_
