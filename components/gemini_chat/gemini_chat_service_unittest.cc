// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gemini_chat/gemini_chat_service.h"

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gemini_chat {
namespace {

class GeminiChatServiceTest : public testing::Test {
 protected:
  void SetUp() override {
    test_url_loader_factory_ =
        std::make_unique<network::TestURLLoaderFactory>();
    service_ = std::make_unique<GeminiChatService>(
        test_url_loader_factory_->GetSafeWeakWrapper());
  }

  void TearDown() override {
    service_.reset();
    test_url_loader_factory_.reset();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<network::TestURLLoaderFactory> test_url_loader_factory_;
  std::unique_ptr<GeminiChatService> service_;
};

TEST_F(GeminiChatServiceTest, HasValidApiKey_Empty) {
  EXPECT_FALSE(service_->HasValidApiKey());
}

TEST_F(GeminiChatServiceTest, HasValidApiKey_TooShort) {
  service_->SetApiKey("short");
  EXPECT_FALSE(service_->HasValidApiKey());
}

TEST_F(GeminiChatServiceTest, HasValidApiKey_Valid) {
  service_->SetApiKey("valid-api-key-12345");
  EXPECT_TRUE(service_->HasValidApiKey());
}

TEST_F(GeminiChatServiceTest, GetAvailableModels) {
  base::RunLoop run_loop;
  std::vector<GeminiModel> received_models;

  service_->GetAvailableModels(
      base::BindLambdaForTesting([&](const std::vector<GeminiModel>& models) {
        received_models = models;
        run_loop.Quit();
      }));

  run_loop.Run();

  ASSERT_FALSE(received_models.empty());
  EXPECT_EQ(received_models[0].id, "gemini-2.0-flash");
}

TEST_F(GeminiChatServiceTest, SendMessage_NoApiKey) {
  base::RunLoop run_loop;
  GeminiResult received_result;

  std::vector<ChatMessage> history;
  GenerationConfig config;

  service_->SendMessage(
      "gemini-2.0-flash", history, "Hello!",
      config,
      base::BindLambdaForTesting([&](const GeminiResult& result) {
        received_result = result;
        run_loop.Quit();
      }));

  run_loop.Run();

  EXPECT_FALSE(received_result.success);
  EXPECT_EQ(received_result.error_message, "Invalid or missing API key");
}

TEST_F(GeminiChatServiceTest, SendMessage_SuccessfulResponse) {
  service_->SetApiKey("test-api-key-valid");

  // Set up mock response
  const std::string mock_response = R"({
    "candidates": [{
      "content": {
        "parts": [{"text": "Hello! How can I help you today?"}],
        "role": "model"
      }
    }],
    "usageMetadata": {
      "promptTokenCount": 10,
      "candidatesTokenCount": 8,
      "totalTokenCount": 18
    }
  })";

  test_url_loader_factory_->AddResponse(
      "https://generativelanguage.googleapis.com/v1beta/models/"
      "gemini-2.0-flash:generateContent?key=test-api-key-valid",
      mock_response);

  base::RunLoop run_loop;
  GeminiResult received_result;

  std::vector<ChatMessage> history;
  GenerationConfig config;

  service_->SendMessage(
      "gemini-2.0-flash", history, "Hello!",
      config,
      base::BindLambdaForTesting([&](const GeminiResult& result) {
        received_result = result;
        run_loop.Quit();
      }));

  run_loop.Run();

  EXPECT_TRUE(received_result.success);
  EXPECT_EQ(received_result.content, "Hello! How can I help you today?");
  EXPECT_EQ(received_result.prompt_tokens, 10);
  EXPECT_EQ(received_result.completion_tokens, 8);
  EXPECT_EQ(received_result.total_tokens, 18);
}

TEST_F(GeminiChatServiceTest, SendMessage_ErrorResponse) {
  service_->SetApiKey("test-api-key-valid");

  const std::string error_response = R"({
    "error": {
      "code": 400,
      "message": "Invalid request format",
      "status": "INVALID_ARGUMENT"
    }
  })";

  test_url_loader_factory_->AddResponse(
      "https://generativelanguage.googleapis.com/v1beta/models/"
      "gemini-2.0-flash:generateContent?key=test-api-key-valid",
      error_response);

  base::RunLoop run_loop;
  GeminiResult received_result;

  std::vector<ChatMessage> history;
  GenerationConfig config;

  service_->SendMessage(
      "gemini-2.0-flash", history, "Test message",
      config,
      base::BindLambdaForTesting([&](const GeminiResult& result) {
        received_result = result;
        run_loop.Quit();
      }));

  run_loop.Run();

  EXPECT_FALSE(received_result.success);
  EXPECT_EQ(received_result.error_message, "Invalid request format");
}

TEST_F(GeminiChatServiceTest, SendMessage_WithConversationHistory) {
  service_->SetApiKey("test-api-key-valid");

  const std::string mock_response = R"({
    "candidates": [{
      "content": {
        "parts": [{"text": "I remember our conversation!"}],
        "role": "model"
      }
    }]
  })";

  test_url_loader_factory_->AddResponse(
      "https://generativelanguage.googleapis.com/v1beta/models/"
      "gemini-1.5-pro:generateContent?key=test-api-key-valid",
      mock_response);

  base::RunLoop run_loop;
  GeminiResult received_result;

  std::vector<ChatMessage> history;
  history.push_back(ChatMessage("1", MessageRole::kUser, "Previous message"));
  history.push_back(ChatMessage("2", MessageRole::kModel, "Previous response"));

  GenerationConfig config;
  config.temperature = 0.7f;
  config.max_output_tokens = 2048;

  service_->SendMessage(
      "gemini-1.5-pro", history, "New message",
      config,
      base::BindLambdaForTesting([&](const GeminiResult& result) {
        received_result = result;
        run_loop.Quit();
      }));

  run_loop.Run();

  EXPECT_TRUE(received_result.success);
  EXPECT_EQ(received_result.content, "I remember our conversation!");
}

TEST_F(GeminiChatServiceTest, GetInstance) {
  EXPECT_EQ(GeminiChatService::GetInstance(), service_.get());
}

}  // namespace
}  // namespace gemini_chat
