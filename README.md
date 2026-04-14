# Spotify Display

A compact ESP32-powered display that shows your currently playing Spotify track including the song name, artist, and album art on a 1.8" TFT screen, all housed in a custom 3D-printed case.

## Why I Made This

I wanted a physical, always-on display sitting on my desk that shows what I'm listening to without having to look at my phone or computer. 

## 3D Model
<img width="674" height="538" alt="image" src="https://github.com/user-attachments/assets/c9ea7e5f-e3bf-49cc-9a1e-b0e4ccc261aa" />

<img width="718" height="499" alt="image" src="https://github.com/user-attachments/assets/9be779b4-c3a9-4cf7-871d-b8119ea59dd0" />

This is an image of my CAD for the project and as you can see, it has specific places where I can insert the keys, the esp32 and the screen. It's effieicient because you will see none of the wires and will be left with a clean looking display.

## Wiring Diagram
<img width="910" height="611" alt="image" src="https://github.com/user-attachments/assets/c915eeb1-df1e-4366-ae59-d9f3b7113b5f" />

### TFT Display

| TFT Pin | ESP32 Pin |
|---------|-----------|
| CS      | GPIO 5    |
| RST     | GPIO 4    |
| A0      | GPIO 2    |
| SCK     | GPIO 6    |
| SDA     | GPIO 7    |
| VCC     | 3.3V      |
| GND     | GND       |
| LED     | 3.3V      |

### Keyboard Switches

Each switch has two legs. One leg connects to a GPIO pin, the other to GND.
The firmware uses the ESP32's internal pull-up resistors, so no external resistors are needed.

| Switch       | Function   | ESP32 Pin | Other Leg |
|--------------|------------|-----------|-----------|
| Switch 1     | Play/Pause | GPIO 8    | GND       |
| Switch 2     | Skip       | GPIO 9    | GND       |
| Switch 3     | Previous   | GPIO 10   | GND       |

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
