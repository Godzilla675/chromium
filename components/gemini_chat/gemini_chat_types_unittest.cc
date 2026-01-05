// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gemini_chat/gemini_chat_types.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gemini_chat {
namespace {

TEST(GeminiChatTypesTest, GeminiModelConstruction) {
  GeminiModel model("gemini-2.0-flash", "Gemini 2.0 Flash",
                    "Fast and efficient model", 1000000, 8192, true);

  EXPECT_EQ(model.id, "gemini-2.0-flash");
  EXPECT_EQ(model.display_name, "Gemini 2.0 Flash");
  EXPECT_EQ(model.description, "Fast and efficient model");
  EXPECT_EQ(model.max_input_tokens, 1000000);
  EXPECT_EQ(model.max_output_tokens, 8192);
  EXPECT_TRUE(model.supports_multimodal);
}

TEST(GeminiChatTypesTest, GeminiModelToValueAndBack) {
  GeminiModel original("test-model", "Test Model", "A test model", 500000, 4096,
                       false);

  base::Value::Dict dict = original.ToValue();
  GeminiModel restored = GeminiModel::FromValue(dict);

  EXPECT_EQ(original.id, restored.id);
  EXPECT_EQ(original.display_name, restored.display_name);
  EXPECT_EQ(original.description, restored.description);
  EXPECT_EQ(original.max_input_tokens, restored.max_input_tokens);
  EXPECT_EQ(original.max_output_tokens, restored.max_output_tokens);
  EXPECT_EQ(original.supports_multimodal, restored.supports_multimodal);
}

TEST(GeminiChatTypesTest, ChatMessageConstruction) {
  ChatMessage message("msg-123", MessageRole::kUser, "Hello, world!");

  EXPECT_EQ(message.id, "msg-123");
  EXPECT_EQ(message.role, MessageRole::kUser);
  EXPECT_EQ(message.content, "Hello, world!");
  EXPECT_FALSE(message.is_streaming);
}

TEST(GeminiChatTypesTest, ChatMessageToValueAndBack) {
  ChatMessage original("msg-456", MessageRole::kModel,
                       "This is a model response");
  original.is_streaming = true;

  base::Value::Dict dict = original.ToValue();
  ChatMessage restored = ChatMessage::FromValue(dict);

  EXPECT_EQ(original.id, restored.id);
  EXPECT_EQ(original.role, restored.role);
  EXPECT_EQ(original.content, restored.content);
  EXPECT_EQ(original.is_streaming, restored.is_streaming);
}

TEST(GeminiChatTypesTest, ChatConversationConstruction) {
  ChatConversation conversation("conv-789", "Test Conversation",
                                "gemini-2.0-flash");

  EXPECT_EQ(conversation.id, "conv-789");
  EXPECT_EQ(conversation.title, "Test Conversation");
  EXPECT_EQ(conversation.model_id, "gemini-2.0-flash");
  EXPECT_TRUE(conversation.messages.empty());
}

TEST(GeminiChatTypesTest, ChatConversationWithMessages) {
  ChatConversation conversation("conv-001", "Chat with AI",
                                "gemini-1.5-pro");

  conversation.messages.push_back(
      ChatMessage("msg-1", MessageRole::kUser, "Hello!"));
  conversation.messages.push_back(
      ChatMessage("msg-2", MessageRole::kModel, "Hi there! How can I help?"));

  base::Value::Dict dict = conversation.ToValue();
  ChatConversation restored = ChatConversation::FromValue(dict);

  EXPECT_EQ(restored.id, "conv-001");
  EXPECT_EQ(restored.title, "Chat with AI");
  EXPECT_EQ(restored.model_id, "gemini-1.5-pro");
  ASSERT_EQ(restored.messages.size(), 2u);
  EXPECT_EQ(restored.messages[0].content, "Hello!");
  EXPECT_EQ(restored.messages[1].content, "Hi there! How can I help?");
}

TEST(GeminiChatTypesTest, GenerationConfigDefaults) {
  GenerationConfig config;

  EXPECT_FLOAT_EQ(config.temperature, 1.0f);
  EXPECT_EQ(config.top_k, 40);
  EXPECT_FLOAT_EQ(config.top_p, 0.95f);
  EXPECT_EQ(config.max_output_tokens, 8192);
  EXPECT_TRUE(config.stop_sequences.empty());
}

TEST(GeminiChatTypesTest, GenerationConfigToValueAndBack) {
  GenerationConfig original;
  original.temperature = 0.7f;
  original.top_k = 30;
  original.top_p = 0.9f;
  original.max_output_tokens = 4096;
  original.stop_sequences = {"STOP", "END"};

  base::Value::Dict dict = original.ToValue();
  GenerationConfig restored = GenerationConfig::FromValue(dict);

  EXPECT_FLOAT_EQ(original.temperature, restored.temperature);
  EXPECT_EQ(original.top_k, restored.top_k);
  EXPECT_FLOAT_EQ(original.top_p, restored.top_p);
  EXPECT_EQ(original.max_output_tokens, restored.max_output_tokens);
  ASSERT_EQ(original.stop_sequences.size(), restored.stop_sequences.size());
  EXPECT_EQ(original.stop_sequences[0], restored.stop_sequences[0]);
  EXPECT_EQ(original.stop_sequences[1], restored.stop_sequences[1]);
}

TEST(GeminiChatTypesTest, GetAvailableModels) {
  std::vector<GeminiModel> models = GetAvailableModels();

  // Should have at least one model
  ASSERT_FALSE(models.empty());

  // First model should be Gemini 2.0 Flash
  EXPECT_EQ(models[0].id, "gemini-2.0-flash");
  EXPECT_EQ(models[0].display_name, "Gemini 2.0 Flash");
  EXPECT_TRUE(models[0].supports_multimodal);
}

TEST(GeminiChatTypesTest, GetDefaultModel) {
  GeminiModel default_model = GetDefaultModel();

  EXPECT_EQ(default_model.id, "gemini-2.0-flash");
  EXPECT_FALSE(default_model.display_name.empty());
}

TEST(GeminiChatTypesTest, GeminiResultDefaults) {
  GeminiResult result;

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.error_message.empty());
  EXPECT_TRUE(result.content.empty());
  EXPECT_EQ(result.prompt_tokens, 0);
  EXPECT_EQ(result.completion_tokens, 0);
  EXPECT_EQ(result.total_tokens, 0);
}

}  // namespace
}  // namespace gemini_chat
