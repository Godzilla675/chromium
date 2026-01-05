// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GEMINI_CHAT_UI_GEMINI_CHAT_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_GEMINI_CHAT_UI_GEMINI_CHAT_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

@protocol GeminiChatViewControllerDelegate;

// View controller for the Gemini AI Chat interface.
@interface GeminiChatViewController : UIViewController

// Delegate for handling user interactions.
@property(nonatomic, weak) id<GeminiChatViewControllerDelegate> delegate;

// The currently selected model ID.
@property(nonatomic, copy) NSString* selectedModelId;

// Whether an API key is configured.
@property(nonatomic, assign) BOOL hasApiKey;

// Updates the model selector with available models.
- (void)setAvailableModels:(NSArray<NSDictionary*>*)models;

// Adds a message to the chat UI.
- (void)addMessageWithRole:(NSString*)role content:(NSString*)content;

// Shows the loading indicator.
- (void)setLoading:(BOOL)loading;

// Shows the API key setup view.
- (void)showApiKeySetup;

// Shows the chat interface.
- (void)showChatInterface;

// Displays an error message.
- (void)showError:(NSString*)error;

@end

// Protocol for handling GeminiChatViewController events.
@protocol GeminiChatViewControllerDelegate <NSObject>

// Called when the user sends a message.
- (void)geminiChatViewController:(GeminiChatViewController*)viewController
                     sendMessage:(NSString*)message;

// Called when the user selects a different model.
- (void)geminiChatViewController:(GeminiChatViewController*)viewController
                    selectModel:(NSString*)modelId;

// Called when the user saves an API key.
- (void)geminiChatViewController:(GeminiChatViewController*)viewController
                     saveApiKey:(NSString*)apiKey;

// Called when the user taps the new chat button.
- (void)geminiChatViewControllerDidTapNewChat:
    (GeminiChatViewController*)viewController;

@end

#endif  // IOS_CHROME_BROWSER_GEMINI_CHAT_UI_GEMINI_CHAT_VIEW_CONTROLLER_H_
