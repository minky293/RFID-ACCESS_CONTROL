#ifndef ASK_GEMINI_H
#define ASK_GEMINI_H

#include <Arduino.h>
#include <WiFi.h>   // <-- REQUIRED for WiFi.status() and WL_CONNECTED

// ------------------------------------------------------------
// User‑supplied globals (defined in the sketch):
// ------------------------------------------------------------
extern String Gemini_APIKey;
extern String Gemini_Model;

// The sketch must define this:
extern void errorHandler(int code);

// ------------------------------------------------------------
// Error codes
// ------------------------------------------------------------
#define ERR_UNKNOWN  -1

// ------------------------------------------------------------
// Optional debug mode
// ------------------------------------------------------------
// #define ASK_GEMINI_DEBUG

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------
String askGemini(const String& userText,
                 const String& instruction,
                 float temperature);

// ------------------------------------------------------------
// Internal helpers
// ------------------------------------------------------------
String jsonEscape(const String& input);
String jsonUnescape(const String& in);
String extractAllTextFields(const String& json);

#endif