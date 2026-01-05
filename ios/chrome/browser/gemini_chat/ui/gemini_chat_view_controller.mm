// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/gemini_chat/ui/gemini_chat_view_controller.h"

#import <UIKit/UIKit.h>

@interface GeminiChatViewController () <UITextFieldDelegate>

// UI Elements
@property(nonatomic, strong) UIStackView* mainStack;
@property(nonatomic, strong) UIView* headerView;
@property(nonatomic, strong) UIButton* modelSelector;
@property(nonatomic, strong) UIScrollView* scrollView;
@property(nonatomic, strong) UIStackView* messagesStack;
@property(nonatomic, strong) UIView* inputContainer;
@property(nonatomic, strong) UITextField* messageInput;
@property(nonatomic, strong) UIButton* sendButton;
@property(nonatomic, strong) UIActivityIndicatorView* loadingIndicator;
@property(nonatomic, strong) UIView* apiKeySetupView;
@property(nonatomic, strong) UITextField* apiKeyInput;

// Data
@property(nonatomic, strong) NSArray<NSDictionary*>* models;

@end

@implementation GeminiChatViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  self.view.backgroundColor = [UIColor colorWithRed:0.102
                                              green:0.102
                                               blue:0.180
                                              alpha:1.0];
  self.title = @"Gemini AI Chat";

  [self setupUI];
  [self setDefaultModels];
}

- (void)setupUI {
  // Main stack view
  self.mainStack = [[UIStackView alloc] init];
  self.mainStack.axis = UILayoutConstraintAxisVertical;
  self.mainStack.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:self.mainStack];

  // Header with model selector
  [self setupHeader];

  // Messages scroll view
  [self setupMessagesArea];

  // Input area
  [self setupInputArea];

  // API Key setup view (hidden by default)
  [self setupApiKeySetupView];

  // Constraints
  [NSLayoutConstraint activateConstraints:@[
    [self.mainStack.topAnchor
        constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],
    [self.mainStack.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.mainStack.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    [self.mainStack.bottomAnchor
        constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor],
  ]];
}

- (void)setupHeader {
  self.headerView = [[UIView alloc] init];
  self.headerView.backgroundColor = [UIColor colorWithRed:0.086
                                                    green:0.129
                                                     blue:0.243
                                                    alpha:1.0];
  self.headerView.translatesAutoresizingMaskIntoConstraints = NO;

  UILabel* modelLabel = [[UILabel alloc] init];
  modelLabel.text = @"Model:";
  modelLabel.textColor = [UIColor lightGrayColor];
  modelLabel.font = [UIFont systemFontOfSize:14];
  modelLabel.translatesAutoresizingMaskIntoConstraints = NO;

  self.modelSelector = [UIButton buttonWithType:UIButtonTypeSystem];
  [self.modelSelector setTitle:@"Gemini 2.0 Flash"
                      forState:UIControlStateNormal];
  [self.modelSelector setTitleColor:[UIColor whiteColor]
                           forState:UIControlStateNormal];
  self.modelSelector.translatesAutoresizingMaskIntoConstraints = NO;
  [self.modelSelector addTarget:self
                         action:@selector(showModelPicker)
               forControlEvents:UIControlEventTouchUpInside];

  UIButton* newChatButton = [UIButton buttonWithType:UIButtonTypeSystem];
  [newChatButton setTitle:@"+ New Chat" forState:UIControlStateNormal];
  [newChatButton setTitleColor:[UIColor systemBlueColor]
                      forState:UIControlStateNormal];
  newChatButton.translatesAutoresizingMaskIntoConstraints = NO;
  [newChatButton addTarget:self
                    action:@selector(newChatTapped)
          forControlEvents:UIControlEventTouchUpInside];

  [self.headerView addSubview:modelLabel];
  [self.headerView addSubview:self.modelSelector];
  [self.headerView addSubview:newChatButton];
  [self.mainStack addArrangedSubview:self.headerView];

  [NSLayoutConstraint activateConstraints:@[
    [self.headerView.heightAnchor constraintEqualToConstant:50],
    [modelLabel.leadingAnchor
        constraintEqualToAnchor:self.headerView.leadingAnchor
                       constant:16],
    [modelLabel.centerYAnchor
        constraintEqualToAnchor:self.headerView.centerYAnchor],
    [self.modelSelector.leadingAnchor
        constraintEqualToAnchor:modelLabel.trailingAnchor
                       constant:8],
    [self.modelSelector.centerYAnchor
        constraintEqualToAnchor:self.headerView.centerYAnchor],
    [newChatButton.trailingAnchor
        constraintEqualToAnchor:self.headerView.trailingAnchor
                       constant:-16],
    [newChatButton.centerYAnchor
        constraintEqualToAnchor:self.headerView.centerYAnchor],
  ]];
}

- (void)setupMessagesArea {
  self.scrollView = [[UIScrollView alloc] init];
  self.scrollView.translatesAutoresizingMaskIntoConstraints = NO;

  self.messagesStack = [[UIStackView alloc] init];
  self.messagesStack.axis = UILayoutConstraintAxisVertical;
  self.messagesStack.spacing = 12;
  self.messagesStack.translatesAutoresizingMaskIntoConstraints = NO;

  [self.scrollView addSubview:self.messagesStack];
  [self.mainStack addArrangedSubview:self.scrollView];

  [NSLayoutConstraint activateConstraints:@[
    [self.messagesStack.topAnchor
        constraintEqualToAnchor:self.scrollView.topAnchor
                       constant:16],
    [self.messagesStack.leadingAnchor
        constraintEqualToAnchor:self.scrollView.leadingAnchor
                       constant:16],
    [self.messagesStack.trailingAnchor
        constraintEqualToAnchor:self.scrollView.trailingAnchor
                       constant:-16],
    [self.messagesStack.bottomAnchor
        constraintEqualToAnchor:self.scrollView.bottomAnchor
                       constant:-16],
    [self.messagesStack.widthAnchor
        constraintEqualToAnchor:self.scrollView.widthAnchor
                       constant:-32],
  ]];

  // Add welcome message
  [self addWelcomeMessage];
}

- (void)setupInputArea {
  self.inputContainer = [[UIView alloc] init];
  self.inputContainer.backgroundColor = [UIColor colorWithRed:0.086
                                                        green:0.129
                                                         blue:0.243
                                                        alpha:1.0];
  self.inputContainer.translatesAutoresizingMaskIntoConstraints = NO;

  self.messageInput = [[UITextField alloc] init];
  self.messageInput.placeholder = @"Type your message...";
  self.messageInput.textColor = [UIColor whiteColor];
  self.messageInput.backgroundColor = [UIColor colorWithRed:0.118
                                                      green:0.118
                                                       blue:0.188
                                                      alpha:1.0];
  self.messageInput.layer.cornerRadius = 8;
  self.messageInput.leftView =
      [[UIView alloc] initWithFrame:CGRectMake(0, 0, 12, 0)];
  self.messageInput.leftViewMode = UITextFieldViewModeAlways;
  self.messageInput.translatesAutoresizingMaskIntoConstraints = NO;
  self.messageInput.delegate = self;
  [self.messageInput addTarget:self
                        action:@selector(textFieldDidChange:)
              forControlEvents:UIControlEventEditingChanged];

  self.sendButton = [UIButton buttonWithType:UIButtonTypeSystem];
  [self.sendButton setTitle:@"➤" forState:UIControlStateNormal];
  [self.sendButton setTitleColor:[UIColor whiteColor]
                        forState:UIControlStateNormal];
  self.sendButton.backgroundColor = [UIColor systemBlueColor];
  self.sendButton.layer.cornerRadius = 20;
  self.sendButton.translatesAutoresizingMaskIntoConstraints = NO;
  self.sendButton.enabled = NO;
  [self.sendButton addTarget:self
                      action:@selector(sendMessage)
            forControlEvents:UIControlEventTouchUpInside];

  self.loadingIndicator = [[UIActivityIndicatorView alloc]
      initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleMedium];
  self.loadingIndicator.color = [UIColor systemBlueColor];
  self.loadingIndicator.translatesAutoresizingMaskIntoConstraints = NO;
  self.loadingIndicator.hidesWhenStopped = YES;

  [self.inputContainer addSubview:self.messageInput];
  [self.inputContainer addSubview:self.sendButton];
  [self.inputContainer addSubview:self.loadingIndicator];
  [self.mainStack addArrangedSubview:self.inputContainer];

  [NSLayoutConstraint activateConstraints:@[
    [self.inputContainer.heightAnchor constraintEqualToConstant:70],
    [self.messageInput.leadingAnchor
        constraintEqualToAnchor:self.inputContainer.leadingAnchor
                       constant:16],
    [self.messageInput.centerYAnchor
        constraintEqualToAnchor:self.inputContainer.centerYAnchor],
    [self.messageInput.heightAnchor constraintEqualToConstant:40],
    [self.sendButton.leadingAnchor
        constraintEqualToAnchor:self.messageInput.trailingAnchor
                       constant:12],
    [self.sendButton.trailingAnchor
        constraintEqualToAnchor:self.inputContainer.trailingAnchor
                       constant:-16],
    [self.sendButton.centerYAnchor
        constraintEqualToAnchor:self.inputContainer.centerYAnchor],
    [self.sendButton.widthAnchor constraintEqualToConstant:40],
    [self.sendButton.heightAnchor constraintEqualToConstant:40],
    [self.loadingIndicator.centerXAnchor
        constraintEqualToAnchor:self.sendButton.centerXAnchor],
    [self.loadingIndicator.centerYAnchor
        constraintEqualToAnchor:self.sendButton.centerYAnchor],
  ]];
}

- (void)setupApiKeySetupView {
  self.apiKeySetupView = [[UIView alloc] init];
  self.apiKeySetupView.backgroundColor = self.view.backgroundColor;
  self.apiKeySetupView.translatesAutoresizingMaskIntoConstraints = NO;
  self.apiKeySetupView.hidden = YES;
  [self.view addSubview:self.apiKeySetupView];

  UILabel* emojiLabel = [[UILabel alloc] init];
  emojiLabel.text = @"🔑";
  emojiLabel.font = [UIFont systemFontOfSize:60];
  emojiLabel.textAlignment = NSTextAlignmentCenter;
  emojiLabel.translatesAutoresizingMaskIntoConstraints = NO;

  UILabel* titleLabel = [[UILabel alloc] init];
  titleLabel.text = @"Set Up Your API Key";
  titleLabel.textColor = [UIColor whiteColor];
  titleLabel.font = [UIFont boldSystemFontOfSize:24];
  titleLabel.textAlignment = NSTextAlignmentCenter;
  titleLabel.translatesAutoresizingMaskIntoConstraints = NO;

  UILabel* descriptionLabel = [[UILabel alloc] init];
  descriptionLabel.text = @"To use Gemini AI Chat, you need a Google API key "
                          @"with Gemini access.";
  descriptionLabel.textColor = [UIColor lightGrayColor];
  descriptionLabel.font = [UIFont systemFontOfSize:14];
  descriptionLabel.textAlignment = NSTextAlignmentCenter;
  descriptionLabel.numberOfLines = 0;
  descriptionLabel.translatesAutoresizingMaskIntoConstraints = NO;

  self.apiKeyInput = [[UITextField alloc] init];
  self.apiKeyInput.placeholder = @"Enter your Google API key...";
  self.apiKeyInput.textColor = [UIColor whiteColor];
  self.apiKeyInput.backgroundColor = [UIColor colorWithRed:0.118
                                                     green:0.118
                                                      blue:0.188
                                                     alpha:1.0];
  self.apiKeyInput.layer.cornerRadius = 8;
  self.apiKeyInput.leftView =
      [[UIView alloc] initWithFrame:CGRectMake(0, 0, 12, 0)];
  self.apiKeyInput.leftViewMode = UITextFieldViewModeAlways;
  self.apiKeyInput.secureTextEntry = YES;
  self.apiKeyInput.translatesAutoresizingMaskIntoConstraints = NO;

  UIButton* saveButton = [UIButton buttonWithType:UIButtonTypeSystem];
  [saveButton setTitle:@"Save Key" forState:UIControlStateNormal];
  [saveButton setTitleColor:[UIColor whiteColor]
                   forState:UIControlStateNormal];
  saveButton.backgroundColor = [UIColor systemBlueColor];
  saveButton.layer.cornerRadius = 8;
  saveButton.translatesAutoresizingMaskIntoConstraints = NO;
  [saveButton addTarget:self
                 action:@selector(saveApiKeyTapped)
       forControlEvents:UIControlEventTouchUpInside];

  [self.apiKeySetupView addSubview:emojiLabel];
  [self.apiKeySetupView addSubview:titleLabel];
  [self.apiKeySetupView addSubview:descriptionLabel];
  [self.apiKeySetupView addSubview:self.apiKeyInput];
  [self.apiKeySetupView addSubview:saveButton];

  [NSLayoutConstraint activateConstraints:@[
    [self.apiKeySetupView.topAnchor constraintEqualToAnchor:self.view.topAnchor],
    [self.apiKeySetupView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.apiKeySetupView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    [self.apiKeySetupView.bottomAnchor
        constraintEqualToAnchor:self.view.bottomAnchor],

    [emojiLabel.centerXAnchor
        constraintEqualToAnchor:self.apiKeySetupView.centerXAnchor],
    [emojiLabel.bottomAnchor constraintEqualToAnchor:titleLabel.topAnchor
                                            constant:-16],

    [titleLabel.centerXAnchor
        constraintEqualToAnchor:self.apiKeySetupView.centerXAnchor],
    [titleLabel.centerYAnchor
        constraintEqualToAnchor:self.apiKeySetupView.centerYAnchor
                       constant:-80],

    [descriptionLabel.topAnchor constraintEqualToAnchor:titleLabel.bottomAnchor
                                               constant:16],
    [descriptionLabel.leadingAnchor
        constraintEqualToAnchor:self.apiKeySetupView.leadingAnchor
                       constant:32],
    [descriptionLabel.trailingAnchor
        constraintEqualToAnchor:self.apiKeySetupView.trailingAnchor
                       constant:-32],

    [self.apiKeyInput.topAnchor
        constraintEqualToAnchor:descriptionLabel.bottomAnchor
                       constant:24],
    [self.apiKeyInput.leadingAnchor
        constraintEqualToAnchor:self.apiKeySetupView.leadingAnchor
                       constant:32],
    [self.apiKeyInput.trailingAnchor
        constraintEqualToAnchor:self.apiKeySetupView.trailingAnchor
                       constant:-32],
    [self.apiKeyInput.heightAnchor constraintEqualToConstant:44],

    [saveButton.topAnchor constraintEqualToAnchor:self.apiKeyInput.bottomAnchor
                                         constant:16],
    [saveButton.leadingAnchor
        constraintEqualToAnchor:self.apiKeySetupView.leadingAnchor
                       constant:32],
    [saveButton.trailingAnchor
        constraintEqualToAnchor:self.apiKeySetupView.trailingAnchor
                       constant:-32],
    [saveButton.heightAnchor constraintEqualToConstant:44],
  ]];
}

- (void)setDefaultModels {
  self.models = @[
    @{
      @"id" : @"gemini-2.0-flash",
      @"displayName" : @"Gemini 2.0 Flash"
    },
    @{
      @"id" : @"gemini-2.0-flash-thinking",
      @"displayName" : @"Gemini 2.0 Flash Thinking"
    },
    @{@"id" : @"gemini-1.5-pro", @"displayName" : @"Gemini 1.5 Pro"},
    @{@"id" : @"gemini-1.5-flash", @"displayName" : @"Gemini 1.5 Flash"},
    @{@"id" : @"gemini-1.5-flash-8b", @"displayName" : @"Gemini 1.5 Flash-8B"},
  ];
  self.selectedModelId = @"gemini-2.0-flash";
}

- (void)addWelcomeMessage {
  UIView* welcomeView = [[UIView alloc] init];
  welcomeView.translatesAutoresizingMaskIntoConstraints = NO;

  UILabel* iconLabel = [[UILabel alloc] init];
  iconLabel.text = @"✨";
  iconLabel.font = [UIFont systemFontOfSize:48];
  iconLabel.textAlignment = NSTextAlignmentCenter;
  iconLabel.translatesAutoresizingMaskIntoConstraints = NO;

  UILabel* titleLabel = [[UILabel alloc] init];
  titleLabel.text = @"Welcome to Gemini AI Chat";
  titleLabel.textColor = [UIColor whiteColor];
  titleLabel.font = [UIFont boldSystemFontOfSize:20];
  titleLabel.textAlignment = NSTextAlignmentCenter;
  titleLabel.translatesAutoresizingMaskIntoConstraints = NO;

  UILabel* subtitleLabel = [[UILabel alloc] init];
  subtitleLabel.text =
      @"Start a conversation with one of Google's most capable AI models.";
  subtitleLabel.textColor = [UIColor lightGrayColor];
  subtitleLabel.font = [UIFont systemFontOfSize:14];
  subtitleLabel.textAlignment = NSTextAlignmentCenter;
  subtitleLabel.numberOfLines = 0;
  subtitleLabel.translatesAutoresizingMaskIntoConstraints = NO;

  [welcomeView addSubview:iconLabel];
  [welcomeView addSubview:titleLabel];
  [welcomeView addSubview:subtitleLabel];
  [self.messagesStack addArrangedSubview:welcomeView];

  [NSLayoutConstraint activateConstraints:@[
    [welcomeView.heightAnchor constraintGreaterThanOrEqualToConstant:150],
    [iconLabel.topAnchor constraintEqualToAnchor:welcomeView.topAnchor
                                        constant:24],
    [iconLabel.centerXAnchor
        constraintEqualToAnchor:welcomeView.centerXAnchor],
    [titleLabel.topAnchor constraintEqualToAnchor:iconLabel.bottomAnchor
                                         constant:16],
    [titleLabel.centerXAnchor
        constraintEqualToAnchor:welcomeView.centerXAnchor],
    [subtitleLabel.topAnchor constraintEqualToAnchor:titleLabel.bottomAnchor
                                            constant:8],
    [subtitleLabel.leadingAnchor
        constraintEqualToAnchor:welcomeView.leadingAnchor],
    [subtitleLabel.trailingAnchor
        constraintEqualToAnchor:welcomeView.trailingAnchor],
    [subtitleLabel.bottomAnchor constraintEqualToAnchor:welcomeView.bottomAnchor
                                               constant:-24],
  ]];
}

#pragma mark - Public Methods

- (void)setAvailableModels:(NSArray<NSDictionary*>*)models {
  self.models = models;
  if (models.count > 0) {
    [self.modelSelector setTitle:models[0][@"displayName"]
                        forState:UIControlStateNormal];
    self.selectedModelId = models[0][@"id"];
  }
}

- (void)addMessageWithRole:(NSString*)role content:(NSString*)content {
  BOOL isUser = [role isEqualToString:@"user"];

  UIView* messageContainer = [[UIView alloc] init];
  messageContainer.translatesAutoresizingMaskIntoConstraints = NO;

  UILabel* avatarLabel = [[UILabel alloc] init];
  avatarLabel.text = isUser ? @"👤" : @"✨";
  avatarLabel.font = [UIFont systemFontOfSize:16];
  avatarLabel.textAlignment = NSTextAlignmentCenter;
  avatarLabel.backgroundColor = isUser ? [UIColor systemBlueColor]
                                       : [UIColor colorWithRed:0.059
                                                         green:0.204
                                                          blue:0.376
                                                         alpha:1.0];
  avatarLabel.layer.cornerRadius = 16;
  avatarLabel.clipsToBounds = YES;
  avatarLabel.translatesAutoresizingMaskIntoConstraints = NO;

  // Create message bubble with padding
  UIColor* bubbleColor = isUser ? [UIColor systemBlueColor]
                                : [UIColor colorWithRed:0.176
                                                  green:0.216
                                                   blue:0.282
                                                  alpha:1.0];

  UIView* paddingView = [[UIView alloc] init];
  paddingView.backgroundColor = bubbleColor;
  paddingView.layer.cornerRadius = 12;
  paddingView.translatesAutoresizingMaskIntoConstraints = NO;

  UILabel* paddedLabel = [[UILabel alloc] init];
  paddedLabel.text = content;
  paddedLabel.textColor = [UIColor whiteColor];
  paddedLabel.font = [UIFont systemFontOfSize:15];
  paddedLabel.numberOfLines = 0;
  paddedLabel.translatesAutoresizingMaskIntoConstraints = NO;

  [paddingView addSubview:paddedLabel];

  if (isUser) {
    [messageContainer addSubview:paddingView];
    [messageContainer addSubview:avatarLabel];
  } else {
    [messageContainer addSubview:avatarLabel];
    [messageContainer addSubview:paddingView];
  }

  [self.messagesStack addArrangedSubview:messageContainer];

  if (isUser) {
    [NSLayoutConstraint activateConstraints:@[
      [avatarLabel.widthAnchor constraintEqualToConstant:32],
      [avatarLabel.heightAnchor constraintEqualToConstant:32],
      [avatarLabel.trailingAnchor
          constraintEqualToAnchor:messageContainer.trailingAnchor],
      [avatarLabel.topAnchor constraintEqualToAnchor:messageContainer.topAnchor],
      [paddingView.trailingAnchor
          constraintEqualToAnchor:avatarLabel.leadingAnchor
                         constant:-8],
      [paddingView.topAnchor
          constraintEqualToAnchor:messageContainer.topAnchor],
      [paddingView.bottomAnchor
          constraintEqualToAnchor:messageContainer.bottomAnchor],
      [paddingView.leadingAnchor
          constraintGreaterThanOrEqualToAnchor:messageContainer.leadingAnchor
                                      constant:48],
      [paddedLabel.topAnchor constraintEqualToAnchor:paddingView.topAnchor
                                            constant:12],
      [paddedLabel.leadingAnchor
          constraintEqualToAnchor:paddingView.leadingAnchor
                         constant:16],
      [paddedLabel.trailingAnchor
          constraintEqualToAnchor:paddingView.trailingAnchor
                         constant:-16],
      [paddedLabel.bottomAnchor constraintEqualToAnchor:paddingView.bottomAnchor
                                               constant:-12],
    ]];
  } else {
    [NSLayoutConstraint activateConstraints:@[
      [avatarLabel.widthAnchor constraintEqualToConstant:32],
      [avatarLabel.heightAnchor constraintEqualToConstant:32],
      [avatarLabel.leadingAnchor
          constraintEqualToAnchor:messageContainer.leadingAnchor],
      [avatarLabel.topAnchor constraintEqualToAnchor:messageContainer.topAnchor],
      [paddingView.leadingAnchor
          constraintEqualToAnchor:avatarLabel.trailingAnchor
                         constant:8],
      [paddingView.topAnchor
          constraintEqualToAnchor:messageContainer.topAnchor],
      [paddingView.bottomAnchor
          constraintEqualToAnchor:messageContainer.bottomAnchor],
      [paddingView.trailingAnchor
          constraintLessThanOrEqualToAnchor:messageContainer.trailingAnchor
                                   constant:-48],
      [paddedLabel.topAnchor constraintEqualToAnchor:paddingView.topAnchor
                                            constant:12],
      [paddedLabel.leadingAnchor
          constraintEqualToAnchor:paddingView.leadingAnchor
                         constant:16],
      [paddedLabel.trailingAnchor
          constraintEqualToAnchor:paddingView.trailingAnchor
                         constant:-16],
      [paddedLabel.bottomAnchor constraintEqualToAnchor:paddingView.bottomAnchor
                                               constant:-12],
    ]];
  }

  // Scroll to bottom
  dispatch_async(dispatch_get_main_queue(), ^{
    CGPoint bottomOffset = CGPointMake(
        0, self.scrollView.contentSize.height - self.scrollView.bounds.size.height);
    if (bottomOffset.y > 0) {
      [self.scrollView setContentOffset:bottomOffset animated:YES];
    }
  });
}

- (void)setLoading:(BOOL)loading {
  if (loading) {
    [self.loadingIndicator startAnimating];
    self.sendButton.hidden = YES;
  } else {
    [self.loadingIndicator stopAnimating];
    self.sendButton.hidden = NO;
  }
  self.messageInput.enabled = !loading;
}

- (void)showApiKeySetup {
  self.apiKeySetupView.hidden = NO;
  self.mainStack.hidden = YES;
}

- (void)showChatInterface {
  self.apiKeySetupView.hidden = YES;
  self.mainStack.hidden = NO;
}

- (void)showError:(NSString*)error {
  UIAlertController* alert =
      [UIAlertController alertControllerWithTitle:@"Error"
                                          message:error
                                   preferredStyle:UIAlertControllerStyleAlert];
  [alert addAction:[UIAlertAction actionWithTitle:@"OK"
                                            style:UIAlertActionStyleDefault
                                          handler:nil]];
  [self presentViewController:alert animated:YES completion:nil];
}

#pragma mark - Actions

- (void)showModelPicker {
  UIAlertController* alert = [UIAlertController
      alertControllerWithTitle:@"Select Model"
                       message:nil
                preferredStyle:UIAlertControllerStyleActionSheet];

  for (NSDictionary* model in self.models) {
    UIAlertAction* action = [UIAlertAction
        actionWithTitle:model[@"displayName"]
                  style:UIAlertActionStyleDefault
                handler:^(UIAlertAction* action) {
                  self.selectedModelId = model[@"id"];
                  [self.modelSelector setTitle:model[@"displayName"]
                                      forState:UIControlStateNormal];
                  [self.delegate geminiChatViewController:self
                                             selectModel:self.selectedModelId];
                }];
    [alert addAction:action];
  }

  [alert addAction:[UIAlertAction actionWithTitle:@"Cancel"
                                            style:UIAlertActionStyleCancel
                                          handler:nil]];

  [self presentViewController:alert animated:YES completion:nil];
}

- (void)sendMessage {
  NSString* message = self.messageInput.text;
  if (message.length == 0) {
    return;
  }

  self.messageInput.text = @"";
  self.sendButton.enabled = NO;

  [self.delegate geminiChatViewController:self sendMessage:message];
}

- (void)newChatTapped {
  // Clear messages
  for (UIView* view in self.messagesStack.arrangedSubviews) {
    [view removeFromSuperview];
  }
  [self addWelcomeMessage];

  [self.delegate geminiChatViewControllerDidTapNewChat:self];
}

- (void)saveApiKeyTapped {
  NSString* apiKey = self.apiKeyInput.text;
  if (apiKey.length == 0) {
    [self showError:@"Please enter an API key"];
    return;
  }

  [self.delegate geminiChatViewController:self saveApiKey:apiKey];
}

- (void)textFieldDidChange:(UITextField*)textField {
  self.sendButton.enabled = textField.text.length > 0;
}

#pragma mark - UITextFieldDelegate

- (BOOL)textFieldShouldReturn:(UITextField*)textField {
  if (textField == self.messageInput) {
    [self sendMessage];
    return NO;
  }
  return YES;
}

@end
