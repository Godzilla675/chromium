// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_GEMINI_CHAT_GEMINI_CHAT_UI_H_
#define CHROME_BROWSER_UI_WEBUI_GEMINI_CHAT_GEMINI_CHAT_UI_H_

#include "chrome/browser/ui/webui/gemini_chat/gemini_chat_handler.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"

namespace content {
class WebUI;
}  // namespace content

class GeminiChatUI;

// WebUI config for chrome://gemini-chat
class GeminiChatUIConfig : public content::DefaultWebUIConfig<GeminiChatUI> {
 public:
  GeminiChatUIConfig();
  ~GeminiChatUIConfig() override = default;

  // content::WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

// The WebUI controller for chrome://gemini-chat
class GeminiChatUI : public content::WebUIController {
 public:
  explicit GeminiChatUI(content::WebUI* web_ui);
  ~GeminiChatUI() override;

  GeminiChatUI(const GeminiChatUI&) = delete;
  GeminiChatUI& operator=(const GeminiChatUI&) = delete;

 private:
  std::unique_ptr<GeminiChatHandler> handler_;
};

#endif  // CHROME_BROWSER_UI_WEBUI_GEMINI_CHAT_GEMINI_CHAT_UI_H_
