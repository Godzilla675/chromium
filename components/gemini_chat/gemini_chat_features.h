// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GEMINI_CHAT_GEMINI_CHAT_FEATURES_H_
#define COMPONENTS_GEMINI_CHAT_GEMINI_CHAT_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace gemini_chat::features {

// Main feature flag to enable/disable Gemini Chat.
COMPONENT_EXPORT(GEMINI_CHAT)
BASE_DECLARE_FEATURE(kGeminiChat);

// Feature flag for streaming responses (experimental).
COMPONENT_EXPORT(GEMINI_CHAT)
BASE_DECLARE_FEATURE(kGeminiChatStreaming);

// Feature flag for multimodal input support (images, etc.).
COMPONENT_EXPORT(GEMINI_CHAT)
BASE_DECLARE_FEATURE(kGeminiChatMultimodal);

// Returns whether Gemini Chat is enabled.
COMPONENT_EXPORT(GEMINI_CHAT) bool IsGeminiChatEnabled();

// Returns whether streaming responses are enabled.
COMPONENT_EXPORT(GEMINI_CHAT) bool IsGeminiChatStreamingEnabled();

// Returns whether multimodal input is enabled.
COMPONENT_EXPORT(GEMINI_CHAT) bool IsGeminiChatMultimodalEnabled();

}  // namespace gemini_chat::features

#endif  // COMPONENTS_GEMINI_CHAT_GEMINI_CHAT_FEATURES_H_
