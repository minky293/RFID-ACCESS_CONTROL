#include "AskGemini.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#define ASK_GEMINI_DEBUG

// ------------------------------------------------------------
// JSON escape helper
// ------------------------------------------------------------
String jsonEscape(const String& input) {
    String out;
    out.reserve(input.length() * 1.2);

    for (unsigned int i = 0; i < input.length(); i++) {
        char c = input[i];
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\"': out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

// ------------------------------------------------------------
// JSON unescape helper
// ------------------------------------------------------------
String jsonUnescape(const String& in) {
    String out;
    out.reserve(in.length());

    for (int i = 0; i < in.length(); i++) {
        char c = in[i];

        if (c == '\\' && i + 1 < in.length()) {
            char next = in[i + 1];

            switch (next) {
                case 'n': out += '\n'; i++; continue;
                case 'r': out += '\r'; i++; continue;
                case 't': out += '\t'; i++; continue;
                case '\\': out += '\\'; i++; continue;
                case '"': out += '"'; i++; continue;
            }
        }

        out += c;
    }

    return out;
}

// ------------------------------------------------------------
// Extract all "text" fields from Gemini JSON response
// ------------------------------------------------------------
String extractAllTextFields(const String& json) {
    String out;
    int searchIndex = 0;

    while (true) {
        int keyIndex = json.indexOf("\"text\":", searchIndex);
        if (keyIndex < 0) break;

        int startQuote = json.indexOf('"', keyIndex + 7);
        if (startQuote < 0) break;

        int endQuote = json.indexOf('"', startQuote + 1);
        if (endQuote < 0) break;

        String raw = json.substring(startQuote + 1, endQuote);
        out += jsonUnescape(raw);
        out += " ";

        searchIndex = endQuote + 1;
    }

    out.trim();
    return out;
}

// ------------------------------------------------------------
// Main Gemini request function
// ------------------------------------------------------------
String askGemini(const String& userText,
                 const String& instruction,
                 float temperature)
{
#ifdef ASK_GEMINI_DEBUG
    Serial.println("AskGemini.cpp: ENTER askGemini()");
#endif

// Allow a brief reconnection window
int tries = 0;
while (WiFi.status() != WL_CONNECTED && tries < 10) {
    delay(100);
    tries++;
}
if (WiFi.status() != WL_CONNECTED) {
    errorHandler(ERR_UNKNOWN);
    return "";
}

#ifdef ASK_GEMINI_DEBUG
    Serial.print("Free heap before Gemini: ");
    Serial.println(ESP.getFreeHeap());
#endif

    // Fresh TLS client
    WiFiClientSecure client;
    client.setInsecure();

    // Gemini 2.0 Flash requires v1beta endpoint
    String path =
        "/v1beta/models/" + Gemini_Model +
        ":generateContent?key=" + Gemini_APIKey;

#ifdef ASK_GEMINI_DEBUG
    Serial.print("AskGemini.cpp: PATH = ");
    Serial.println(path);
#endif

    HTTPClient http;

#ifdef ASK_GEMINI_DEBUG
    Serial.println("AskGemini.cpp: starting http.begin(host, path)");
#endif

    if (!http.begin(client,
                    "generativelanguage.googleapis.com",
                    443,
                    path,
                    true))
    {
#ifdef ASK_GEMINI_DEBUG
        Serial.println("AskGemini.cpp: http.begin FAILED");
#endif
        errorHandler(ERR_UNKNOWN);
        return "";
    }

    http.setTimeout(20000);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Connection", "close");

    // Combine instruction + user text (Gemini 2.0 requirement)
    String prompt =
        jsonEscape(instruction) + "\n\n" +
        jsonEscape(userText);

    // Correct Gemini 2.0 Flash payload
    String payload = "{"
        "\"contents\":[{"
            "\"role\":\"user\","
            "\"parts\":[{"
                "\"text\":\"" + prompt + "\""
            "}]"
        "}],"
        "\"generationConfig\":{"
            "\"temperature\":" + String(temperature, 3) + ","
            "\"maxOutputTokens\":128"
        "}"
    "}";

#ifdef ASK_GEMINI_DEBUG
    Serial.println("AskGemini.cpp: sending POST...");
#endif

    int code = http.POST(payload);

#ifdef ASK_GEMINI_DEBUG
    Serial.print("AskGemini.cpp: HTTP POST code: ");
    Serial.println(code);
#endif

    if (code <= 0) {
        http.end();
        errorHandler(ERR_UNKNOWN);
        return "";
    }

    String response = http.getString();
    http.end();

#ifdef ASK_GEMINI_DEBUG
    Serial.println("AskGemini.cpp: RAW RESPONSE:");
    Serial.println(response);
#endif

    if (response.length() == 0) {
        errorHandler(ERR_UNKNOWN);
        return "";
    }

#ifdef ASK_GEMINI_DEBUG
    Serial.println("AskGemini.cpp: EXIT askGemini()");
#endif

    return extractAllTextFields(response);
}