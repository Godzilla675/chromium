// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gemini_chat/gemini_chat_types.h"

#include "base/logging.h"
#include "base/uuid.h"

namespace gemini_chat {

// GeminiModel implementation
GeminiModel::GeminiModel()
    : max_input_tokens(0),
      max_output_tokens(0),
      supports_multimodal(false) {}

GeminiModel::GeminiModel(const std::string& id,
                         const std::string& display_name,
                         const std::string& description,
                         int max_input_tokens,
                         int max_output_tokens,
                         bool supports_multimodal)
    : id(id),
      display_name(display_name),
      description(description),
      max_input_tokens(max_input_tokens),
      max_output_tokens(max_output_tokens),
      supports_multimodal(supports_multimodal) {}

GeminiModel::GeminiModel(const GeminiModel& other) = default;
GeminiModel& GeminiModel::operator=(const GeminiModel& other) = default;
GeminiModel::~GeminiModel() = default;

base::Value::Dict GeminiModel::ToValue() const {
  base::Value::Dict dict;
  dict.Set("id", id);
  dict.Set("displayName", display_name);
  dict.Set("description", description);
  dict.Set("maxInputTokens", max_input_tokens);
  dict.Set("maxOutputTokens", max_output_tokens);
  dict.Set("supportsMultimodal", supports_multimodal);
  return dict;
}

GeminiModel GeminiModel::FromValue(const base::Value::Dict& value) {
  GeminiModel model;
  if (const std::string* id = value.FindString("id")) {
    model.id = *id;
  }
  if (const std::string* display_name = value.FindString("displayName")) {
    model.display_name = *display_name;
  }
  if (const std::string* description = value.FindString("description")) {
    model.description = *description;
  }
  if (std::optional<int> max_input = value.FindInt("maxInputTokens")) {
    model.max_input_tokens = *max_input;
  }
  if (std::optional<int> max_output = value.FindInt("maxOutputTokens")) {
    model.max_output_tokens = *max_output;
  }
  if (std::optional<bool> multimodal = value.FindBool("supportsMultimodal")) {
    model.supports_multimodal = *multimodal;
  }
  return model;
}

// ChatMessage implementation
ChatMessage::ChatMessage()
    : role(MessageRole::kUser),
      timestamp(base::Time::Now()),
      is_streaming(false) {
  id = base::Uuid::GenerateRandomV4().AsLowercaseString();
}

ChatMessage::ChatMessage(const std::string& id,
                         MessageRole role,
                         const std::string& content)
    : id(id),
      role(role),
      content(content),
      timestamp(base::Time::Now()),
      is_streaming(false) {}

ChatMessage::ChatMessage(const ChatMessage& other) = default;
ChatMessage& ChatMessage::operator=(const ChatMessage& other) = default;
ChatMessage::~ChatMessage() = default;

base::Value::Dict ChatMessage::ToValue() const {
  base::Value::Dict dict;
  dict.Set("id", id);
  dict.Set("role", static_cast<int>(role));
  dict.Set("content", content);
  dict.Set("timestamp", timestamp.InMillisecondsFSinceUnixEpoch());
  dict.Set("isStreaming", is_streaming);
  return dict;
}

ChatMessage ChatMessage::FromValue(const base::Value::Dict& value) {
  ChatMessage message;
  if (const std::string* id = value.FindString("id")) {
    message.id = *id;
  }
  if (std::optional<int> role = value.FindInt("role")) {
    message.role = static_cast<MessageRole>(*role);
  }
  if (const std::string* content = value.FindString("content")) {
    message.content = *content;
  }
  if (std::optional<double> timestamp = value.FindDouble("timestamp")) {
    message.timestamp = base::Time::FromMillisecondsSinceUnixEpoch(*timestamp);
  }
  if (std::optional<bool> streaming = value.FindBool("isStreaming")) {
    message.is_streaming = *streaming;
  }
  return message;
}

// ChatConversation implementation
ChatConversation::ChatConversation()
    : created_at(base::Time::Now()),
      updated_at(base::Time::Now()) {
  id = base::Uuid::GenerateRandomV4().AsLowercaseString();
}

ChatConversation::ChatConversation(const std::string& id,
                                   const std::string& title,
                                   const std::string& model_id)
    : id(id),
      title(title),
      model_id(model_id),
      created_at(base::Time::Now()),
      updated_at(base::Time::Now()) {}

ChatConversation::ChatConversation(const ChatConversation& other) = default;
ChatConversation& ChatConversation::operator=(const ChatConversation& other) = default;
ChatConversation::~ChatConversation() = default;

base::Value::Dict ChatConversation::ToValue() const {
  base::Value::Dict dict;
  dict.Set("id", id);
  dict.Set("title", title);
  dict.Set("modelId", model_id);

  base::Value::List messages_list;
  for (const auto& message : messages) {
    messages_list.Append(message.ToValue());
  }
  dict.Set("messages", std::move(messages_list));
  dict.Set("createdAt", created_at.InMillisecondsFSinceUnixEpoch());
  dict.Set("updatedAt", updated_at.InMillisecondsFSinceUnixEpoch());
  return dict;
}

ChatConversation ChatConversation::FromValue(const base::Value::Dict& value) {
  ChatConversation conversation;
  if (const std::string* id = value.FindString("id")) {
    conversation.id = *id;
  }
  if (const std::string* title = value.FindString("title")) {
    conversation.title = *title;
  }
  if (const std::string* model_id = value.FindString("modelId")) {
    conversation.model_id = *model_id;
  }
  if (const base::Value::List* messages_list = value.FindList("messages")) {
    for (const auto& msg_value : *messages_list) {
      if (msg_value.is_dict()) {
        conversation.messages.push_back(ChatMessage::FromValue(msg_value.GetDict()));
      }
    }
  }
  if (std::optional<double> created_at = value.FindDouble("createdAt")) {
    conversation.created_at = base::Time::FromMillisecondsSinceUnixEpoch(*created_at);
  }
  if (std::optional<double> updated_at = value.FindDouble("updatedAt")) {
    conversation.updated_at = base::Time::FromMillisecondsSinceUnixEpoch(*updated_at);
  }
  return conversation;
}

// GenerationConfig implementation
GenerationConfig::GenerationConfig()
    : temperature(1.0f),
      top_k(40),
      top_p(0.95f),
      max_output_tokens(8192) {}

GenerationConfig::GenerationConfig(const GenerationConfig& other) = default;
GenerationConfig& GenerationConfig::operator=(const GenerationConfig& other) = default;
GenerationConfig::~GenerationConfig() = default;

base::Value::Dict GenerationConfig::ToValue() const {
  base::Value::Dict dict;
  dict.Set("temperature", static_cast<double>(temperature));
  dict.Set("topK", top_k);
  dict.Set("topP", static_cast<double>(top_p));
  dict.Set("maxOutputTokens", max_output_tokens);

  base::Value::List stop_list;
  for (const auto& seq : stop_sequences) {
    stop_list.Append(seq);
  }
  dict.Set("stopSequences", std::move(stop_list));
  return dict;
}

GenerationConfig GenerationConfig::FromValue(const base::Value::Dict& value) {
  GenerationConfig config;
  if (std::optional<double> temp = value.FindDouble("temperature")) {
    config.temperature = static_cast<float>(*temp);
  }
  if (std::optional<int> top_k = value.FindInt("topK")) {
    config.top_k = *top_k;
  }
  if (std::optional<double> top_p = value.FindDouble("topP")) {
    config.top_p = static_cast<float>(*top_p);
  }
  if (std::optional<int> max_output = value.FindInt("maxOutputTokens")) {
    config.max_output_tokens = *max_output;
  }
  if (const base::Value::List* stop_list = value.FindList("stopSequences")) {
    for (const auto& seq : *stop_list) {
      if (seq.is_string()) {
        config.stop_sequences.push_back(seq.GetString());
      }
    }
  }
  return config;
}

// GeminiResult implementation
GeminiResult::GeminiResult()
    : success(false),
      prompt_tokens(0),
      completion_tokens(0),
      total_tokens(0) {}

GeminiResult::GeminiResult(const GeminiResult& other) = default;
GeminiResult& GeminiResult::operator=(const GeminiResult& other) = default;
GeminiResult::~GeminiResult() = default;

// Returns a list of available Gemini models (newest models as of 2024)
std::vector<GeminiModel> GetAvailableModels() {
  return {
      GeminiModel(
          "gemini-2.0-flash",
          "Gemini 2.0 Flash",
          "Fastest and most efficient model for everyday tasks. Supports text, "
          "images, and code generation.",
          1048576,  // 1M input tokens
          8192,     // 8K output tokens
          true      // Supports multimodal
      ),
      GeminiModel(
          "gemini-2.0-flash-thinking",
          "Gemini 2.0 Flash Thinking",
          "Enhanced reasoning capabilities with step-by-step thinking for "
          "complex problems.",
          1048576,  // 1M input tokens
          65536,    // 64K output tokens
          true      // Supports multimodal
      ),
      GeminiModel(
          "gemini-1.5-pro",
          "Gemini 1.5 Pro",
          "Most capable model for complex reasoning, analysis, and long-context "
          "understanding. Best for demanding tasks.",
          2097152,  // 2M input tokens
          8192,     // 8K output tokens
          true      // Supports multimodal
      ),
      GeminiModel(
          "gemini-1.5-flash",
          "Gemini 1.5 Flash",
          "Fast and versatile model balanced for speed and capability. Great "
          "for most use cases.",
          1048576,  // 1M input tokens
          8192,     // 8K output tokens
          true      // Supports multimodal
      ),
      GeminiModel(
          "gemini-1.5-flash-8b",
          "Gemini 1.5 Flash-8B",
          "Compact and efficient model optimized for high-volume, lower latency "
          "tasks.",
          1048576,  // 1M input tokens
          8192,     // 8K output tokens
          true      // Supports multimodal
      ),
  };
}

// Returns the default model to use
GeminiModel GetDefaultModel() {
  auto models = GetAvailableModels();
  if (!models.empty()) {
    return models[0];  // Gemini 2.0 Flash is the default
  }
  return GeminiModel();
}

}  // namespace gemini_chat
