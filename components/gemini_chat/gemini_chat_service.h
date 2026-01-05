// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GEMINI_CHAT_GEMINI_CHAT_SERVICE_H_
#define COMPONENTS_GEMINI_CHAT_GEMINI_CHAT_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/gemini_chat/gemini_chat_types.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace gemini_chat {

// Observer interface for Gemini chat events.
class COMPONENT_EXPORT(GEMINI_CHAT) GeminiChatServiceObserver {
 public:
  virtual ~GeminiChatServiceObserver() = default;

  // Called when streaming content is received.
  virtual void OnStreamingContent(const std::string& conversation_id,
                                  const std::string& content,
                                  bool is_complete) {}

  // Called when an error occurs.
  virtual void OnError(const std::string& conversation_id,
                       const std::string& error_message) {}

  // Called when the available models list is updated.
  virtual void OnModelsUpdated(const std::vector<GeminiModel>& models) {}
};

// Service for interacting with the Gemini AI API.
class COMPONENT_EXPORT(GEMINI_CHAT) GeminiChatService {
 public:
  // Callback types
  using SendMessageCallback = base::OnceCallback<void(const GeminiResult&)>;
  using GetModelsCallback =
      base::OnceCallback<void(const std::vector<GeminiModel>&)>;

  explicit GeminiChatService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~GeminiChatService();

  GeminiChatService(const GeminiChatService&) = delete;
  GeminiChatService& operator=(const GeminiChatService&) = delete;

  // Sets the API key for Gemini requests.
  void SetApiKey(const std::string& api_key);

  // Returns whether the API key is set and valid.
  bool HasValidApiKey() const;

  // Gets the list of available models.
  void GetAvailableModels(GetModelsCallback callback);

  // Sends a message and gets a response from Gemini.
  void SendMessage(const std::string& model_id,
                   const std::vector<ChatMessage>& conversation_history,
                   const std::string& message,
                   const GenerationConfig& config,
                   SendMessageCallback callback);

  // Cancels any ongoing request for the specified conversation.
  void CancelRequest(const std::string& conversation_id);

  // Observer management
  void AddObserver(GeminiChatServiceObserver* observer);
  void RemoveObserver(GeminiChatServiceObserver* observer);

  // Returns the singleton instance
  static GeminiChatService* GetInstance();

 private:
  // Builds the API request body for Gemini.
  std::string BuildRequestBody(
      const std::vector<ChatMessage>& conversation_history,
      const std::string& message,
      const GenerationConfig& config) const;

  // Handles the API response.
  void OnApiResponse(SendMessageCallback callback,
                     std::unique_ptr<std::string> response_body);

  // Parses the Gemini API response.
  GeminiResult ParseResponse(const std::string& response_body) const;

  // Gets the API endpoint URL for the specified model.
  std::string GetApiEndpoint(const std::string& model_id) const;

  // The API key for Gemini requests.
  std::string api_key_;

  // URL loader factory for making HTTP requests.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // Current URL loader for the active request.
  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  // List of observers.
  base::ObserverList<GeminiChatServiceObserver> observers_;

  base::WeakPtrFactory<GeminiChatService> weak_factory_{this};
};

}  // namespace gemini_chat

#endif  // COMPONENTS_GEMINI_CHAT_GEMINI_CHAT_SERVICE_H_
