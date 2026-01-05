// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GEMINI_CHAT_GEMINI_CHAT_TYPES_H_
#define COMPONENTS_GEMINI_CHAT_GEMINI_CHAT_TYPES_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/time/time.h"
#include "base/values.h"

namespace gemini_chat {

// Represents a Gemini model that can be used for chat.
struct COMPONENT_EXPORT(GEMINI_CHAT) GeminiModel {
  // Unique identifier for the model (e.g., "gemini-2.0-flash")
  std::string id;

  // Display name for the model (e.g., "Gemini 2.0 Flash")
  std::string display_name;

  // Description of the model's capabilities
  std::string description;

  // Maximum input tokens supported by the model
  int max_input_tokens;

  // Maximum output tokens supported by the model
  int max_output_tokens;

  // Whether the model supports multimodal input (images, etc.)
  bool supports_multimodal;

  GeminiModel();
  GeminiModel(const std::string& id,
              const std::string& display_name,
              const std::string& description,
              int max_input_tokens,
              int max_output_tokens,
              bool supports_multimodal);
  GeminiModel(const GeminiModel& other);
  GeminiModel& operator=(const GeminiModel& other);
  ~GeminiModel();

  base::Value::Dict ToValue() const;
  static GeminiModel FromValue(const base::Value::Dict& value);
};

// Role of the message sender
enum class MessageRole {
  kUser,
  kModel,
  kSystem
};

// Represents a single message in a chat conversation.
struct COMPONENT_EXPORT(GEMINI_CHAT) ChatMessage {
  // Unique identifier for the message
  std::string id;

  // Role of the sender (user, model, system)
  MessageRole role;

  // Text content of the message
  std::string content;

  // Timestamp when the message was created
  base::Time timestamp;

  // Whether the message is still being generated (streaming)
  bool is_streaming;

  ChatMessage();
  ChatMessage(const std::string& id,
              MessageRole role,
              const std::string& content);
  ChatMessage(const ChatMessage& other);
  ChatMessage& operator=(const ChatMessage& other);
  ~ChatMessage();

  base::Value::Dict ToValue() const;
  static ChatMessage FromValue(const base::Value::Dict& value);
};

// Represents a chat conversation.
struct COMPONENT_EXPORT(GEMINI_CHAT) ChatConversation {
  // Unique identifier for the conversation
  std::string id;

  // Title of the conversation (auto-generated or user-defined)
  std::string title;

  // The model used for this conversation
  std::string model_id;

  // Messages in the conversation
  std::vector<ChatMessage> messages;

  // Timestamp when the conversation was created
  base::Time created_at;

  // Timestamp when the conversation was last updated
  base::Time updated_at;

  ChatConversation();
  ChatConversation(const std::string& id,
                   const std::string& title,
                   const std::string& model_id);
  ChatConversation(const ChatConversation& other);
  ChatConversation& operator=(const ChatConversation& other);
  ~ChatConversation();

  base::Value::Dict ToValue() const;
  static ChatConversation FromValue(const base::Value::Dict& value);
};

// Request configuration for generating content
struct COMPONENT_EXPORT(GEMINI_CHAT) GenerationConfig {
  // Controls randomness in generation (0.0 - 2.0)
  float temperature;

  // Top-K sampling parameter
  int top_k;

  // Top-P (nucleus) sampling parameter
  float top_p;

  // Maximum tokens to generate
  int max_output_tokens;

  // Stop sequences to end generation
  std::vector<std::string> stop_sequences;

  GenerationConfig();
  GenerationConfig(const GenerationConfig& other);
  GenerationConfig& operator=(const GenerationConfig& other);
  ~GenerationConfig();

  base::Value::Dict ToValue() const;
  static GenerationConfig FromValue(const base::Value::Dict& value);
};

// Result of a Gemini API request
struct COMPONENT_EXPORT(GEMINI_CHAT) GeminiResult {
  // Whether the request was successful
  bool success;

  // Error message if the request failed
  std::string error_message;

  // The generated text content
  std::string content;

  // Token usage information
  int prompt_tokens;
  int completion_tokens;
  int total_tokens;

  GeminiResult();
  GeminiResult(const GeminiResult& other);
  GeminiResult& operator=(const GeminiResult& other);
  ~GeminiResult();
};

// Returns a list of available Gemini models
COMPONENT_EXPORT(GEMINI_CHAT)
std::vector<GeminiModel> GetAvailableModels();

// Returns the default model to use
COMPONENT_EXPORT(GEMINI_CHAT)
GeminiModel GetDefaultModel();

}  // namespace gemini_chat

#endif  // COMPONENTS_GEMINI_CHAT_GEMINI_CHAT_TYPES_H_
