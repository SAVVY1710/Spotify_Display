#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <SpotifyEsp32.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <JPEGDecoder.h>

// --- Pin Definitions ---
#define TFT_CS    5
#define TFT_RST   4
#define TFT_DC    2
#define TFT_MOSI  7
#define TFT_SCLK  6

#define BTN_PLAYPAUSE 8
#define BTN_SKIP      9
#define BTN_PREVIOUS  10

// --- Credentials ---
char*       SSID          = "YOUR_WIFI_SSID";
char*       PASSWORD      = "YOUR_WIFI_PASSWORD";
const char* CLIENT_ID     = "YOUR_SPOTIFY_CLIENT_ID";
const char* CLIENT_SECRET = "YOUR_SPOTIFY_CLIENT_SECRET";

// --- Objects ---
Spotify         sp(CLIENT_ID, CLIENT_SECRET);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// --- State ---
String lastArtist   = "";
String lastTrack    = "";
String lastImageUrl = "";

// --- Layout ---
#define ART_X    0
#define ART_Y    0
#define ART_SIZE 64
#define TEXT_X   68
#define TEXT_W   (160 - TEXT_X)

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
    if (len <= 0 || len > 30000) {
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
// Decode and draw JPEG on TFT scaled to ART_SIZE x ART_SIZE
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
    uint8_t* jpegData = fetchJpeg(url, jpegSize);

    if (jpegData) {
        tft.fillRect(ART_X, ART_Y, ART_SIZE, ART_SIZE, ST77XX_BLACK);
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
    tft.fillRect(x, y, w, h, ST77XX_BLACK);
    tft.setCursor(x, y);
    tft.setTextSize(size);
    tft.setTextColor(color);
    tft.setTextWrap(true);
    tft.write(text.c_str());
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

    // Init TFT
    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    Serial.println("TFT ready");

    // Connect to WiFi
    tft.setCursor(0, 0);
    tft.write("Connecting to WiFi");
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");

    // Show IP — copy this into your Spotify redirect URI
    String ip = WiFi.localIP().toString();
    Serial.println("IP: " + ip);
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.write("WiFi connected!\nIP:\n");
    tft.write(ip.c_str());
    delay(3000);

    // Authenticate Spotify
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.write("Open browser:\n");
    tft.write(ip.c_str());
    tft.write("/callback\n\nto authenticate\nSpotify");

    sp.begin();
    while (!sp.is_auth()) {
        sp.handle_client();
    }
    Serial.println("Spotify authenticated!");

    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.write("Spotify ready!");
    delay(1500);
    tft.fillScreen(ST77XX_BLACK);
}

void loop() {
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
    String currentUrl    = sp.get_current_album_image_url(2);

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
        drawText(TEXT_X, 36, TEXT_W, 56, lastTrack, 1, ST77XX_WHITE);
    }

    if (artistChanged) {
        lastArtist = currentArtist;
        Serial.println("Artist: " + lastArtist);
        drawText(TEXT_X, 4, TEXT_W, 28, lastArtist, 1, ST77XX_GREEN);
    }

    delay(2000);
}
