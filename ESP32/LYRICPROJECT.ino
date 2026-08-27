#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_0 18
#define SCL_0 19
#define SDA_1 21
#define SCL_1 22

Adafruit_SSD1306 display_096(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SH1106G display_130(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1);

const char* ssid     = "            ";
const char* password = "      ";
WebServer server(80);

String songTitle = "No Track";
String artistName = "Unknown Artist";
String currentLyricLine = "Waiting for sync...";
long trackPositionMs = 0;
unsigned long lastUpdate = 0;
bool isPlaying = false;
bool needToFetchLyrics = false;

const int MAX_LINES = 60; 
long lineTimes[MAX_LINES];
String lineText[MAX_LINES];
int totalLines = 0;

const int numBars = 8;
int barWidth = 12;
int barSpacing = 4;
int currentHeights[numBars] = {0};
int targetHeights[numBars] = {0};

TaskHandle_t FetchTask;

long parseLrcTime(String timeStr) {
  if(timeStr.length() < 7) return 0;
  int min = timeStr.substring(1, 3).toInt();
  int sec = timeStr.substring(4, 6).toInt();
  int ms  = timeStr.substring(7, 9).toInt() * 10;
  return (min * 60000) + (sec * 1000) + ms;
}

void FetchTaskCode(void * pvParameters) {
  for(;;) {
    server.handleClient();

    if (needToFetchLyrics) {
      needToFetchLyrics = false;
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String queryTitle = songTitle;
        String queryArtist = artistName;
        queryTitle.replace(" ", "%20");
        queryArtist.replace(" ", "%20");
        
        String url = "https://lrclib.net/api/search?q=" + queryArtist + "%20" + queryTitle;
        http.begin(url);
        
        int httpCode = http.GET();
        if (httpCode == 200) {
          String payload = http.getString();
          DynamicJsonDocument doc(8192); 
          DeserializationError error = deserializeJson(doc, payload);
          
          if (!error && doc.isArray() && doc.size() > 0) {
            String syncedLyrics = doc[0]["syncedLyrics"].as<String>();
            totalLines = 0;
            
            int startIdx = 0;
            while (startIdx < syncedLyrics.length() && totalLines < MAX_LINES) {
              int endIdx = syncedLyrics.indexOf('\n', startIdx);
              if (endIdx == -1) endIdx = syncedLyrics.length();
              
              String line = syncedLyrics.substring(startIdx, endIdx);
              if (line.indexOf('[') == 0 && line.indexOf(']') > 0) {
                int closeBracket = line.indexOf(']');
                String timePart = line.substring(0, closeBracket + 1);
                String textPart = line.substring(closeBracket + 1);
                textPart.trim();
                
                lineTimes[totalLines] = parseLrcTime(timePart);
                lineText[totalLines] = textPart;
                totalLines++;
              }
              startIdx = endIdx + 1;
            }
          }
        }
        http.end();
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void handleUpdate() {
  if (server.hasArg("title")) {
    String newTitle = server.arg("title");
    if(newTitle != songTitle) {
      songTitle = newTitle;
      needToFetchLyrics = true; 
    }
    isPlaying = true;
    lastUpdate = millis();
  }
  if (server.hasArg("artist")) artistName = server.arg("artist");
  if (server.hasArg("position")) trackPositionMs = server.arg("position").toInt();

  server.send(200, "text/plain", "OK");
}

void drawVisualizer() {
  display_096.clearDisplay();
  display_096.setCursor(0, 0);
  display_096.setTextSize(1);
  display_096.setTextColor(SSD1306_WHITE);
  display_096.println(songTitle);
  
  for (int i = 0; i < numBars; i++) {
    if (isPlaying) {
      if (currentHeights[i] == targetHeights[i] || random(0, 10) > 7) {
        targetHeights[i] = random(8, 48); 
      }
    } else {
      targetHeights[i] = 0;
    }
    if (currentHeights[i] < targetHeights[i]) currentHeights[i] += 4; 
    if (currentHeights[i] > targetHeights[i]) currentHeights[i] -= 3;
    currentHeights[i] = constrain(currentHeights[i], 0, 45);

    int x = i * (barWidth + barSpacing) + 2;
    int y = SCREEN_HEIGHT - currentHeights[i];
    display_096.fillRect(x, y, barWidth, currentHeights[i], SSD1306_WHITE);
  }
  display_096.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_0, SCL_0, 100000);
  Wire1.begin(SDA_1, SCL_1, 100000);
  
  display_096.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display_130.begin(0x3C, true);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  display_096.clearDisplay();
  display_096.println("System Connected!");
  display_096.println(WiFi.localIP().toString());
  display_096.display();

  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();

  xTaskCreatePinnedToCore(FetchTaskCode, "FetchTask", 10000, NULL, 1, &FetchTask, 0);
}

void loop() {
  if (millis() - lastUpdate > 8000) isPlaying = false;

  static unsigned long lastFrame = 0;
  if (millis() - lastFrame > 33) { 
    lastFrame = millis();
    drawVisualizer();
  }

  if (isPlaying && totalLines > 0) {
    String targetLine = "";
    for (int i = 0; i < totalLines; i++) {
      if (trackPositionMs >= lineTimes[i]) {
        targetLine = lineText[i];
      } else {
        break; 
      }
    }
    
   
    if (targetLine != currentLyricLine && targetLine != "") {
      currentLyricLine = targetLine;
      
      display_130.clearDisplay();
      display_130.setTextColor(SH110X_WHITE);
      display_130.setTextWrap(true);
      display_130.setCursor(0, 16);
      display_130.setTextSize(2); 
      display_130.println(currentLyricLine);
      display_130.display();
    }
  }
}
