# Spotify Display

A compact ESP32-powered display that shows your currently playing Spotify track including the song name, artist, and album art on a 1.8" TFT screen, all housed in a custom 3D-printed case.

## Why I Made This

I wanted a physical, always-on display sitting on my desk that shows what I'm listening to without having to look at my phone or computer. 

## 3D Model
<img width="674" height="538" alt="image" src="https://github.com/user-attachments/assets/c9ea7e5f-e3bf-49cc-9a1e-b0e4ccc261aa" />

This is an image of my CAD for the project and as you can see, it has specific places where I can insert the keys, the esp32 and the screen. It's effieicient because you will see none of the wires and will be left with a clean looking display.

## Wiring Diagram

### TFT Display

| TFT Pin | ESP32 Pin |
|---------|-----------|
| CS      | GPIO 5    |
| RST     | GPIO 4    |
| DC      | GPIO 2    |
| SCLK    | GPIO 18   |
| MOSI    | GPIO 23   |
| VCC     | 3.3V      |
| GND     | GND       |

### Keyboard Switches

Each switch has two legs. One leg connects to a GPIO pin, the other to GND.
The firmware uses the ESP32's internal pull-up resistors, so no external resistors are needed.

| Switch       | Function   | ESP32 Pin | Other Leg |
|--------------|------------|-----------|-----------|
| Switch 1     | Play/Pause | GPIO 6    | GND       |
| Switch 2     | Skip       | GPIO 7    | GND       |
| Switch 3     | Previous   | GPIO 8    | GND       |

## Features

- Displays current song name and artist pulled live from Spotify
- Renders album art as a scaled JPEG on the TFT screen
- 3 physical keyboard switches for playback controls (play/pause, skip, previous)
- Custom 3D-printed enclosure designed in Fusion360
- Auto-reconnects and polls Spotify every 2 seconds

## Setup

### Hardware
- ESP32 (LOLIN C3 Mini or equivalent)
- 1.8" ST7735 TFT display
- 3x tactile/keyboard switches
- 4x M3 heatset inserts
- 3D-printed case (see `/cad` folder)

### Software
1. Install [Arduino IDE](https://www.arduino.cc/en/software/)
2. Install the ESP32 board via Board Manager
3. Install libraries:
   - `SpotifyEsp32` (from `/libraries` folder or as .zip)
   - `Adafruit_ST7735`
   - `Adafruit_GFX`
   - `JPEGDecoder`
4. Create a Spotify app at [developer.spotify.com](https://developer.spotify.com) and copy your Client ID and Client Secret
5. Fill in your WiFi credentials and Spotify credentials in the sketch
6. Upload, note the IP shown on screen, and set your Spotify redirect URI to `http://<that-ip>/callback`
7. Open that URL in a browser on the same network to authenticate

## Bill of Materials

| Item | Quantity | Unit Price | 
|------|----------|------------|
| ESP32 LOLIN C3 Mini | 1 | $5.66 |
| 1.8" ST7735 TFT Display | 1 | $2.85 |
| Tactile / Keyboard Switches | 100 | $1.78 |
