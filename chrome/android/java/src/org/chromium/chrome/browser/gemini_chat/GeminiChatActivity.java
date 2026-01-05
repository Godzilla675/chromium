// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.gemini_chat;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.Log;

import java.util.ArrayList;
import java.util.List;

/**
 * Activity for the Gemini AI Chat feature on Android.
 * Provides a chat interface for interacting with Google's Gemini AI models.
 */
public class GeminiChatActivity extends AppCompatActivity {
    private static final String TAG = "GeminiChat";

    // Views
    private Spinner mModelSpinner;
    private LinearLayout mMessagesContainer;
    private ScrollView mScrollView;
    private EditText mMessageInput;
    private Button mSendButton;
    private ProgressBar mProgressBar;
    private View mApiKeySetupView;
    private EditText mApiKeyInput;
    private Button mSaveApiKeyButton;

    // State
    private GeminiChatClient mChatClient;
    private List<ChatMessage> mMessageHistory = new ArrayList<>();
    private String mCurrentModelId = "gemini-2.0-flash";
    private boolean mIsLoading = false;

    // Available models
    private static final String[][] MODELS = {
        {"gemini-2.0-flash", "Gemini 2.0 Flash"},
        {"gemini-2.0-flash-thinking", "Gemini 2.0 Flash Thinking"},
        {"gemini-1.5-pro", "Gemini 1.5 Pro"},
        {"gemini-1.5-flash", "Gemini 1.5 Flash"},
        {"gemini-1.5-flash-8b", "Gemini 1.5 Flash-8B"},
    };

    /**
     * Launches the Gemini Chat activity.
     */
    public static void launch(Context context) {
        Intent intent = new Intent(context, GeminiChatActivity.class);
        if (!(context instanceof Activity)) {
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        }
        context.startActivity(intent);
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_gemini_chat);

        initializeViews();
        setupModelSpinner();
        setupListeners();
        initializeChatClient();
    }

    private void initializeViews() {
        mModelSpinner = findViewById(R.id.model_spinner);
        mMessagesContainer = findViewById(R.id.messages_container);
        mScrollView = findViewById(R.id.scroll_view);
        mMessageInput = findViewById(R.id.message_input);
        mSendButton = findViewById(R.id.send_button);
        mProgressBar = findViewById(R.id.progress_bar);
        mApiKeySetupView = findViewById(R.id.api_key_setup);
        mApiKeyInput = findViewById(R.id.api_key_input);
        mSaveApiKeyButton = findViewById(R.id.save_api_key_button);

        // Set up action bar
        if (getSupportActionBar() != null) {
            getSupportActionBar().setTitle("Gemini AI Chat");
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        }
    }

    private void setupModelSpinner() {
        List<String> modelNames = new ArrayList<>();
        for (String[] model : MODELS) {
            modelNames.add(model[1]);
        }

        ArrayAdapter<String> adapter = new ArrayAdapter<>(
            this, android.R.layout.simple_spinner_item, modelNames);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mModelSpinner.setAdapter(adapter);

        mModelSpinner.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(android.widget.AdapterView<?> parent, View view, int position, long id) {
                mCurrentModelId = MODELS[position][0];
                Log.d(TAG, "Selected model: " + mCurrentModelId);
            }

            @Override
            public void onNothingSelected(android.widget.AdapterView<?> parent) {
                // Keep default
            }
        });
    }

    private void setupListeners() {
        mSendButton.setOnClickListener(v -> sendMessage());

        mMessageInput.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                mSendButton.setEnabled(!mIsLoading && s.length() > 0);
            }

            @Override
            public void afterTextChanged(Editable s) {}
        });

        mMessageInput.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_SEND) {
                sendMessage();
                return true;
            }
            return false;
        });

        mSaveApiKeyButton.setOnClickListener(v -> saveApiKey());
    }

    private void initializeChatClient() {
        mChatClient = new GeminiChatClient(this);

        if (mChatClient.hasValidApiKey()) {
            showChatInterface();
        } else {
            showApiKeySetup();
        }
    }

    private void showApiKeySetup() {
        mApiKeySetupView.setVisibility(View.VISIBLE);
        mScrollView.setVisibility(View.GONE);
        findViewById(R.id.input_container).setVisibility(View.GONE);
    }

    private void showChatInterface() {
        mApiKeySetupView.setVisibility(View.GONE);
        mScrollView.setVisibility(View.VISIBLE);
        findViewById(R.id.input_container).setVisibility(View.VISIBLE);
        mMessageInput.setEnabled(true);
    }

    private void saveApiKey() {
        String apiKey = mApiKeyInput.getText().toString().trim();
        if (apiKey.isEmpty()) {
            showError("Please enter an API key");
            return;
        }

        mChatClient.setApiKey(apiKey);
        showChatInterface();
        showWelcomeMessage();
    }

    private void showWelcomeMessage() {
        if (mMessagesContainer.getChildCount() == 0) {
            addMessageToUI(false, "Hello! I'm Gemini, Google's most capable AI model. " +
                "I can help you with a wide range of tasks including answering questions, " +
                "writing content, analyzing information, and coding assistance. " +
                "How can I help you today?");
        }
    }

    private void sendMessage() {
        String message = mMessageInput.getText().toString().trim();
        if (message.isEmpty() || mIsLoading) {
            return;
        }

        // Add user message to UI
        addMessageToUI(true, message);
        mMessageInput.setText("");

        // Add to history
        mMessageHistory.add(new ChatMessage(true, message));

        // Show loading state
        setLoading(true);

        // Send to API
        mChatClient.sendMessage(
            mCurrentModelId,
            message,
            mMessageHistory,
            new GeminiChatClient.Callback() {
                @Override
                public void onSuccess(String response) {
                    runOnUiThread(() -> {
                        setLoading(false);
                        mMessageHistory.add(new ChatMessage(false, response));
                        addMessageToUI(false, response);
                    });
                }

                @Override
                public void onError(String error) {
                    runOnUiThread(() -> {
                        setLoading(false);
                        addMessageToUI(false, "Error: " + error);
                    });
                }
            }
        );
    }

    private void addMessageToUI(boolean isUser, String content) {
        View messageView = getLayoutInflater().inflate(
            isUser ? R.layout.item_message_user : R.layout.item_message_model,
            mMessagesContainer, false);

        TextView textView = messageView.findViewById(R.id.message_text);
        textView.setText(content);

        mMessagesContainer.addView(messageView);

        // Scroll to bottom
        mScrollView.post(() -> mScrollView.fullScroll(View.FOCUS_DOWN));
    }

    private void setLoading(boolean loading) {
        mIsLoading = loading;
        mProgressBar.setVisibility(loading ? View.VISIBLE : View.GONE);
        mSendButton.setEnabled(!loading && mMessageInput.getText().length() > 0);
        mMessageInput.setEnabled(!loading);
    }

    private void showError(String message) {
        new AlertDialog.Builder(this)
            .setTitle("Error")
            .setMessage(message)
            .setPositiveButton("OK", null)
            .show();
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBackPressed();
        return true;
    }

    /**
     * Simple class to hold a chat message.
     */
    public static class ChatMessage {
        public final boolean isUser;
        public final String content;
        public final long timestamp;

        public ChatMessage(boolean isUser, String content) {
            this.isUser = isUser;
            this.content = content;
            this.timestamp = System.currentTimeMillis();
        }
    }
}
