// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GEMINI_CHAT_COORDINATOR_GEMINI_CHAT_COORDINATOR_H_
#define IOS_CHROME_BROWSER_GEMINI_CHAT_COORDINATOR_GEMINI_CHAT_COORDINATOR_H_

#import "ios/chrome/browser/shared/coordinator/chrome_coordinator/chrome_coordinator.h"

class Browser;

// Coordinator for the Gemini AI Chat feature.
@interface GeminiChatCoordinator : ChromeCoordinator

// Initializes the coordinator with the given browser.
- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                   browser:(Browser*)browser;

@end

#endif  // IOS_CHROME_BROWSER_GEMINI_CHAT_COORDINATOR_GEMINI_CHAT_COORDINATOR_H_
