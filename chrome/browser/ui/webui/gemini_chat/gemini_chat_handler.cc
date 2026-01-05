// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/gemini_chat/gemini_chat_handler.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/gemini_chat/gemini_chat_features.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"

namespace {

// Pref name for storing the Gemini API key.
// Note: In a production implementation, the API key should be stored securely
// using the platform's secure storage mechanism (e.g., Keychain on macOS,
// Credential Manager on Windows, or libsecret on Linux). For this initial
// implementation, we store it in memory only during the session.
constexpr char kGeminiApiKeyPref[] = "gemini_chat.api_key";

}  // namespace

GeminiChatHandler::GeminiChatHandler(Profile* profile)
    : profile_(profile) {}

GeminiChatHandler::~GeminiChatHandler() {
  if (chat_service_) {
    chat_service_->RemoveObserver(this);
  }
}

void GeminiChatHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getModels",
      base::BindRepeating(&GeminiChatHandler::HandleGetModels,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "sendMessage",
      base::BindRepeating(&GeminiChatHandler::HandleSendMessage,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setApiKey",
      base::BindRepeating(&GeminiChatHandler::HandleSetApiKey,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getApiKeyStatus",
      base::BindRepeating(&GeminiChatHandler::HandleGetApiKeyStatus,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "cancelRequest",
      base::BindRepeating(&GeminiChatHandler::HandleCancelRequest,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getConversations",
      base::BindRepeating(&GeminiChatHandler::HandleGetConversations,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "saveConversation",
      base::BindRepeating(&GeminiChatHandler::HandleSaveConversation,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "deleteConversation",
      base::BindRepeating(&GeminiChatHandler::HandleDeleteConversation,
                          base::Unretained(this)));
}

void GeminiChatHandler::OnJavascriptAllowed() {
  InitializeService();
}

void GeminiChatHandler::OnJavascriptDisallowed() {
  if (chat_service_) {
    chat_service_->RemoveObserver(this);
  }
}

void GeminiChatHandler::InitializeService() {
  if (!chat_service_) {
    auto* storage_partition = profile_->GetDefaultStoragePartition();
    chat_service_ = std::make_unique<gemini_chat::GeminiChatService>(
        storage_partition->GetURLLoaderFactoryForBrowserProcess());
    chat_service_->AddObserver(this);

    // Try to load saved API key from prefs
    // Note: In a production environment, the API key should be stored securely
    // For now, we'll rely on the user setting it via the UI
  }
}

void GeminiChatHandler::HandleGetModels(const base::Value::List& args) {
  if (args.empty() || !args[0].is_string()) {
    return;
  }

  std::string callback_id = args[0].GetString();
  AllowJavascript();

  if (!chat_service_) {
    InitializeService();
  }

  chat_service_->GetAvailableModels(
      base::BindOnce(&GeminiChatHandler::OnModelsReceived,
                     weak_factory_.GetWeakPtr(), callback_id));
}

void GeminiChatHandler::OnModelsReceived(
    const std::string& callback_id,
    const std::vector<gemini_chat::GeminiModel>& models) {
  base::Value::List models_list;
  for (const auto& model : models) {
    models_list.Append(model.ToValue());
  }

  ResolveJavascriptCallback(base::Value(callback_id),
                            base::Value(std::move(models_list)));
}

void GeminiChatHandler::HandleSendMessage(const base::Value::List& args) {
  if (args.size() < 4 || !args[0].is_string()) {
    return;
  }

  std::string callback_id = args[0].GetString();
  AllowJavascript();

  if (!chat_service_) {
    InitializeService();
  }

  if (!chat_service_->HasValidApiKey()) {
    base::Value::Dict error;
    error.Set("success", false);
    error.Set("error", "Please set your Google API key first");
    ResolveJavascriptCallback(base::Value(callback_id),
                              base::Value(std::move(error)));
    return;
  }

  // Parse arguments
  std::string model_id = args[1].is_string() ? args[1].GetString() : "gemini-2.0-flash";
  std::string message = args[2].is_string() ? args[2].GetString() : "";

  // Parse conversation history
  std::vector<gemini_chat::ChatMessage> history;
  if (args[3].is_list()) {
    for (const auto& msg : args[3].GetList()) {
      if (msg.is_dict()) {
        history.push_back(gemini_chat::ChatMessage::FromValue(msg.GetDict()));
      }
    }
  }

  // Parse generation config (optional)
  gemini_chat::GenerationConfig config;
  if (args.size() > 4 && args[4].is_dict()) {
    config = gemini_chat::GenerationConfig::FromValue(args[4].GetDict());
  }

  chat_service_->SendMessage(
      model_id, history, message, config,
      base::BindOnce(&GeminiChatHandler::OnMessageResponse,
                     weak_factory_.GetWeakPtr(), callback_id));
}

void GeminiChatHandler::OnMessageResponse(
    const std::string& callback_id,
    const gemini_chat::GeminiResult& result) {
  base::Value::Dict response;
  response.Set("success", result.success);

  if (result.success) {
    response.Set("content", result.content);
    response.Set("promptTokens", result.prompt_tokens);
    response.Set("completionTokens", result.completion_tokens);
    response.Set("totalTokens", result.total_tokens);
  } else {
    response.Set("error", result.error_message);
  }

  ResolveJavascriptCallback(base::Value(callback_id),
                            base::Value(std::move(response)));
}

void GeminiChatHandler::HandleSetApiKey(const base::Value::List& args) {
  if (args.size() < 2 || !args[0].is_string() || !args[1].is_string()) {
    return;
  }

  std::string callback_id = args[0].GetString();
  std::string api_key = args[1].GetString();
  AllowJavascript();

  if (!chat_service_) {
    InitializeService();
  }

  current_api_key_ = api_key;
  chat_service_->SetApiKey(api_key);

  base::Value::Dict response;
  response.Set("success", chat_service_->HasValidApiKey());
  response.Set("hasApiKey", !api_key.empty());

  ResolveJavascriptCallback(base::Value(callback_id),
                            base::Value(std::move(response)));
}

void GeminiChatHandler::HandleGetApiKeyStatus(const base::Value::List& args) {
  if (args.empty() || !args[0].is_string()) {
    return;
  }

  std::string callback_id = args[0].GetString();
  AllowJavascript();

  bool has_valid_key = chat_service_ && chat_service_->HasValidApiKey();

  base::Value::Dict response;
  response.Set("hasApiKey", has_valid_key);
  response.Set("isValid", has_valid_key);

  ResolveJavascriptCallback(base::Value(callback_id),
                            base::Value(std::move(response)));
}

void GeminiChatHandler::HandleCancelRequest(const base::Value::List& args) {
  if (args.size() < 2 || !args[1].is_string()) {
    return;
  }

  std::string conversation_id = args[1].GetString();

  if (chat_service_) {
    chat_service_->CancelRequest(conversation_id);
  }
}

void GeminiChatHandler::HandleGetConversations(const base::Value::List& args) {
  if (args.empty() || !args[0].is_string()) {
    return;
  }

  std::string callback_id = args[0].GetString();
  AllowJavascript();

  base::Value::List conversations_list;
  for (const auto& conversation : conversations_) {
    conversations_list.Append(conversation.ToValue());
  }

  ResolveJavascriptCallback(base::Value(callback_id),
                            base::Value(std::move(conversations_list)));
}

void GeminiChatHandler::HandleSaveConversation(const base::Value::List& args) {
  if (args.size() < 2 || !args[0].is_string() || !args[1].is_dict()) {
    return;
  }

  std::string callback_id = args[0].GetString();
  AllowJavascript();

  gemini_chat::ChatConversation conversation =
      gemini_chat::ChatConversation::FromValue(args[1].GetDict());

  // Find and update existing conversation or add new one
  bool found = false;
  for (auto& existing : conversations_) {
    if (existing.id == conversation.id) {
      existing = conversation;
      found = true;
      break;
    }
  }

  if (!found) {
    conversations_.push_back(conversation);
  }

  base::Value::Dict response;
  response.Set("success", true);
  response.Set("id", conversation.id);

  ResolveJavascriptCallback(base::Value(callback_id),
                            base::Value(std::move(response)));
}

void GeminiChatHandler::HandleDeleteConversation(
    const base::Value::List& args) {
  if (args.size() < 2 || !args[0].is_string() || !args[1].is_string()) {
    return;
  }

  std::string callback_id = args[0].GetString();
  std::string conversation_id = args[1].GetString();
  AllowJavascript();

  std::erase_if(conversations_, [&conversation_id](const auto& c) {
    return c.id == conversation_id;
  });

  base::Value::Dict response;
  response.Set("success", true);

  ResolveJavascriptCallback(base::Value(callback_id),
                            base::Value(std::move(response)));
}

void GeminiChatHandler::OnStreamingContent(const std::string& conversation_id,
                                           const std::string& content,
                                           bool is_complete) {
  base::Value::Dict data;
  data.Set("conversationId", conversation_id);
  data.Set("content", content);
  data.Set("isComplete", is_complete);

  FireWebUIListener("streaming-content", base::Value(std::move(data)));
}

void GeminiChatHandler::OnError(const std::string& conversation_id,
                                const std::string& error_message) {
  base::Value::Dict data;
  data.Set("conversationId", conversation_id);
  data.Set("error", error_message);

  FireWebUIListener("chat-error", base::Value(std::move(data)));
}

void GeminiChatHandler::OnModelsUpdated(
    const std::vector<gemini_chat::GeminiModel>& models) {
  base::Value::List models_list;
  for (const auto& model : models) {
    models_list.Append(model.ToValue());
  }

  FireWebUIListener("models-updated", base::Value(std::move(models_list)));
}
