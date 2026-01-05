// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/gemini_chat/gemini_chat_ui.h"

#include <memory>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/gemini_chat_resources.h"
#include "chrome/grit/gemini_chat_resources_map.h"
#include "components/gemini_chat/gemini_chat_features.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_allowlist.h"

namespace {

void CreateAndAddGeminiChatSource(Profile* profile) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      profile, chrome::kChromeUIGeminiChatHost);

  // Add resources
  webui::SetupWebUIDataSource(
      source,
      base::make_span(kGeminiChatResources, kGeminiChatResourcesSize),
      IDR_GEMINI_CHAT_GEMINI_CHAT_HTML);

  // Add localized strings
  source->AddLocalizedString("title", IDS_GEMINI_CHAT_TITLE);
  source->AddLocalizedString("send", IDS_GEMINI_CHAT_SEND);
  source->AddLocalizedString("placeholder", IDS_GEMINI_CHAT_PLACEHOLDER);
  source->AddLocalizedString("apiKeyLabel", IDS_GEMINI_CHAT_API_KEY_LABEL);
  source->AddLocalizedString("apiKeyPlaceholder", IDS_GEMINI_CHAT_API_KEY_PLACEHOLDER);
  source->AddLocalizedString("selectModel", IDS_GEMINI_CHAT_SELECT_MODEL);
  source->AddLocalizedString("newChat", IDS_GEMINI_CHAT_NEW_CHAT);
  source->AddLocalizedString("thinking", IDS_GEMINI_CHAT_THINKING);

  // Allow the data source to load
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' 'unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes,
      "trusted-types static-types;");
}

}  // namespace

GeminiChatUIConfig::GeminiChatUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme,
                         chrome::kChromeUIGeminiChatHost) {}

bool GeminiChatUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return gemini_chat::features::IsGeminiChatEnabled();
}

GeminiChatUI::GeminiChatUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);

  // Create the data source
  CreateAndAddGeminiChatSource(profile);

  // Create and add the handler
  handler_ = std::make_unique<GeminiChatHandler>(profile);
  web_ui->AddMessageHandler(std::move(handler_));
}

GeminiChatUI::~GeminiChatUI() = default;
