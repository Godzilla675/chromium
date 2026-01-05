// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/gemini_chat/coordinator/gemini_chat_coordinator.h"

#import "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/gemini_chat/ui/gemini_chat_view_controller.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"

@interface GeminiChatCoordinator () <GeminiChatViewControllerDelegate>

@property(nonatomic, strong) GeminiChatViewController* viewController;
@property(nonatomic, strong) NSMutableArray<NSDictionary*>* messageHistory;
@property(nonatomic, copy) NSString* apiKey;

@end

@implementation GeminiChatCoordinator

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                   browser:(Browser*)browser {
  self = [super initWithBaseViewController:viewController browser:browser];
  if (self) {
    _messageHistory = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void)start {
  [super start];

  self.viewController = [[GeminiChatViewController alloc] init];
  self.viewController.delegate = self;

  UINavigationController* navController = [[UINavigationController alloc]
      initWithRootViewController:self.viewController];
  navController.modalPresentationStyle = UIModalPresentationFullScreen;

  [self.baseViewController presentViewController:navController
                                        animated:YES
                                      completion:nil];

  // Check if API key is set
  if (self.apiKey.length > 0) {
    [self.viewController showChatInterface];
  } else {
    [self.viewController showApiKeySetup];
  }
}

- (void)stop {
  [self.viewController dismissViewControllerAnimated:YES completion:nil];
  self.viewController = nil;
  [super stop];
}

#pragma mark - GeminiChatViewControllerDelegate

- (void)geminiChatViewController:(GeminiChatViewController*)viewController
                     sendMessage:(NSString*)message {
  // Add user message to UI
  [viewController addMessageWithRole:@"user" content:message];

  // Add to history
  [self.messageHistory addObject:@{@"role" : @"user", @"content" : message}];

  // Show loading
  [viewController setLoading:YES];

  // Send to Gemini API
  [self sendMessageToGemini:message
                   modelId:viewController.selectedModelId
                completion:^(NSString* response, NSString* error) {
                  dispatch_async(dispatch_get_main_queue(), ^{
                    [viewController setLoading:NO];

                    if (error) {
                      [viewController addMessageWithRole:@"model"
                                                content:[NSString
                                                    stringWithFormat:@"Error: %@",
                                                                     error]];
                    } else {
                      [viewController addMessageWithRole:@"model"
                                                content:response];
                      [self.messageHistory
                          addObject:@{@"role" : @"model", @"content" : response}];
                    }
                  });
                }];
}

- (void)geminiChatViewController:(GeminiChatViewController*)viewController
                    selectModel:(NSString*)modelId {
  // Model selection is handled by the view controller
}

- (void)geminiChatViewController:(GeminiChatViewController*)viewController
                     saveApiKey:(NSString*)apiKey {
  self.apiKey = apiKey;
  [viewController showChatInterface];
}

- (void)geminiChatViewControllerDidTapNewChat:
    (GeminiChatViewController*)viewController {
  [self.messageHistory removeAllObjects];
}

#pragma mark - Gemini API

- (void)sendMessageToGemini:(NSString*)message
                   modelId:(NSString*)modelId
                completion:(void (^)(NSString*, NSString*))completion {
  if (self.apiKey.length == 0) {
    completion(nil, @"No API key configured");
    return;
  }

  NSString* urlString =
      [NSString stringWithFormat:@"https://generativelanguage.googleapis.com/"
                                 @"v1beta/models/%@:generateContent?key=%@",
                                 modelId, self.apiKey];

  NSURL* url = [NSURL URLWithString:urlString];
  NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
  request.HTTPMethod = @"POST";
  [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];

  // Build request body
  NSMutableArray* contents = [[NSMutableArray alloc] init];

  // Add history
  for (NSDictionary* msg in self.messageHistory) {
    NSString* role =
        [msg[@"role"] isEqualToString:@"user"] ? @"user" : @"model";
    [contents addObject:@{
      @"role" : role,
      @"parts" : @[ @{@"text" : msg[@"content"]} ]
    }];
  }

  // Add new message
  [contents addObject:@{
    @"role" : @"user",
    @"parts" : @[ @{@"text" : message} ]
  }];

  NSDictionary* body = @{
    @"contents" : contents,
    @"generationConfig" : @{
      @"temperature" : @1.0,
      @"topK" : @40,
      @"topP" : @0.95,
      @"maxOutputTokens" : @8192
    },
    @"safetySettings" : @[
      @{
        @"category" : @"HARM_CATEGORY_HARASSMENT",
        @"threshold" : @"BLOCK_ONLY_HIGH"
      },
      @{
        @"category" : @"HARM_CATEGORY_HATE_SPEECH",
        @"threshold" : @"BLOCK_ONLY_HIGH"
      },
      @{
        @"category" : @"HARM_CATEGORY_SEXUALLY_EXPLICIT",
        @"threshold" : @"BLOCK_ONLY_HIGH"
      },
      @{
        @"category" : @"HARM_CATEGORY_DANGEROUS_CONTENT",
        @"threshold" : @"BLOCK_ONLY_HIGH"
      }
    ]
  };

  NSError* jsonError;
  NSData* jsonData = [NSJSONSerialization dataWithJSONObject:body
                                                     options:0
                                                       error:&jsonError];
  if (jsonError) {
    completion(nil, @"Failed to create request");
    return;
  }

  request.HTTPBody = jsonData;

  NSURLSessionDataTask* task = [[NSURLSession sharedSession]
      dataTaskWithRequest:request
        completionHandler:^(NSData* data, NSURLResponse* response,
                            NSError* error) {
          if (error) {
            completion(nil, error.localizedDescription);
            return;
          }

          NSError* parseError;
          NSDictionary* json =
              [NSJSONSerialization JSONObjectWithData:data
                                              options:0
                                                error:&parseError];
          if (parseError) {
            completion(nil, @"Failed to parse response");
            return;
          }

          if (json[@"error"]) {
            completion(nil, json[@"error"][@"message"]);
            return;
          }

          NSArray* candidates = json[@"candidates"];
          if (candidates.count > 0) {
            NSDictionary* content = candidates[0][@"content"];
            NSArray* parts = content[@"parts"];
            if (parts.count > 0) {
              NSString* text = parts[0][@"text"];
              completion(text, nil);
              return;
            }
          }

          completion(nil, @"No response generated");
        }];

  [task resume];
}

@end
