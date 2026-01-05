// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gemini_chat/gemini_chat_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace gemini_chat {

namespace {

// Gemini API constants
constexpr char kGeminiApiBaseUrl[] =
    "https://generativelanguage.googleapis.com/v1beta/models/";
constexpr char kGenerateContentEndpoint[] = ":generateContent";

// Maximum response size (10 MB)
constexpr int kMaxResponseSize = 10 * 1024 * 1024;

// Network traffic annotation for Gemini API requests
constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("gemini_chat_service", R"(
      semantics {
        sender: "Gemini Chat Service"
        description:
          "Sends user messages to the Google Gemini AI API and receives "
          "AI-generated responses for the chat conversation."
        trigger:
          "User sends a message in the Gemini Chat interface."
        data:
          "User's chat messages and conversation history. No personally "
          "identifiable information beyond the content of the messages."
        destination: GOOGLE_OWNED_SERVICE
        internal {
          contacts { email: "chromium-ai-team@google.com" }
        }
        user_data {
          type: OTHER
        }
        last_reviewed: "2024-12-01"
      }
      policy {
        cookies_allowed: NO
        setting:
          "Users can enable or disable this feature through the Gemini Chat "
          "settings. The feature requires a valid Google API key."
        chrome_policy {
          GeminiChatEnabled {
            GeminiChatEnabled: false
          }
        }
      }
    )");

// Singleton instance
GeminiChatService* g_instance = nullptr;

}  // namespace

GeminiChatService::GeminiChatService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(std::move(url_loader_factory)) {
  DCHECK(!g_instance);
  g_instance = this;
}

GeminiChatService::~GeminiChatService() {
  DCHECK_EQ(g_instance, this);
  g_instance = nullptr;
}

// static
GeminiChatService* GeminiChatService::GetInstance() {
  return g_instance;
}

void GeminiChatService::SetApiKey(const std::string& api_key) {
  api_key_ = api_key;
}

bool GeminiChatService::HasValidApiKey() const {
  return !api_key_.empty() && api_key_.length() >= 10;
}

void GeminiChatService::GetAvailableModels(GetModelsCallback callback) {
  // Return the statically defined models
  std::move(callback).Run(gemini_chat::GetAvailableModels());
}

std::string GeminiChatService::GetApiEndpoint(
    const std::string& model_id) const {
  return base::StringPrintf("%s%s%s?key=%s", kGeminiApiBaseUrl, model_id.c_str(),
                            kGenerateContentEndpoint, api_key_.c_str());
}

std::string GeminiChatService::BuildRequestBody(
    const std::vector<ChatMessage>& conversation_history,
    const std::string& message,
    const GenerationConfig& config) const {
  base::Value::Dict request;

  // Build contents array with conversation history
  base::Value::List contents;

  // Add conversation history
  for (const auto& msg : conversation_history) {
    base::Value::Dict content;
    content.Set("role", msg.role == MessageRole::kUser ? "user" : "model");

    base::Value::List parts;
    base::Value::Dict text_part;
    text_part.Set("text", msg.content);
    parts.Append(std::move(text_part));
    content.Set("parts", std::move(parts));

    contents.Append(std::move(content));
  }

  // Add the new user message
  base::Value::Dict user_content;
  user_content.Set("role", "user");

  base::Value::List user_parts;
  base::Value::Dict user_text;
  user_text.Set("text", message);
  user_parts.Append(std::move(user_text));
  user_content.Set("parts", std::move(user_parts));

  contents.Append(std::move(user_content));
  request.Set("contents", std::move(contents));

  // Add generation config
  base::Value::Dict gen_config;
  gen_config.Set("temperature", static_cast<double>(config.temperature));
  gen_config.Set("topK", config.top_k);
  gen_config.Set("topP", static_cast<double>(config.top_p));
  gen_config.Set("maxOutputTokens", config.max_output_tokens);

  if (!config.stop_sequences.empty()) {
    base::Value::List stop_seqs;
    for (const auto& seq : config.stop_sequences) {
      stop_seqs.Append(seq);
    }
    gen_config.Set("stopSequences", std::move(stop_seqs));
  }

  request.Set("generationConfig", std::move(gen_config));

  // Add safety settings to allow reasonable content
  base::Value::List safety_settings;
  for (const char* category :
       {"HARM_CATEGORY_HARASSMENT", "HARM_CATEGORY_HATE_SPEECH",
        "HARM_CATEGORY_SEXUALLY_EXPLICIT", "HARM_CATEGORY_DANGEROUS_CONTENT"}) {
    base::Value::Dict setting;
    setting.Set("category", category);
    setting.Set("threshold", "BLOCK_ONLY_HIGH");
    safety_settings.Append(std::move(setting));
  }
  request.Set("safetySettings", std::move(safety_settings));

  std::string json;
  base::JSONWriter::Write(request, &json);
  return json;
}

void GeminiChatService::SendMessage(
    const std::string& model_id,
    const std::vector<ChatMessage>& conversation_history,
    const std::string& message,
    const GenerationConfig& config,
    SendMessageCallback callback) {
  if (!HasValidApiKey()) {
    GeminiResult result;
    result.success = false;
    result.error_message = "Invalid or missing API key";
    std::move(callback).Run(result);
    return;
  }

  // Cancel any existing request
  url_loader_.reset();

  // Build the request
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(GetApiEndpoint(model_id));
  resource_request->method = "POST";
  resource_request->headers.SetHeader("Content-Type", "application/json");
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->load_flags =
      net::LOAD_DO_NOT_SAVE_COOKIES | net::LOAD_DO_NOT_SEND_COOKIES;

  std::string request_body =
      BuildRequestBody(conversation_history, message, config);

  url_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                  kTrafficAnnotation);
  url_loader_->AttachStringForUpload(request_body, "application/json");
  url_loader_->SetRetryOptions(
      2, network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE);

  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&GeminiChatService::OnApiResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)),
      kMaxResponseSize);
}

void GeminiChatService::OnApiResponse(SendMessageCallback callback,
                                      std::unique_ptr<std::string> response_body) {
  GeminiResult result;

  if (!response_body) {
    int net_error = url_loader_->NetError();
    result.success = false;
    result.error_message =
        base::StringPrintf("Network error: %d", net_error);

    if (url_loader_->ResponseInfo() &&
        url_loader_->ResponseInfo()->headers) {
      int response_code = url_loader_->ResponseInfo()->headers->response_code();
      if (response_code == net::HTTP_UNAUTHORIZED) {
        result.error_message = "Invalid API key. Please check your Google API key.";
      } else if (response_code == net::HTTP_TOO_MANY_REQUESTS) {
        result.error_message = "Rate limit exceeded. Please try again later.";
      } else if (response_code >= 400) {
        result.error_message =
            base::StringPrintf("HTTP error: %d", response_code);
      }
    }

    std::move(callback).Run(result);
    return;
  }

  result = ParseResponse(*response_body);
  std::move(callback).Run(result);
}

GeminiResult GeminiChatService::ParseResponse(
    const std::string& response_body) const {
  GeminiResult result;

  std::optional<base::Value> json = base::JSONReader::Read(response_body);
  if (!json || !json->is_dict()) {
    result.success = false;
    result.error_message = "Failed to parse API response";
    return result;
  }

  const base::Value::Dict& response_dict = json->GetDict();

  // Check for error response
  if (const base::Value::Dict* error = response_dict.FindDict("error")) {
    result.success = false;
    if (const std::string* message = error->FindString("message")) {
      result.error_message = *message;
    } else {
      result.error_message = "Unknown API error";
    }
    return result;
  }

  // Extract the generated content
  if (const base::Value::List* candidates =
          response_dict.FindList("candidates")) {
    if (!candidates->empty() && (*candidates)[0].is_dict()) {
      const base::Value::Dict& candidate = (*candidates)[0].GetDict();

      if (const base::Value::Dict* content = candidate.FindDict("content")) {
        if (const base::Value::List* parts = content->FindList("parts")) {
          std::string full_text;
          for (const auto& part : *parts) {
            if (part.is_dict()) {
              if (const std::string* text =
                      part.GetDict().FindString("text")) {
                full_text += *text;
              }
            }
          }
          result.content = full_text;
          result.success = true;
        }
      }
    }
  }

  // Extract usage metadata if available
  if (const base::Value::Dict* usage_metadata =
          response_dict.FindDict("usageMetadata")) {
    if (std::optional<int> prompt_tokens =
            usage_metadata->FindInt("promptTokenCount")) {
      result.prompt_tokens = *prompt_tokens;
    }
    if (std::optional<int> completion_tokens =
            usage_metadata->FindInt("candidatesTokenCount")) {
      result.completion_tokens = *completion_tokens;
    }
    if (std::optional<int> total_tokens =
            usage_metadata->FindInt("totalTokenCount")) {
      result.total_tokens = *total_tokens;
    }
  }

  if (!result.success && result.content.empty()) {
    result.error_message = "No content generated";
  }

  return result;
}

void GeminiChatService::CancelRequest(const std::string& conversation_id) {
  url_loader_.reset();
}

void GeminiChatService::AddObserver(GeminiChatServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void GeminiChatService::RemoveObserver(GeminiChatServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace gemini_chat
