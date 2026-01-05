// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_GEMINI_CHAT_GEMINI_CHAT_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_GEMINI_CHAT_GEMINI_CHAT_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "components/gemini_chat/gemini_chat_service.h"
#include "components/gemini_chat/gemini_chat_types.h"
#include "content/public/browser/web_ui_message_handler.h"

class Profile;

// Handles JavaScript messages for the Gemini Chat WebUI.
class GeminiChatHandler : public content::WebUIMessageHandler,
                          public gemini_chat::GeminiChatServiceObserver {
 public:
  explicit GeminiChatHandler(Profile* profile);
  ~GeminiChatHandler() override;

  GeminiChatHandler(const GeminiChatHandler&) = delete;
  GeminiChatHandler& operator=(const GeminiChatHandler&) = delete;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  // gemini_chat::GeminiChatServiceObserver:
  void OnStreamingContent(const std::string& conversation_id,
                          const std::string& content,
                          bool is_complete) override;
  void OnError(const std::string& conversation_id,
               const std::string& error_message) override;
  void OnModelsUpdated(
      const std::vector<gemini_chat::GeminiModel>& models) override;

 private:
  // Initializes the service with the API key.
  void InitializeService();

  // JavaScript message handlers
  void HandleGetModels(const base::Value::List& args);
  void HandleSendMessage(const base::Value::List& args);
  void HandleSetApiKey(const base::Value::List& args);
  void HandleGetApiKeyStatus(const base::Value::List& args);
  void HandleCancelRequest(const base::Value::List& args);
  void HandleGetConversations(const base::Value::List& args);
  void HandleSaveConversation(const base::Value::List& args);
  void HandleDeleteConversation(const base::Value::List& args);

  // Callbacks for service responses
  void OnModelsReceived(const std::string& callback_id,
                        const std::vector<gemini_chat::GeminiModel>& models);
  void OnMessageResponse(const std::string& callback_id,
                         const gemini_chat::GeminiResult& result);

  raw_ptr<Profile> profile_;
  std::unique_ptr<gemini_chat::GeminiChatService> chat_service_;
  std::vector<gemini_chat::ChatConversation> conversations_;
  std::string current_api_key_;

  base::WeakPtrFactory<GeminiChatHandler> weak_factory_{this};
};

#endif  // CHROME_BROWSER_UI_WEBUI_GEMINI_CHAT_GEMINI_CHAT_HANDLER_H_
