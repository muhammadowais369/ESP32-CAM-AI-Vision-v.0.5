/*
  ESP32-CAM Fear Detection with OpenRouter API
  Modified to detect fear and activate GPIO2 when fear is detected
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <Base64.h>
#include "esp_camera.h"
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "SSID";
const char* password = "PASSWORD";

// ========== PUT YOUR OPENROUTER API KEY HERE ==========
const String openRouterApiKey = "API KEY";
// Get free API key from: https://openrouter.ai/keys
// ======================================================

// Pin definitions for ESP32-CAM AI-Thinker module
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define BUTTON_PIN 13
#define FEAR_LED_PIN 2  // External LED connected to GPIO2 for fear detection

// Fear detection state
bool fearDetected = false;

// Function to encode image to Base64
String encodeImageToBase64(const uint8_t* imageData, size_t imageSize) {
  return base64::encode(imageData, imageSize);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(FEAR_LED_PIN, OUTPUT);
  digitalWrite(FEAR_LED_PIN, LOW); // Start with fear LED off

  Serial.println("\n\n=== ESP32-CAM-AI-Vision-v.0.5 ===");
  Serial.println("MUHAMMAD OWAIS 369");
  Serial.println("Using OpenRouter Vision API for Fear Detection");
  Serial.println("==============================================\n");
  delay(3000);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("WiFi Connected!");
  delay(2000);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  Serial.println("Camera Initialized");
  delay(2000);

  Serial.println("Press button to capture and analyze for fear detection");
  Serial.println("Fear LED will activate ONLY when fear is detected");
}

void captureAndAnalyzeImage() {
  Serial.println("Capturing image for fear analysis...");

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  esp_camera_fb_return(fb);
  fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  Serial.println("Image captured");
  String base64Image = encodeImageToBase64(fb->buf, fb->len);

  beep();
  esp_camera_fb_return(fb);

  if (base64Image.isEmpty()) {
    Serial.println("Failed to encode the image!");
    return;
  }
  
  AnalyzeImageForFear(base64Image);
}

void AnalyzeImageForFear(const String& base64Image) {
  Serial.println("Analyzing image for fear detection...");
  Serial.println("Processing...");

  String result;
  bool fearFound = false;

  // Try different vision models
  String models[] = {
    "openai/gpt-4o-mini"
  };

  bool success = false;
  
  for (int i = 0; i < 4 && !success; i++) {
    Serial.println("Trying model: " + models[i]);
    success = sendFearDetectionRequest(models[i], base64Image, result, fearFound);
    
    if (!success) {
      Serial.println("Model " + models[i] + " failed, trying next...");
      delay(1000);
    }
  }

  if (success) {
    Serial.println("\n=== FEAR ANALYSIS RESULT ===");
    Serial.println(result);
    Serial.println("==============================\n");
    
    // Control the fear LED based on detection
    if (fearFound) {
      Serial.println("FEAR DETECTED! Activating fear LED...");
      digitalWrite(FEAR_LED_PIN, HIGH);
      fearDetected = true;
      
      // Blink LED 3 times to indicate fear detection
      for(int i = 0; i < 3; i++) {
        digitalWrite(FEAR_LED_PIN, HIGH);
        delay(500);
        digitalWrite(FEAR_LED_PIN, LOW);
        delay(200);
      }
      digitalWrite(FEAR_LED_PIN, HIGH); // Keep LED on
    } else {
      Serial.println("No fear detected. Environment is safe.");
      digitalWrite(FEAR_LED_PIN, LOW);
      fearDetected = false;
    }
    
    Serial.println("Press button to capture again");
  } else {
    Serial.println("All models failed. Last error: " + result);
    digitalWrite(FEAR_LED_PIN, LOW); // Turn off LED on error
    fearDetected = false;
  }
}

bool sendFearDetectionRequest(const String& model, const String& base64Image, String& result, bool& fearFound) {
  HTTPClient http;
  
  String url = "https://openrouter.ai/api/v1/chat/completions";
  http.begin(url);
  
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + openRouterApiKey);
  http.addHeader("HTTP-Referer", "https://github.com/muhammadowais369");
  http.addHeader("X-Title", "ESP32-CAM-AI-Vision-v.0.5");
  
  http.setTimeout(30000);

  DynamicJsonDocument doc(16384);
  doc["model"] = model;
  
  JsonArray messages = doc.createNestedArray("messages");
  JsonObject message = messages.createNestedObject();
  message["role"] = "user";
  
  JsonArray content = message.createNestedArray("content");
  
  JsonObject textContent = content.createNestedObject();
  textContent["type"] = "text";
  textContent["text"] = "Analyze this image in detail and determine if there is any element that could cause fear, danger, or threat. "
                       "Look for: scary objects, dangerous situations, threatening people or animals, dark or ominous settings, "
                       "anything that might cause fear or anxiety. "
                       "First, provide a general description of the image. "
                       "Then, at the very end of your response, add exactly one of these two lines: "
                       "'FEAR_DETECTED: YES' if you find anything fearful/dangerous, or "
                       "'FEAR_DETECTED: NO' if the image appears safe and non-threatening. "
                       "Be very specific about what you see that could cause fear.";
  
  JsonObject imageContent = content.createNestedObject();
  imageContent["type"] = "image_url";
  JsonObject imageUrl = imageContent.createNestedObject("image_url");
  imageUrl["url"] = "data:image/jpeg;base64," + base64Image;

  doc["max_tokens"] = 800;

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  Serial.print("Payload size: ");
  Serial.println(jsonPayload.length());

  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode == 200) {
    String response = http.getString();
    http.end();
    
    DynamicJsonDocument responseDoc(8192);
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (!error && responseDoc.containsKey("choices") && 
        responseDoc["choices"].size() > 0 &&
        responseDoc["choices"][0].containsKey("message") &&
        responseDoc["choices"][0]["message"].containsKey("content")) {
      
      result = responseDoc["choices"][0]["message"]["content"].as<String>();
      
      // Check for fear detection in the response
      fearFound = checkForFearDetection(result);
      return true;
    } else {
      result = "Failed to parse response";
      return false;
    }
  } else {
    result = http.getString();
    http.end();
    return false;
  }
}

bool checkForFearDetection(const String& response) {
  // Convert to lowercase for easier matching
  String lowerResponse = response;
  lowerResponse.toLowerCase();
  
  // Check for fear indicators in the response
  if (response.indexOf("FEAR_DETECTED: YES") != -1) {
    return true;
  }
  if (response.indexOf("FEAR_DETECTED: NO") != -1) {
    return false;
  }
  
  // Fallback: Check for fear-related keywords if the specific format isn't found
  String fearKeywords[] = {
    "fear", "scary", "danger", "threat", "dangerous", "afraid", 
    "terror", "horror", "frightening", "alarming", "ominous",
    "threatening", "unsafe", "hazard", "risk", "panic"
  };
  
  int fearCount = 0;
  for (const String& keyword : fearKeywords) {
    if (lowerResponse.indexOf(keyword) != -1) {
      fearCount++;
    }
  }
  
  // If multiple fear keywords found, consider it fear detected
  return fearCount >= 2;
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button pressed! Capturing image for fear analysis...");
    captureAndAnalyzeImage();
    delay(1000);
  }
}

void beep(){
  digitalWrite(FEAR_LED_PIN, HIGH);
  delay(100);
  digitalWrite(FEAR_LED_PIN, LOW);
  delay(50);
  digitalWrite(FEAR_LED_PIN, HIGH);
  delay(100);
  digitalWrite(FEAR_LED_PIN, LOW);
}
