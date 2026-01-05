// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Gemini AI Chat JavaScript implementation
 */

class GeminiChat {
  constructor() {
    this.models = [];
    this.currentModel = null;
    this.conversations = [];
    this.currentConversation = null;
    this.messageHistory = [];
    this.isLoading = false;
    this.hasApiKey = false;
    this.generationConfig = {
      temperature: 1.0,
      topK: 40,
      topP: 0.95,
      maxOutputTokens: 8192,
    };

    this.init();
  }

  async init() {
    this.bindElements();
    this.bindEvents();
    await this.checkApiKey();
    await this.loadModels();
  }

  bindElements() {
    // Header elements
    this.modelSelect = document.getElementById('model-select');
    this.newChatBtn = document.getElementById('new-chat-btn');
    this.settingsBtn = document.getElementById('settings-btn');

    // API Key setup
    this.apiKeySetup = document.getElementById('api-key-setup');
    this.apiKeyInput = document.getElementById('api-key-input');
    this.saveApiKeyBtn = document.getElementById('save-api-key-btn');

    // Chat elements
    this.chatContainer = document.getElementById('chat-container');
    this.messagesContainer = document.getElementById('messages');
    this.messageInput = document.getElementById('message-input');
    this.sendBtn = document.getElementById('send-btn');
    this.tokenInfo = document.getElementById('token-info');

    // Model info
    this.currentModelName = document.getElementById('current-model-name');
    this.currentModelDescription = document.getElementById('current-model-description');

    // Settings modal
    this.settingsModal = document.getElementById('settings-modal');
    this.closeSettingsBtn = document.getElementById('close-settings-btn');
    this.settingsApiKey = document.getElementById('settings-api-key');
    this.updateApiKeyBtn = document.getElementById('update-api-key-btn');
    this.temperatureSlider = document.getElementById('temperature-slider');
    this.tempValue = document.getElementById('temp-value');
    this.maxTokensInput = document.getElementById('max-tokens-input');

    // Sidebar
    this.conversationsList = document.getElementById('conversations-list');
  }

  bindEvents() {
    // Model selector
    this.modelSelect.addEventListener('change', (e) => this.onModelChange(e));

    // New chat button
    this.newChatBtn.addEventListener('click', () => this.startNewChat());

    // Settings
    this.settingsBtn.addEventListener('click', () => this.openSettings());
    this.closeSettingsBtn.addEventListener('click', () => this.closeSettings());
    this.settingsModal.addEventListener('click', (e) => {
      if (e.target === this.settingsModal) this.closeSettings();
    });

    // API Key
    this.saveApiKeyBtn.addEventListener('click', () => this.saveApiKey());
    this.updateApiKeyBtn.addEventListener('click', () => this.updateApiKey());
    this.apiKeyInput.addEventListener('keypress', (e) => {
      if (e.key === 'Enter') this.saveApiKey();
    });

    // Message input
    this.messageInput.addEventListener('input', () => this.autoResizeInput());
    this.messageInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' && !e.shiftKey) {
        e.preventDefault();
        this.sendMessage();
      }
    });
    this.sendBtn.addEventListener('click', () => this.sendMessage());

    // Settings sliders
    this.temperatureSlider.addEventListener('input', (e) => {
      this.generationConfig.temperature = parseFloat(e.target.value);
      this.tempValue.textContent = e.target.value;
    });

    this.maxTokensInput.addEventListener('change', (e) => {
      this.generationConfig.maxOutputTokens = parseInt(e.target.value, 10);
    });

    // Listen for streaming content updates
    if (typeof cr !== 'undefined' && cr.addWebUIListener) {
      cr.addWebUIListener('streaming-content', (data) => {
        this.handleStreamingContent(data);
      });
      cr.addWebUIListener('chat-error', (data) => {
        this.handleError(data);
      });
    }
  }

  async checkApiKey() {
    return new Promise((resolve) => {
      if (typeof chrome !== 'undefined' && chrome.send) {
        chrome.send('getApiKeyStatus', []);
        // Use sendWithPromise for proper async handling
        cr.sendWithPromise('getApiKeyStatus').then((response) => {
          this.hasApiKey = response.hasApiKey && response.isValid;
          this.updateUIForApiKey();
          resolve();
        });
      } else {
        // Demo mode for testing outside Chrome
        this.hasApiKey = false;
        this.updateUIForApiKey();
        resolve();
      }
    });
  }

  updateUIForApiKey() {
    if (this.hasApiKey) {
      this.apiKeySetup.style.display = 'none';
      this.chatContainer.style.display = 'block';
      this.messageInput.disabled = false;
      this.sendBtn.disabled = false;
    } else {
      this.apiKeySetup.style.display = 'flex';
      this.chatContainer.style.display = 'none';
      this.messageInput.disabled = true;
      this.sendBtn.disabled = true;
    }
  }

  async loadModels() {
    if (typeof cr !== 'undefined' && cr.sendWithPromise) {
      this.models = await cr.sendWithPromise('getModels');
    } else {
      // Fallback demo models
      this.models = [
        {
          id: 'gemini-2.0-flash',
          displayName: 'Gemini 2.0 Flash',
          description: 'Fastest and most efficient model for everyday tasks.',
          maxInputTokens: 1048576,
          maxOutputTokens: 8192,
          supportsMultimodal: true,
        },
        {
          id: 'gemini-2.0-flash-thinking',
          displayName: 'Gemini 2.0 Flash Thinking',
          description: 'Enhanced reasoning with step-by-step thinking.',
          maxInputTokens: 1048576,
          maxOutputTokens: 65536,
          supportsMultimodal: true,
        },
        {
          id: 'gemini-1.5-pro',
          displayName: 'Gemini 1.5 Pro',
          description: 'Most capable model for complex reasoning and analysis.',
          maxInputTokens: 2097152,
          maxOutputTokens: 8192,
          supportsMultimodal: true,
        },
        {
          id: 'gemini-1.5-flash',
          displayName: 'Gemini 1.5 Flash',
          description: 'Fast and versatile for most use cases.',
          maxInputTokens: 1048576,
          maxOutputTokens: 8192,
          supportsMultimodal: true,
        },
        {
          id: 'gemini-1.5-flash-8b',
          displayName: 'Gemini 1.5 Flash-8B',
          description: 'Compact and efficient for high-volume tasks.',
          maxInputTokens: 1048576,
          maxOutputTokens: 8192,
          supportsMultimodal: true,
        },
      ];
    }

    this.populateModelSelector();

    if (this.models.length > 0) {
      this.currentModel = this.models[0];
      this.updateModelInfo();
    }
  }

  populateModelSelector() {
    this.modelSelect.innerHTML = '';
    this.models.forEach((model) => {
      const option = document.createElement('option');
      option.value = model.id;
      option.textContent = model.displayName;
      this.modelSelect.appendChild(option);
    });
  }

  onModelChange(event) {
    const modelId = event.target.value;
    this.currentModel = this.models.find((m) => m.id === modelId);
    this.updateModelInfo();
  }

  updateModelInfo() {
    if (this.currentModel) {
      this.currentModelName.textContent = this.currentModel.displayName;
      this.currentModelDescription.textContent = this.currentModel.description;
    }
  }

  async saveApiKey() {
    const apiKey = this.apiKeyInput.value.trim();
    if (!apiKey) {
      alert('Please enter an API key');
      return;
    }

    if (typeof cr !== 'undefined' && cr.sendWithPromise) {
      const response = await cr.sendWithPromise('setApiKey', apiKey);
      if (response.success) {
        this.hasApiKey = true;
        this.updateUIForApiKey();
      } else {
        alert('Invalid API key format');
      }
    } else {
      // Demo mode
      this.hasApiKey = true;
      this.updateUIForApiKey();
    }
  }

  async updateApiKey() {
    const apiKey = this.settingsApiKey.value.trim();
    if (apiKey) {
      if (typeof cr !== 'undefined' && cr.sendWithPromise) {
        await cr.sendWithPromise('setApiKey', apiKey);
      }
      this.closeSettings();
    }
  }

  openSettings() {
    this.settingsModal.style.display = 'flex';
  }

  closeSettings() {
    this.settingsModal.style.display = 'none';
  }

  startNewChat() {
    this.messageHistory = [];
    this.currentConversation = {
      id: this.generateId(),
      title: 'New Chat',
      modelId: this.currentModel?.id || 'gemini-2.0-flash',
      messages: [],
      createdAt: Date.now(),
      updatedAt: Date.now(),
    };

    // Clear messages and show welcome
    this.messagesContainer.innerHTML = `
      <div class="welcome-message">
        <div class="welcome-icon">✨</div>
        <h2>Welcome to Gemini AI Chat</h2>
        <p>Start a conversation with one of Google's most capable AI models.</p>
        <div class="model-info">
          <strong>Current Model:</strong> <span id="current-model-name">${this.currentModel?.displayName || 'Gemini 2.0 Flash'}</span>
          <p id="current-model-description">${this.currentModel?.description || 'Fast and efficient model.'}</p>
        </div>
      </div>
    `;
  }

  autoResizeInput() {
    this.messageInput.style.height = 'auto';
    this.messageInput.style.height = Math.min(this.messageInput.scrollHeight, 200) + 'px';
  }

  async sendMessage() {
    const message = this.messageInput.value.trim();
    if (!message || this.isLoading) return;

    // Clear welcome message if this is the first message
    const welcomeMessage = this.messagesContainer.querySelector('.welcome-message');
    if (welcomeMessage) {
      welcomeMessage.remove();
    }

    // Add user message to UI
    this.addMessageToUI('user', message);

    // Clear input
    this.messageInput.value = '';
    this.messageInput.style.height = 'auto';

    // Add to history
    const userMessage = {
      id: this.generateId(),
      role: 0, // kUser
      content: message,
      timestamp: Date.now(),
      isStreaming: false,
    };
    this.messageHistory.push(userMessage);

    // Show loading indicator
    this.isLoading = true;
    this.sendBtn.disabled = true;
    const loadingMessage = this.addLoadingMessage();

    try {
      let response;
      if (typeof cr !== 'undefined' && cr.sendWithPromise) {
        response = await cr.sendWithPromise(
          'sendMessage',
          this.currentModel?.id || 'gemini-2.0-flash',
          message,
          this.messageHistory.slice(0, -1), // Exclude the message we just sent
          this.generationConfig
        );
      } else {
        // Demo response for testing
        await new Promise((resolve) => setTimeout(resolve, 1000));
        response = {
          success: true,
          content: 'Hello! I\'m Gemini, a large language model created by Google. ' +
            'This is a demo response. To use the real Gemini API, please configure your API key.\n\n' +
            'Here\'s a sample code block:\n```javascript\nconst greeting = "Hello, World!";\nconsole.log(greeting);\n```\n\n' +
            'How can I help you today?',
          promptTokens: 10,
          completionTokens: 50,
          totalTokens: 60,
        };
      }

      // Remove loading indicator
      loadingMessage.remove();

      if (response.success) {
        // Add model response to UI
        this.addMessageToUI('model', response.content);

        // Add to history
        this.messageHistory.push({
          id: this.generateId(),
          role: 1, // kModel
          content: response.content,
          timestamp: Date.now(),
          isStreaming: false,
        });

        // Update token info
        if (response.totalTokens) {
          this.tokenInfo.textContent = `Tokens: ${response.totalTokens} (prompt: ${response.promptTokens}, completion: ${response.completionTokens})`;
        }

        // Update conversation title based on first message
        if (this.messageHistory.length === 2 && this.currentConversation) {
          this.currentConversation.title = message.slice(0, 50) + (message.length > 50 ? '...' : '');
        }
      } else {
        this.addMessageToUI('model', `Error: ${response.error}`);
      }
    } catch (error) {
      loadingMessage.remove();
      this.addMessageToUI('model', `Error: ${error.message || 'Failed to get response'}`);
    } finally {
      this.isLoading = false;
      this.sendBtn.disabled = false;
      this.messageInput.focus();
    }
  }

  addMessageToUI(role, content) {
    const messageDiv = document.createElement('div');
    messageDiv.className = `message ${role}`;

    const avatar = document.createElement('div');
    avatar.className = 'message-avatar';
    avatar.textContent = role === 'user' ? '👤' : '✨';

    const contentDiv = document.createElement('div');
    contentDiv.className = 'message-content';
    contentDiv.innerHTML = this.formatMessage(content);

    if (role === 'model') {
      messageDiv.appendChild(avatar);
      messageDiv.appendChild(contentDiv);
    } else {
      messageDiv.appendChild(contentDiv);
      messageDiv.appendChild(avatar);
    }

    this.messagesContainer.appendChild(messageDiv);
    this.scrollToBottom();

    return messageDiv;
  }

  addLoadingMessage() {
    const messageDiv = document.createElement('div');
    messageDiv.className = 'message model';
    messageDiv.innerHTML = `
      <div class="message-avatar">✨</div>
      <div class="message-content">
        <div class="typing-indicator">
          <div class="typing-dot"></div>
          <div class="typing-dot"></div>
          <div class="typing-dot"></div>
        </div>
      </div>
    `;
    this.messagesContainer.appendChild(messageDiv);
    this.scrollToBottom();
    return messageDiv;
  }

  formatMessage(content) {
    // Escape HTML
    let formatted = content
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;');

    // Format code blocks
    formatted = formatted.replace(
      /```(\w*)\n([\s\S]*?)```/g,
      '<pre><code class="language-$1">$2</code></pre>'
    );

    // Format inline code
    formatted = formatted.replace(/`([^`]+)`/g, '<code>$1</code>');

    // Format bold
    formatted = formatted.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');

    // Format italic
    formatted = formatted.replace(/\*([^*]+)\*/g, '<em>$1</em>');

    // Format newlines
    formatted = formatted.replace(/\n/g, '<br>');

    return formatted;
  }

  scrollToBottom() {
    this.chatContainer.scrollTop = this.chatContainer.scrollHeight;
  }

  handleStreamingContent(data) {
    // Handle streaming updates (for future implementation)
    console.log('Streaming content:', data);
  }

  handleError(data) {
    console.error('Chat error:', data);
    this.addMessageToUI('model', `Error: ${data.error}`);
  }

  generateId() {
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, (c) => {
      const r = (Math.random() * 16) | 0;
      const v = c === 'x' ? r : (r & 0x3) | 0x8;
      return v.toString(16);
    });
  }
}

// Initialize the chat when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
  window.geminiChat = new GeminiChat();
});
