# NOVA STRIKE — Arcade Space Shooter

A pixel-perfect arcade shooter running entirely on a **Seeed Xiao ESP32-S3** microcontroller with a **0.96" OLED display** and **MAX98357A audio amplifier**.

> **1,151 lines of C++** | **~50 FPS** | **3 Enemy Types** | **4 Power-ups** | **Infinite Waves**

## Quick Start

### Hardware Required
- **Seeed Xiao ESP32-S3** (Xtensa LX7 @ 240 MHz, 512KB SRAM, 8MB Flash)
- **SSD1306 0.96" OLED** (128×64, I²C, Monochrome)
- **4× TTP223 Capacitive Touch Sensors** (or push switches with 10kΩ pull-down)
- **MAX98357A I²S Amplifier** (3.2W @ 4Ω, 5V recommended)

### Wiring

#### OLED (I²C)
| Pin | GPIO | Label |
|-----|------|-------|
| SDA | GPIO5 | D4 |
| SCL | GPIO6 | D5 |
| VCC | 3.3V | — |
| GND | GND | — |

#### Buttons (HIGH = Pressed)
| Button | GPIO | Label | Notes |
|--------|------|-------|-------|
| UP | GPIO1 | D0 | TTP223 or switch + 10kΩ pull-down |
| DOWN | GPIO9 | D10 | *Moved to avoid SPK conflict* |
| FIRE | GPIO3 | D2 | TTP223 or switch + 10kΩ pull-down |
| BOMB | GPIO44 | D7 | TTP223 or switch + 10kΩ pull-down |

#### MAX98357A (I²S)
| Pin | GPIO | Label | Notes |
|-----|------|-------|-------|
| BCLK | GPIO7 | D8 | Bit Clock |
| LRC | GPIO4 | D3 | Left/Right Clock |
| DIN | GPIO2 | D1 | Data In |
| VIN | 5V | — | Recommended for audio quality |
| GND | GND | — | — |

> **Note:** If using GPIO2 for speaker DIN conflicts with your button layout, move DOWN button to GPIO9 (D10) as shown above.

---

## Installation & Upload

### Prerequisites
1. Install **PlatformIO** in VS Code
2. Install **Arduino IDE** (for library management)
3. From Arduino Library Manager, install:
   - **Adafruit SSD1306**
   - **Adafruit GFX Library**

### Build & Upload
```bash
# Using PlatformIO
pio run -t upload

# Or in VS Code:
# 1. Open command palette (Ctrl+Shift+P)
# 2. Search "PlatformIO: Upload" and press Enter
```

### Serial Monitor
```bash
pio device monitor -b 115200
```

---

## Controls

| Button | Action |
|--------|--------|
| **UP** | Move ship upward (hold for continuous) |
| **DOWN** | Move ship downward (hold for continuous) |
| **FIRE** | Single shot on press; hold for rapid fire (every 4 frames); **TriShot** adds two diagonal bullets |
| **BOMB** | Smart bomb — expanding ring clears all enemies; deals 3 HP to boss; max 5 bombs; starts with 3 |

---

## Gameplay

Your ship is **fixed on the left side** of the screen. Enemies scroll in from the right.

- **Objective:** Shoot enemies before they pass or collide with you
- **Lives:** Start with 3; lose one on collision or boss bullet hit (unless shielded)
- **Invincibility:** 120-frame grace period after each hit
- **Wave System:** Every 10 kills advances the wave; enemies get faster and more aggressive
- **Boss Waves:** Every 3rd wave spawns a multi-hit boss
- **Idle Timeout:** 6 seconds of inactivity returns to title with hyperspace animation

### Scoring
- **Drifter:** 10 pts
- **Hunter:** 20 pts
- **Kamikaze:** 30 pts
- **Boss:** 500 + wave × 100 pts
- **Bomb Kill:** 5 pts per enemy

---

## Enemy Types

### Drifter
- **Behavior:** Moves straight across screen at constant speed
- **HP:** 1
- **Speed:** Slow, increases with wave
- **Pattern:** Slight vertical drift

### Hunter
- **Behavior:** Slowly homes in on player Y position
- **HP:** 2
- **Speed:** Medium
- **Pattern:** Tracks player vertically while scrolling left

### Kamikaze
- **Behavior:** Hovers, then dashes directly at player
- **HP:** 1
- **Speed:** Fast dash, 2.2× wave speed
- **Pattern:** 40-frame hover + approach phase, then dash

### Boss
- **Behavior:** Slides in from right, bounces up/down, shoots 3-bullet spreads
- **HP:** 8 + 4 × wave
- **Speed:** Boss HP = wave difficulty
- **Reward:** 500 + 100 × wave pts
- **Spawn:** Every 3 waves (Wave 3, 6, 9, etc.)

---

## Power-ups

Spawn with **20% chance** when enemies die. Auto-collect when ship touches them.

| Type | Icon | Effect | Duration |
|------|------|--------|----------|
| **Shield** | S | Absorbs next hit; blocks one collision | 300 frames |
| **TriShot** | T | Shoot 3 bullets: center + diagonals | 400 frames |
| **Speed Boost** | V | Movement speed × 1.7; stars scroll 1.5× | 350 frames |
| **Bomb Refill** | B | +1 bomb (max 5) | Instant |

---

## HUD Display

- **Lives:** Left side, small triangle icons
- **Bombs:** Right side, small circle indicators
- **Score:** Center-bottom, 6-digit counter
- **Wave:** Top-center "W#" indicator
- **Power-up Status:** Top-left: S (Shield), T (TriShot), V (Speed Boost)

---

## Animations & Effects

### Boot Sequence
1. **Scanline Fill** — White lines scan down screen (8ms per line)
2. **"NOVA" Title** — Large text appears
3. **Flash & "STRIKE!"** — 3 inverts with audio jingle

### Title Screen
- Parallax starfield (3-layer)
- Animated ship flying left-to-right
- Wavy underline effect on title
- High score display
- Idle countdown bar after 2.5 seconds

### Wave Clear
- Radiating lines from center
- "WAVE X CLEARED!" message
- 90-frame animation before next wave

### Boss Entry
- "! BOSS !" warning (flashing)
- Boss slides in from right
- Wave sound effect triggers gameplay

### Death Animation
- Expanding shockwave ring from ship
- "SHIP LOST" + score display
- Invincibility period, then score screen

### Score Screen
- Animated score count-up (×8 per frame)
- Star medal icon (5-point star)
- Best score display
- "* NEW RECORD *" if high score beaten
- Idle bar for auto-return to title

### Return to Title
- **Hyperspace Effect:** Stars streak horizontally
- **Letter-by-letter animation:** "NOVA STRIKE" slides in

---

## Audio

### Sound Effects (16 kHz, mono, I²S)
- **Shoot:** 880 Hz, 22 ms (soft)
- **Hit:** 220 Hz, 35 ms (mid-volume)
- **Explode:** 180 Hz + 120 Hz sequence, 60–80 ms
- **Bomb:** 440/330/220 Hz chord sequence
- **Power-up:** 660/880/1320 Hz ascending
- **Shield Hit:** 440 Hz, 30 ms (quiet)
- **Boss Death:** 800–560 Hz descending sequence
- **Jingle (Start):** 523/659/784/1047 Hz (1 octave)
- **Game Over:** 392/330/262 Hz descending
- **Wave Clear:** 1047/1319/1568 Hz ascending

### Envelope
- **Attack:** 80 samples (~5 ms)
- **Release:** 160 samples (~10 ms)
- **Amplitude:** 0–32767 (16-bit signed)

---

## Code Structure

```
src/main.cpp (1,151 lines)
├── Display setup (SSD1306, 128×64)
├── Button input (debounce, edge detection, hold state)
├── I²S Audio (tone queue, envelope, synthesis)
├── Game objects (Ship, Enemies, Boss, Bullets, Particles, Power-ups)
├── Collision detection (pixel-perfect)
├── Sprite rendering (pixel art: 7×7 ship, 5×5 enemies, 13×9 boss)
├── Game state machine (Title, Playing, Wave Clear, Boss, Dead, Score)
├── Animation systems (particles, parallax stars, UI)
└── Main loop (60 Hz target)
```

### Key Constants
- **Screen:** 128×64 pixels
- **Ship Speed:** 2 px/frame (3.4 px/frame with boost)
- **Bullet Speed:** 5 px/frame
- **Wave Scaling:** Speed = 1.0 + 0.25 × wave
- **Enemy Spawn Rate:** 60 – 8 × wave frames (min 15)
- **Boss Spawn:** Every 3 waves (1, 4, 7, 10, ...)

---

## Performance

- **Target Frame Rate:** 50 FPS (20 ms per frame)
- **Title Screen:** 28 ms delay / frame
- **Playing:** 18 ms delay / frame (fast, tight loop)
- **Wave Clear:** 20 ms delay / frame
- **Score Screen:** 22 ms delay / frame
- **Memory:** Entire game fits in 512 KB SRAM (no PSRAM needed)

---

## Troubleshooting

### OLED not displaying
- Check I²C pull-up resistors (should be on OLED module or add 4.7 kΩ)
- Verify SDA/SCL on GPIO5/GPIO6
- Ensure Wire.begin(5, 6) matches your pins
- Try I²C address 0x3C (default for most 0.96" modules)

### Buttons not responding
- Verify GPIO pins match wiring diagram
- TTP223: Check polarity (3.3V → VCC, GND → GND)
- Push switches: Confirm pull-down resistors are 10 kΩ
- Read serial output: `Serial.print(digitalRead(BTN_UP))` should toggle

### No sound
- Verify I²S pins: BCLK (7), LRC (4), DIN (2)
- Check MAX98357A power (5V recommended)
- Confirm speaker connections (L+/L− or mono out)
- Try adjusting volume with `queueTone(freq, ms, 0.5f)` parameter

### Game feels slow / stuttery
- Reduce star count (MAX_STARS = 25)
- Disable complex particle effects temporarily
- Check if Serial.print() statements are slowing loop
- Verify PlatformIO isn't running debug build

---

## License

Open source. Feel free to modify and redistribute with attribution.

---

## References

- [Seeed Xiao ESP32-S3 Docs](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [ESP-IDF I²S Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2s.html)

---

**Built with ❤️ for the ESP32 ecosystem**
