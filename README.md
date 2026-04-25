# Spotify Display
A compact ESP32-powered display that shows your currently playing Spotify track including 
the song name, artist, and album art on a 3.5" TFT screen, all housed in a custom 
3D-printed case. Features a rotary encoder for volume control and 3 mechanical keyboard 
switches for playback controls.

## Why I Made This
I wanted a physical, always-on display sitting on my desk that shows what I'm listening 
to without having to look at my phone or computer.

## 3D Model

<img width="331" height="295" alt="image" src="https://github.com/user-attachments/assets/ee5327a1-cf28-4acf-a30d-6e1cbeeb2066" />

This is an image of my CAD for the project and as you can see, it has specific places 
where I can insert the keys, the ESP32, the rotary encoder, and the screen. It's 
efficient because you will see none of the wires and will be left with a clean looking 
display.

## Wiring Diagram
<img width="1057" height="649" alt="image" src="https://github.com/user-attachments/assets/65158f05-2e12-4588-b4a9-db7bb379686b" />


### TFT Display
| TFT Pin  | ESP32 Pin |
|----------|-----------|
| CS       | GPIO 0    |
| RST      | GPIO 1    |
| DC/RS    | GPIO 2    |
| SDI/MOSI | GPIO 3    |
| SCK      | GPIO 4    |
| VCC      | 3.3V      |
| GND      | GND       |
| LED      | 3.3V      |

### Keyboard Switches
Each switch has two legs. One leg connects to a GPIO pin, the other to GND.
The firmware uses the ESP32's internal pull-up resistors, so no external resistors are needed.

| Switch   | Function   | ESP32 Pin | Other Leg |
|----------|------------|-----------|-----------|
| Switch 1 | Play/Pause | GPIO 5    | GND       |
| Switch 2 | Skip       | GPIO 6    | GND       |
| Switch 3 | Previous   | GPIO 7    | GND       |

### Rotary Encoder
The rotary encoder controls volume. Rotating clockwise increases volume, 
counter-clockwise decreases it. Pressing the encoder click acts as an 
additional play/pause button.

| Encoder Pin | ESP32 Pin |
|-------------|-----------|
| CLK         | GPIO 8    |
| DT          | GPIO 10    | 
| VCC         | 3.3V      |
| GND         | GND       |

## Features
- Displays current song name and artist pulled live from Spotify
- Renders album art as a scaled JPEG on the 3.5" TFT screen
- 3 mechanical keyboard switches for playback controls (play/pause, skip, previous)
- Rotary encoder for smooth volume control
- Boot animation on startup
- Custom 3D-printed enclosure designed in Fusion360
- Auto-reconnects and polls Spotify every 2 seconds

## Setup

### Hardware
- ESP32 LOLIN C3 Mini
- 3.5" ILI9341 TFT display
- 3x mechanical keyboard switches
- 1x rotary encoder (KY-040 or EC11)
- 3D-printed case (see `/cad` folder)

### Software
1. Install [Arduino IDE](https://www.arduino.cc/en/software/)
2. Install the ESP32 board via Board Manager
3. Install libraries:
   - `SpotifyEsp32` (from `/libraries` folder or as .zip)
   - `Adafruit_ILI9341`
   - `Adafruit_GFX`
   - `JPEGDecoder`
4. Create a Spotify app at [developer.spotify.com](https://developer.spotify.com) 
   and copy your Client ID and Client Secret
5. Fill in your WiFi credentials and Spotify credentials in the sketch
6. Upload, note the IP shown on screen, and set your Spotify redirect URI 
   to `http://<that-ip>/callback`
7. Open that URL in a browser on the same network to authenticate

## Bill of Materials

| Item                         | Quantity | Unit Price |
|------------------------------|----------|------------|
| ESP32 LOLIN C3 Mini          | 1        | $5.66      |
| 3.5" ILI9341 TFT Display     | 1        | $8.18      |
| Mechanical Keyboard Switches | 1 pack   | $0.99      |
| Rotary Encoder               | 1        | $2.34      |
| **Total**                    |          | **$17.17** |
