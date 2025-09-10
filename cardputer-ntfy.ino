/*

  Oboi-DLP Cardputer ntfy monitor


A simple ntfy SSE streamer for M5 Cardputer
Enter your WiFi details, and ntfy topic then press C to monitor.

  (c)Scot D Forshaw : Toridion Ltd
  scot.forshaw@toridion.com
  
  Features:
    - Menu: (A) Connect WiFi, (B) Set topic, (C) Start Monitoring
    - Save/load settings to /oboi.cfg on SD
    - Append alerts to /alerts.txt, keep last 20
    - Beep on new alert


*/

#include <SD.h>
#include <M5GFX.h>
#include <SPI.h>
#include <M5Cardputer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>

std::vector<String> alertBuffer;

#define CONFIG_FILE "/oboi.cfg"
#define ALERT_FILE  "/alerts.txt"
#define MAX_ALERTS  20

String wifiSSID = "WIFI-SSID-HERE";
String wifiPASS = "WIFI-PASS-HERE";
String topic = "NTFY-TOPIC-HERE";

String inputBuffer = "";
bool enteringSSID = false;
bool enteringPASS = false;
bool enteringTopic = false;

// ---------- SD init ----------
bool initSD() {
  SPI.begin(
    M5.getPin(m5::pin_name_t::sd_spi_sclk),
    M5.getPin(m5::pin_name_t::sd_spi_miso),
    M5.getPin(m5::pin_name_t::sd_spi_mosi),
    M5.getPin(m5::pin_name_t::sd_spi_ss)
  );
  int tries = 0;
  while (!SD.begin(M5.getPin(m5::pin_name_t::sd_spi_ss), SPI)) {
    delay(1);
    if (++tries > 2000) return false;
  }
  return true;
}


void beepSOS() {
    const int freq = 700;
    const int dot = 50;
    const int dash = dot*3;
    const int interSymbol = dot;
    const int interLetter = dot*3;
    //M5.Lcd.setBrightness(10);
    for (int i = 0; i < 3; i++) {
        M5Cardputer.Speaker.tone(freq, dot);
        delay(dot);
        delay(interSymbol);
    }
    delay(interLetter - interSymbol);

    //M5.Lcd.setRotation(1);
    //M5.Lcd.setBrightness(200);
    //M5.Lcd.drawPngFile(SD, "/ntfy.png");

    for (int i = 0; i < 3; i++) {
        M5Cardputer.Speaker.tone(freq, dash);
        delay(dash);
        delay(interSymbol);
    }
    delay(interLetter - interSymbol);

    for (int i = 0; i < 3; i++) {
        M5Cardputer.Speaker.tone(freq, dot);
        delay(dot);
        delay(interSymbol);
    }
    //M5.Lcd.setBrightness(10);
}

void trimAlertsFile() {
  if (!SD.exists(ALERT_FILE)) return;
  File f = SD.open(ALERT_FILE);
  if (!f) return;

  String lines[MAX_ALERTS * 2];
  int idx = 0;
  while (f.available() && idx < MAX_ALERTS * 2) {
    lines[idx++] = f.readStringUntil('\n');
  }
  f.close();

  if (idx <= MAX_ALERTS) return;

  File w = SD.open(ALERT_FILE, FILE_WRITE);
  if (!w) return;
  for (int i = idx - MAX_ALERTS; i < idx; i++) w.println(lines[i]);
  w.close();
}

void addAlertToFile(const String &msg) {
  File f = SD.open(ALERT_FILE, FILE_APPEND);
  if (f) {
    f.println(msg);
    f.close();
  }
  trimAlertsFile();
}

// ---------- Config save/load ----------
void saveConfig() {
  File f = SD.open(CONFIG_FILE, FILE_WRITE);
  if (!f) return;
  f.printf("ssid=%s\n", wifiSSID.c_str());
  f.printf("password=%s\n", wifiPASS.c_str());
  f.printf("topic=%s\n", topic.c_str());
  f.close();
}

void loadConfig() {
  if (!SD.exists(CONFIG_FILE)) return;
  File f = SD.open(CONFIG_FILE);
  if (!f) return;

  while (f.available()) {
    String line = f.readStringUntil('\n'); line.trim();
    if (line.startsWith("ssid=")) wifiSSID = line.substring(5);
    else if (line.startsWith("password=")) wifiPASS = line.substring(9);
    else if (line.startsWith("topic=")) topic = line.substring(6);
  }
  f.close();
}

// ---------- UI ----------
void drawMenu() {
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setCursor(0,0);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.println("Oboi-DLP Monitor");
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.println("--------------------");
  M5Cardputer.Display.println("(A) Connect WiFi");
  M5Cardputer.Display.println("(B) Set Topic");
  M5Cardputer.Display.println("(C) Start Monitoring");
  M5Cardputer.Display.println("(P) Set Password");
  M5Cardputer.Display.println("");
  M5Cardputer.Display.print("Topic: "); M5Cardputer.Display.println(topic);
  M5Cardputer.Display.print("WiFi: "); M5Cardputer.Display.println(wifiSSID.length()? wifiSSID:"<not set>");
}

void drawInputUI(const String &prompt) {
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setCursor(0,0);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.println(prompt);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.println("");
  M5Cardputer.Display.println(inputBuffer + "_");
}

// ---------- WiFi ----------
void connectWiFi() {
  if (wifiSSID.length()==0) { drawInputUI("No SSID set!"); return; }
  M5Cardputer.Display.clear(); M5Cardputer.Display.setCursor(0,0);
  M5Cardputer.Display.println("Connecting...");
  //Serial.printf("Connecting to WiFi SSID: %s\n", wifiSSID.c_str());
  WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
  int tries = 0;
  while (WiFi.status()!=WL_CONNECTED && tries++ < 60) { delay(250); M5Cardputer.update(); M5Cardputer.Display.print("."); }
  if (WiFi.status()==WL_CONNECTED) {
    M5Cardputer.Display.println("\nConnected!");
    //Serial.println("WiFi connected");
  } else {
    M5Cardputer.Display.println("\nFailed");
    //Serial.println("WiFi connect failed");
  }
  delay(800);
}

// ---------- JSON parsing ----------
String extractMessage(const String &payload) {
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        //Serial.print("JSON parse error: ");
        //Serial.println(err.c_str());
        return payload;
    }

    if (doc.containsKey("message")) return doc["message"].as<String>();
    if (doc.containsKey("title")) return doc["title"].as<String>();
    return payload;
}

String extractMessageId(const String &payload) {
    DynamicJsonDocument doc(1024);
    if (!deserializeJson(doc, payload)) {
        if (doc.containsKey("id")) return doc["id"].as<String>();
    }
    return "";
}


void monitorTopic() {
    unsigned long connectionStartTime = millis();
    const unsigned long STARTUP_IGNORE_TIME = 10000; // still useful safety net

    if (WiFi.status() != WL_CONNECTED) connectWiFi();

    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.println("Monitoring...");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.println("Press X to exit");

    WiFiClient client;
    if (!client.connect("ntfy.sh", 80)) {
        M5Cardputer.Display.clear();
        M5Cardputer.Display.setCursor(0, 0);
        M5Cardputer.Display.println("Failed to connect SSE");
        delay(1500);
        return;
    }

    client.print(String("GET /") + topic + "/sse HTTP/1.1\r\n" +
                 "Host: ntfy.sh\r\n" +
                 "Accept: text/event-stream\r\n" +
                 "Connection: keep-alive\r\n" +
                 "Cache-Control: no-cache\r\n\r\n");

    String lineBuffer;
    String lastMessageId;
    unsigned long lastMessageTime = 0;
    bool headersParsed = false;
    bool firstRealEventSkipped = false;  // <-- NEW FLAG

    while (client.connected()) {
        M5Cardputer.update();

        // Exit on X
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto status = M5Cardputer.Keyboard.keysState();
            for (auto c : status.word) {
                if (c == 'x' || c == 'X') {
                    client.stop();
                    drawMenu();
                    return;
                }
            }
        }

        while (client.available()) {
            char c = client.read();
            if (c == '\n') {
                lineBuffer.trim();

                if (!headersParsed) {
                    if (lineBuffer.length() == 0) headersParsed = true;
                    lineBuffer = "";
                    continue;
                }

                if (lineBuffer.startsWith("data:")) {
                    String jsonData = lineBuffer.substring(5);
                    jsonData.trim();

                    // Ignore empty/keepalive
                    if (jsonData.length() == 0 || jsonData == "{}" || jsonData.indexOf("keepalive") >= 0) {
                        lineBuffer = "";
                        continue;
                    }

                    // Skip very first message after connect (avoids phantom alert)
                    if (!firstRealEventSkipped) {
                        firstRealEventSkipped = true;
                        lineBuffer = "";
                        continue;
                    }

                    // Still skip backlog within first 10s
                    if (millis() - connectionStartTime < STARTUP_IGNORE_TIME) {
                        lineBuffer = "";
                        continue;
                    }

                    // Deduplication logic
                    String messageId = extractMessageId(jsonData);
                    unsigned long now = millis();
                    if ((messageId.length() > 0 && messageId == lastMessageId) ||
                        (now - lastMessageTime < 2000)) {
                        lineBuffer = "";
                        continue;
                    }

                    lastMessageId = messageId;
                    lastMessageTime = now;

                    // Show message on screen + beep
                    String message = extractMessage(jsonData);
                    if (message.length() > 0) {

                        
                      
                      const char *splashPath = "/ntfyrx.png";

                      if (SD.exists(splashPath)) {
                        M5.Lcd.setRotation(1);
                        M5.Lcd.setBrightness(200);
                        M5.Lcd.drawPngFile(SD, "/ntfyrx.png");
                      } else {
                        M5.Lcd.setRotation(1);
                        M5.Lcd.setBrightness(200);
                        M5Cardputer.Display.clear();
                        M5Cardputer.Display.setCursor(0, 0);
                        M5Cardputer.Display.println("  "); 
                        M5Cardputer.Display.println("  ");
                        M5Cardputer.Display.println("  "); 
                        M5Cardputer.Display.println("      █████   ██████  █████  ██████");
                        M5Cardputer.Display.println("     ██   ██    ██    ██       ██ ");
                        M5Cardputer.Display.println("     ██   ██    ██    █████    ██ ");
                        M5Cardputer.Display.println("     ██   ██    ██    ██     ██████ ");
                        M5Cardputer.Display.println("  "); 
                        M5Cardputer.Display.println("       New Notification Received");

                      }

                        
                        beepSOS();
                        delay(3000);

                        M5Cardputer.Display.clear();
                        M5Cardputer.Display.setCursor(0, 0);
                        M5Cardputer.Display.setTextSize(2);
                        M5Cardputer.Display.setTextColor(WHITE);
                        M5Cardputer.Display.println("NEW ALERT:");
                        M5Cardputer.Display.setTextSize(1);
                        M5Cardputer.Display.println("");

                        int lineWidth = 26;
                        for (int i = 0; i < message.length(); i += lineWidth) {
                            M5Cardputer.Display.println(message.substring(i, min(i + lineWidth, (int)message.length())));
                        }
                        M5Cardputer.Display.println("");
                        M5Cardputer.Display.println("Press X to exit");

                        addAlertToFile(String(now) + " " + message);
                        //M5.Lcd.setRotation(1);
                        //M5.Lcd.setBrightness(200);
                        //M5.Lcd.drawPngFile(SD, "/oboi-cardputer-splash.png");
                        //beepSOS();
                        //delay(3000);
                    }
                }
                lineBuffer = "";
            } else {
                lineBuffer += c;
            }
        }
        delay(5);
    }

    client.stop();
    drawMenu();
}


// ---------- setup & loop ----------
void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);
 //Serial.begin(115200);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE);

  initSD(); loadConfig();
  const char *splashPath = "/oboi-cardputer-splash.png";

  if (SD.exists(splashPath)) {
    M5.Lcd.setRotation(1);
    M5.Lcd.setBrightness(200);
    M5.Lcd.drawPngFile(SD, "/oboi-cardputer-splash.png");
  } else {
    M5.Lcd.setRotation(1);
    M5.Lcd.setBrightness(200);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.println("  "); 
    M5Cardputer.Display.println("  ");
    M5Cardputer.Display.println("  "); 
    M5Cardputer.Display.println("  ");
    M5Cardputer.Display.println("  "); 
    M5Cardputer.Display.println("      ████   █████    ████   ████");
    M5Cardputer.Display.println("     ██  ██  ██  ██  ██  ██   ██  ");
    M5Cardputer.Display.println("     ██  ██  █████   ██  ██   ██  ");
    M5Cardputer.Display.println("     ██  ██  ██  ██  ██  ██   ██  ");
    M5Cardputer.Display.println("      ████   █████    ████   ████");
    M5Cardputer.Display.println("");
    M5Cardputer.Display.println("         Data Leak Protection");
    M5Cardputer.Display.println("          By Scot D Forshaw");
  }
    delay(3000);

  drawMenu();
}
void loop() {
  M5Cardputer.update();

  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    auto status = M5Cardputer.Keyboard.keysState();

    if (enteringSSID || enteringPASS || enteringTopic) {
      bool changed = false;

      for (auto c : status.word) {
        inputBuffer += c;
        changed = true;
      }
      if (status.del && inputBuffer.length() > 0) {
        inputBuffer.remove(inputBuffer.length() - 1);
        changed = true;
      }

      if (status.enter) {
        if (enteringSSID) wifiSSID = inputBuffer;
        else if (enteringPASS) wifiPASS = inputBuffer;
        else if (enteringTopic) topic = inputBuffer;

        saveConfig();
        enteringSSID = enteringPASS = enteringTopic = false;
        inputBuffer = "";
        drawMenu();
        return;
      }

      // Only redraw when something actually changed
      if (changed) {
        String prompt = enteringSSID ? "Enter SSID" :
                        enteringPASS ? "Enter PASS" :
                                       "Enter Topic";
        drawInputUI(prompt);
      }
      return;
    }

    // Normal menu key handling
    for (auto c : status.word) {
      if (c == 'a' || c == 'A') { enteringSSID = true; inputBuffer = ""; drawInputUI("Enter SSID"); }
      if (c == 'b' || c == 'B') { enteringTopic = true; inputBuffer = ""; drawInputUI("Enter Topic"); }
      if (c == 'c' || c == 'C') { monitorTopic(); drawMenu(); }
      if (c == 'p' || c == 'P') { enteringPASS = true; inputBuffer = ""; drawInputUI("Enter PASS"); }
    }
  }
}