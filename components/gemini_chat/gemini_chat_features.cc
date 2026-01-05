// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gemini_chat/gemini_chat_features.h"

#include "base/feature_list.h"

namespace gemini_chat::features {

// Main feature flag to enable/disable Gemini Chat.
// Enabled by default on all platforms.
BASE_FEATURE(kGeminiChat,
             "GeminiChat",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Feature flag for streaming responses.
// Disabled by default as it's experimental.
BASE_FEATURE(kGeminiChatStreaming,
             "GeminiChatStreaming",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Feature flag for multimodal input support.
// Enabled by default as Gemini supports it.
BASE_FEATURE(kGeminiChatMultimodal,
             "GeminiChatMultimodal",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsGeminiChatEnabled() {
  return base::FeatureList::IsEnabled(kGeminiChat);
}

bool IsGeminiChatStreamingEnabled() {
  return base::FeatureList::IsEnabled(kGeminiChatStreaming);
}

bool IsGeminiChatMultimodalEnabled() {
  return base::FeatureList::IsEnabled(kGeminiChatMultimodal);
}

}  // namespace gemini_chat::features
