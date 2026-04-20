#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>  // changed from ST7735 to ILI9341 for 3.5" screen
#include <WiFi.h>
#include <SpotifyEsp32.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <JPEGDecoder.h>

// --- TFT Pin Definitions ---
#define TFT_CS    0
#define TFT_RST   1
#define TFT_DC    2
#define TFT_MOSI  3
#define TFT_SCLK  4

// --- Button Pin Definitions ---
#define BTN_PLAYPAUSE 5
#define BTN_SKIP      6
#define BTN_PREVIOUS  7

// --- Rotary Encoder Pin Definitions ---
#define ENC_CLK   8
#define ENC_DT    9
#define ENC_SW    10

// --- Credentials ---
char*       SSID          = "YOUR_WIFI_SSID";
char*       PASSWORD      = "YOUR_WIFI_PASSWORD";
const char* CLIENT_ID     = "1e7799fcda8b4b27bb4e4ebc03d721e6";
const char* CLIENT_SECRET = "Y7ef91d4642fb48039a2cd1e88cb279b3";

// --- Objects ---
Spotify          sp(CLIENT_ID, CLIENT_SECRET);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// --- State ---
String lastArtist   = "";
String lastTrack    = "";
String lastImageUrl = "";
int    volume       = 50; // default volume 0-100

// --- Encoder state ---
int  lastEncCLK      = HIGH;
long lastEncTime     = 0;
bool lastSwState     = HIGH;
long lastSwTime      = 0;

// --- Layout (320x480 landscape = 480 wide, 320 tall) ---
#define ART_X    0
#define ART_Y    0
#define ART_SIZE 160   // bigger art for bigger screen
#define TEXT_X   170
#define TEXT_W   (480 - TEXT_X)

// -----------------------------------------------------------
// Fetch JPEG from URL into heap buffer
// -----------------------------------------------------------
uint8_t* fetchJpeg(const String& url, int& outSize) {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("Image fetch failed, HTTP code: %d\n", httpCode);
        http.end();
        return nullptr;
    }

    int len = http.getSize();
    if (len <= 0 || len > 60000) {  // increased limit for bigger screen
        Serial.printf("Image size out of range: %d bytes\n", len);
        http.end();
        return nullptr;
    }

    uint8_t* buf = (uint8_t*)malloc(len);
    if (!buf) {
        Serial.println("malloc failed — not enough heap");
        http.end();
        return nullptr;
    }

    WiFiClient* stream = http.getStreamPtr();
    int bytesRead = 0;
    unsigned long timeout = millis() + 5000;
    while (bytesRead < len && millis() < timeout) {
        int avail = stream->available();
        if (avail > 0) {
            bytesRead += stream->readBytes(buf + bytesRead, avail);
        }
    }

    http.end();

    if (bytesRead < len) {
        Serial.println("Image download timed out");
        free(buf);
        return nullptr;
    }

    outSize = bytesRead;
    return buf;
}

// -----------------------------------------------------------
// Decode and draw JPEG scaled to ART_SIZE x ART_SIZE
// -----------------------------------------------------------
void drawJpeg(uint8_t* data, int size, int x, int y) {
    if (!JpegDec.decodeArray(data, size)) {
        Serial.println("JPEG decode failed");
        return;
    }

    int srcW = JpegDec.width;
    int srcH = JpegDec.height;

    while (JpegDec.read()) {
        uint16_t* pImg = JpegDec.pImage;
        int mcuX = JpegDec.MCUx * JpegDec.MCUWidth;
        int mcuY = JpegDec.MCUy * JpegDec.MCUHeight;

        for (int row = 0; row < JpegDec.MCUHeight; row++) {
            for (int col = 0; col < JpegDec.MCUWidth; col++) {
                int srcPxX = mcuX + col;
                int srcPxY = mcuY + row;

                int dstX = x + (srcPxX * ART_SIZE) / srcW;
                int dstY = y + (srcPxY * ART_SIZE) / srcH;

                if (dstX < x + ART_SIZE && dstY < y + ART_SIZE) {
                    uint16_t color = pImg[row * JpegDec.MCUWidth + col];
                    color = (color >> 8) | (color << 8);
                    tft.drawPixel(dstX, dstY, color);
                }
            }
        }
    }
}

// -----------------------------------------------------------
// Fetch and render album art
// -----------------------------------------------------------
void updateAlbumArt(const String& url) {
    if (url.isEmpty() || url == "Something went wrong") return;

    Serial.println("Fetching album art...");
    int jpegSize = 0;
    // use index 1 (medium ~300px) since screen is bigger now
    uint8_t* jpegData = fetchJpeg(url, jpegSize);

    if (jpegData) {
        tft.fillRect(ART_X, ART_Y, ART_SIZE, ART_SIZE, ILI9341_BLACK);
        drawJpeg(jpegData, jpegSize, ART_X, ART_Y);
        free(jpegData);
        Serial.println("Album art drawn!");
    } else {
        Serial.println("Could not draw album art");
    }
}

// -----------------------------------------------------------
// Clear a region and print text
// -----------------------------------------------------------
void drawText(int x, int y, int w, int h,
              const String& text, uint8_t size, uint16_t color) {
    tft.fillRect(x, y, w, h, ILI9341_BLACK);
    tft.setCursor(x, y);
    tft.setTextSize(size);
    tft.setTextColor(color);
    tft.setTextWrap(true);
    tft.write(text.c_str());
}

// -----------------------------------------------------------
// Draw volume bar on screen
// -----------------------------------------------------------
void drawVolumeBar(int vol) {
    int barX    = TEXT_X;
    int barY    = 280;
    int barMaxW = TEXT_W - 10;
    int barH    = 12;
    int fillW   = (vol * barMaxW) / 100;

    // background
    tft.fillRect(barX, barY, barMaxW, barH, ILI9341_DARKGREY);
    // fill
    tft.fillRect(barX, barY, fillW, barH, ILI9341_GREEN);
    // label
    tft.setCursor(barX, barY - 16);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.fillRect(barX, barY - 16, 60, 10, ILI9341_BLACK);
    tft.print("Vol: " + String(vol) + "%");
}

// -----------------------------------------------------------
// Boot animation
// -----------------------------------------------------------
void drawBootAnimation() {
    tft.fillScreen(ILI9341_BLACK);

    int cx = 240, cy = 160; // center of 480x320

    // grow circle
    for (int r = 0; r <= 80; r += 3) {
        tft.drawCircle(cx, cy, r, ILI9341_GREEN);
        delay(20);
    }
    tft.fillCircle(cx, cy, 80, ILI9341_GREEN);
    delay(100);

    // draw 3 equalizer bars
    int barW    = 18;
    int barGap  = 10;
    int barBaseY = cy + 25;
    int heights[] = {45, 60, 38};
    int startX  = cx - (3 * barW + 2 * barGap) / 2;

    for (int i = 0; i < 3; i++) {
        int bx = startX + i * (barW + barGap);
        tft.fillRect(bx, barBaseY - heights[i], barW, heights[i], ILI9341_WHITE);
        delay(80);
    }

    delay(600);

    // animate bars
    for (int anim = 0; anim < 4; anim++) {
        int newHeights[] = {
            (anim % 2 == 0) ? 30 : 55,
            (anim % 2 == 0) ? 58 : 28,
            (anim % 2 == 0) ? 48 : 38
        };
        tft.fillCircle(cx, cy, 80, ILI9341_GREEN);
        for (int i = 0; i < 3; i++) {
            int bx = startX + i * (barW + barGap);
            tft.fillRect(bx, barBaseY - newHeights[i], barW, newHeights[i], ILI9341_WHITE);
        }
        delay(150);
    }

    delay(400);

    // shrink circle
    for (int r = 80; r >= 0; r -= 4) {
        tft.fillCircle(cx, cy, r + 4, ILI9341_BLACK);
        tft.fillCircle(cx, cy, r, ILI9341_GREEN);
        delay(20);
    }
    tft.fillScreen(ILI9341_BLACK);
    delay(200);
}

// -----------------------------------------------------------
// Handle rotary encoder for volume
// -----------------------------------------------------------
void handleEncoder() {
    int clkState = digitalRead(ENC_CLK);
    long now = millis();

    // Rotation — debounce 5ms
    if (clkState != lastEncCLK && (now - lastEncTime) > 5) {
        lastEncTime = now;
        if (digitalRead(ENC_DT) != clkState) {
            // clockwise — volume up
            volume = min(100, volume + 5);
        } else {
            // counter-clockwise — volume down
            volume = max(0, volume - 5);
        }
        sp.set_volume(volume);
        drawVolumeBar(volume);
        Serial.println("Volume: " + String(volume));
    }
    lastEncCLK = clkState;

    // Click — debounce 200ms
    bool swState = digitalRead(ENC_SW);
    if (swState == LOW && lastSwState == HIGH && (now - lastSwTime) > 200) {
        lastSwTime = now;
        sp.start_a_users_playback();
        Serial.println("Encoder click: play/pause");
    }
    lastSwState = swState;
}

// -----------------------------------------------------------
// SETUP
// -----------------------------------------------------------
void setup() {
    Serial.begin(115200);

    // Button pins
    pinMode(BTN_PLAYPAUSE, INPUT_PULLUP);
    pinMode(BTN_SKIP,      INPUT_PULLUP);
    pinMode(BTN_PREVIOUS,  INPUT_PULLUP);

    // Encoder pins
    pinMode(ENC_CLK, INPUT_PULLUP);
    pinMode(ENC_DT,  INPUT_PULLUP);
    pinMode(ENC_SW,  INPUT_PULLUP);

    // Init TFT
    tft.begin();
    tft.setRotation(1);         // landscape: 480 wide, 320 tall
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    Serial.println("TFT ready");

    // Boot animation
    drawBootAnimation();

    // Connect to WiFi
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.write("Connecting to WiFi...");
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");

    String ip = WiFi.localIP().toString();
    Serial.println("IP: " + ip);
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.write("WiFi connected!\nIP:\n");
    tft.write(ip.c_str());
    delay(3000);

    // Authenticate Spotify
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.write("Open browser:\n");
    tft.write(ip.c_str());
    tft.write("/callback\n\nto authenticate\nSpotify");

    sp.begin();
    while (!sp.is_auth()) {
        sp.handle_client();
    }
    Serial.println("Spotify authenticated!");

    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.write("Spotify ready!");
    delay(1500);
    tft.fillScreen(ILI9341_BLACK);

    // Draw initial volume bar
    drawVolumeBar(volume);
}

// -----------------------------------------------------------
// LOOP
//
// Screen layout (480x320 landscape):
//  +---------+--------------------------------+
//  |         | Artist (green, size 3)         |
//  | 160x160 |                                |
//  |  art    | Track  (white, size 2)         |
//  |         |                                |
//  |         | [====volume bar====]           |
//  +---------+--------------------------------+
// -----------------------------------------------------------
void loop() {
    // --- Encoder ---
    handleEncoder();

    // --- Buttons ---
    if (digitalRead(BTN_PLAYPAUSE) == LOW) {
        sp.start_a_users_playback();
        delay(300);
    }
    if (digitalRead(BTN_SKIP) == LOW) {
        sp.skip_to_next();
        delay(300);
    }
    if (digitalRead(BTN_PREVIOUS) == LOW) {
        sp.skip_to_previous();
        delay(300);
    }

    // --- Spotify polling ---
    String currentArtist = sp.current_artist_names();
    String currentTrack  = sp.current_track_name();
    String currentUrl    = sp.get_current_album_image_url(1); // index 1 = medium for bigger screen

    bool trackChanged = (currentTrack  != lastTrack  &&
                         currentTrack  != "Something went wrong" &&
                         currentTrack  != "null" &&
                         !currentTrack.isEmpty());

    bool artistChanged = (currentArtist != lastArtist &&
                          currentArtist != "Something went wrong" &&
                          !currentArtist.isEmpty());

    if (trackChanged) {
        lastTrack = currentTrack;

        if (currentUrl != lastImageUrl && !currentUrl.isEmpty()) {
            lastImageUrl = currentUrl;
            updateAlbumArt(currentUrl);
        }

        Serial.println("Track: " + lastTrack);
        drawText(TEXT_X, 100, TEXT_W, 80, lastTrack, 2, ILI9341_WHITE);
    }

    if (artistChanged) {
        lastArtist = currentArtist;
        Serial.println("Artist: " + lastArtist);
        drawText(TEXT_X, 20, TEXT_W, 60, lastArtist, 3, ILI9341_GREEN);
    }

    delay(2000);
}
