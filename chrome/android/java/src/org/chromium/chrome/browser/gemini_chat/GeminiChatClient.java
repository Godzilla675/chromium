// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.gemini_chat;

import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

/**
 * Client for interacting with the Google Gemini AI API.
 */
public class GeminiChatClient {
    private static final String TAG = "GeminiChatClient";
    private static final String PREFS_NAME = "gemini_chat_prefs";
    private static final String PREF_API_KEY = "api_key";
    private static final String API_BASE_URL =
        "https://generativelanguage.googleapis.com/v1beta/models/";

    private final Context mContext;
    private final SharedPreferences mPrefs;
    private final Executor mExecutor;
    private String mApiKey;

    /**
     * Callback interface for API responses.
     */
    public interface Callback {
        void onSuccess(String response);
        void onError(String error);
    }

    public GeminiChatClient(Context context) {
        mContext = context.getApplicationContext();
        mPrefs = mContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        mExecutor = Executors.newSingleThreadExecutor();
        mApiKey = mPrefs.getString(PREF_API_KEY, "");
    }

    /**
     * Sets the API key for Gemini requests.
     */
    public void setApiKey(String apiKey) {
        mApiKey = apiKey;
        mPrefs.edit().putString(PREF_API_KEY, apiKey).apply();
    }

    /**
     * Returns whether a valid API key is set.
     */
    public boolean hasValidApiKey() {
        return mApiKey != null && mApiKey.length() >= 10;
    }

    /**
     * Sends a message to the Gemini API.
     */
    public void sendMessage(String modelId, String message,
                           List<GeminiChatActivity.ChatMessage> history,
                           Callback callback) {
        if (!hasValidApiKey()) {
            callback.onError("Invalid or missing API key");
            return;
        }

        mExecutor.execute(() -> {
            try {
                String response = sendRequest(modelId, message, history);
                callback.onSuccess(response);
            } catch (Exception e) {
                Log.e(TAG, "Error sending message: " + e.getMessage(), e);
                callback.onError(e.getMessage());
            }
        });
    }

    private String sendRequest(String modelId, String message,
                               List<GeminiChatActivity.ChatMessage> history)
            throws IOException, JSONException {
        String urlString = API_BASE_URL + modelId + ":generateContent?key=" + mApiKey;
        URL url = new URL(urlString);

        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setRequestMethod("POST");
        connection.setRequestProperty("Content-Type", "application/json");
        connection.setDoOutput(true);
        connection.setConnectTimeout(30000);
        connection.setReadTimeout(60000);

        // Build request body
        JSONObject requestBody = buildRequestBody(message, history);

        // Send request
        try (OutputStream os = connection.getOutputStream()) {
            byte[] input = requestBody.toString().getBytes(StandardCharsets.UTF_8);
            os.write(input, 0, input.length);
        }

        // Read response
        int responseCode = connection.getResponseCode();
        StringBuilder response = new StringBuilder();

        if (responseCode == HttpURLConnection.HTTP_OK) {
            try (BufferedReader br = new BufferedReader(
                    new InputStreamReader(connection.getInputStream(), StandardCharsets.UTF_8))) {
                String line;
                while ((line = br.readLine()) != null) {
                    response.append(line);
                }
            }

            return parseResponse(response.toString());
        } else {
            try (BufferedReader br = new BufferedReader(
                    new InputStreamReader(connection.getErrorStream(), StandardCharsets.UTF_8))) {
                String line;
                while ((line = br.readLine()) != null) {
                    response.append(line);
                }
            }

            String errorMessage = parseError(response.toString());
            throw new IOException(errorMessage);
        }
    }

    private JSONObject buildRequestBody(String message,
                                        List<GeminiChatActivity.ChatMessage> history)
            throws JSONException {
        JSONObject body = new JSONObject();

        // Build contents array
        JSONArray contents = new JSONArray();

        // Add history
        for (GeminiChatActivity.ChatMessage msg : history) {
            JSONObject content = new JSONObject();
            content.put("role", msg.isUser ? "user" : "model");

            JSONArray parts = new JSONArray();
            JSONObject textPart = new JSONObject();
            textPart.put("text", msg.content);
            parts.put(textPart);
            content.put("parts", parts);

            contents.put(content);
        }

        // Add new message
        JSONObject userContent = new JSONObject();
        userContent.put("role", "user");

        JSONArray userParts = new JSONArray();
        JSONObject userText = new JSONObject();
        userText.put("text", message);
        userParts.put(userText);
        userContent.put("parts", userParts);

        contents.put(userContent);
        body.put("contents", contents);

        // Add generation config
        JSONObject generationConfig = new JSONObject();
        generationConfig.put("temperature", 1.0);
        generationConfig.put("topK", 40);
        generationConfig.put("topP", 0.95);
        generationConfig.put("maxOutputTokens", 8192);
        body.put("generationConfig", generationConfig);

        // Add safety settings
        JSONArray safetySettings = new JSONArray();
        String[] categories = {
            "HARM_CATEGORY_HARASSMENT",
            "HARM_CATEGORY_HATE_SPEECH",
            "HARM_CATEGORY_SEXUALLY_EXPLICIT",
            "HARM_CATEGORY_DANGEROUS_CONTENT"
        };

        for (String category : categories) {
            JSONObject setting = new JSONObject();
            setting.put("category", category);
            setting.put("threshold", "BLOCK_ONLY_HIGH");
            safetySettings.put(setting);
        }
        body.put("safetySettings", safetySettings);

        return body;
    }

    private String parseResponse(String responseBody) throws JSONException {
        JSONObject json = new JSONObject(responseBody);

        if (json.has("error")) {
            JSONObject error = json.getJSONObject("error");
            throw new RuntimeException(error.optString("message", "Unknown error"));
        }

        JSONArray candidates = json.optJSONArray("candidates");
        if (candidates != null && candidates.length() > 0) {
            JSONObject candidate = candidates.getJSONObject(0);
            JSONObject content = candidate.optJSONObject("content");
            if (content != null) {
                JSONArray parts = content.optJSONArray("parts");
                if (parts != null) {
                    StringBuilder fullText = new StringBuilder();
                    for (int i = 0; i < parts.length(); i++) {
                        JSONObject part = parts.getJSONObject(i);
                        fullText.append(part.optString("text", ""));
                    }
                    return fullText.toString();
                }
            }
        }

        return "No response generated";
    }

    private String parseError(String errorBody) {
        try {
            JSONObject json = new JSONObject(errorBody);
            if (json.has("error")) {
                JSONObject error = json.getJSONObject("error");
                return error.optString("message", "Unknown error");
            }
        } catch (JSONException e) {
            Log.e(TAG, "Error parsing error response", e);
        }
        return "Request failed";
    }
}
