# AURA 1 - Autonomous Desktop Companion Robot

Author: Arfat Ansari
Target: Maker Faire Shenzhen 2026 / Hack Club Fallen

This is AURA 1, my autonomous companion robot project. I am building this because I wanted an animatronic companion sitting on my desk that actually feels responsive and alive when I am working or talking, instead of just sitting there like a static piece of plastic.

Most commercial desktop robots either cost hundreds of dollars or their neck movements look so jerky and robotic like a toy servo snapping around. My goal with AURA 1 is to build an open hardware companion robot with smooth organic motion, expressive circular color screens, real time laser distance sensing, and sound reactivity, all running on an ESP32-S3 microcontroller.

## What It Does

- 3-DOF Kinematic Head: The head moves in 3 axes (pan left/right, pitch up/down, roll tilt sideways) driven by three TowerPro MG90S metal gear servos linked with carbon fiber pushrods and brass ball joints on a central 608ZZ bearing turntable.
- Capacitive Petting and Purr: A capacitive touch zone across the skull top detects human hand petting. When stroked, the servos do a subtle 25Hz haptic purr vibration, the eyes squint into happy anime arches, and the neck halo glows warm amber.
- Dual Round Color Eyes: Two GC9A01 1.28 inch 240x240 circular IPS LCD screens running at 60 FPS over dual SPI channels. The eyes have animated pupils that dilate, blink, and track movement with natural saccades.
- Laser Distance Perception: An ST VL53L1X time-of-flight laser distance sensor on the forehead reads ranges up to 4 meters, so the robot knows when someone is sitting in front of it and turns to look at them.
- Sound Reactive: An INMP441 I2S digital MEMS microphone streams audio over DMA with real time FFT beat detection, allowing the robot to nod or react to music and voices.
- Emotion Halo: A 12-LED WS2812B RGB halo ring around the neck turntable gives ambient emotional color feedback.
- Custom Power Stage: High efficiency Silergy SY8089 synchronous step-down buck converter stepping 5V down to a stable 3.3V logic rail with under 2mV ripple, paired with a 470uF low-ESR bulk reservoir capacitor and Schottky diode isolation so servo stall current spikes never drop the logic rail or reset the microcontroller.

## Project Structure

```text
Aura1/
  cad/
    aura_base_chassis_printready.stl      # Base chassis with PCB standoffs and inspection cutout
    aura_skull_head_printready.stl        # Head skull frame with eye recesses and ToF mount
    aura_servo_bracket_printready.stl     # Internal neck servo mounting plate with M2 screw bosses
    aura_turntable_bearing_printready.stl # 608ZZ bearing turntable with central wire pass-through
    aura_assembly.blend                   # Full Blender 4.2 parametric 3D CAD assembly
  firmware/
    platformio.ini                        # PlatformIO configuration for ESP32-S3
    include/
      servo_smooth.h                    # Cubic easing servo motion engine
      display_eyes.h                    # Dual GC9A01 60 FPS eye rendering and saccades
      kinematics.h                      # 3-DOF spherical pushrod kinematics math
      emotion_engine.h                  # State machine for idle, alert, and tracking behaviors
      petting_purr.h                    # Capacitive touch filtering and 25Hz purr vibration
      sensors.h                         # VL53L1X ToF, touch, and microphone polling
      halo_leds.h                       # 12-LED WS2812B RGB halo animations
      audio_dsp.h                       # INMP441 I2S microphone sampling and FFT
    src/
      main.cpp                          # Dual-core FreeRTOS main loop
  images/
    aura_day9_full_showcase.png           # Full 3/4 showcase render in happy resting pose
    aura_day9_petting_amber_glow.png      # Warm amber halo glow during petting reaction
    aura_day9_head_touchpad_closeup.png   # Close-up of head touch zone and happy eye displays
    aura_day9_turntable_assembly.png      # Technical view of 608ZZ bearing turntable
    aura_day9_purr_kinematics.png         # Frontal view showing neck tilt and purr flutter
    aura_day8_full_showcase.png           # Day 8 full showcase render
    aura_day8_eye_closeup.png             # Day 8 eye closeup
    aura_day8_internal_bracket.png        # Day 8 internal servo bracket
  .gitignore
  README.md
```

## Hardware and Components

1. Microcontroller: ESP32-S3-WROOM-1-N16R8 (Xtensa Dual-Core 240MHz, 16MB Flash, 8MB PSRAM)
2. Displays: 2x GC9A01 1.28 inch 240x240 Round IPS LCD Modules (SPI)
3. Actuators: 3x TowerPro MG90S Metal Gear Micro Servos (1x Pan, 2x Differential Tilt)
4. Bearing: 1x 608ZZ Radial Ball Bearing (8mm ID, 22mm OD, 7mm Height)
5. Linkages: 2x 2.0mm Carbon Fiber Pushrods with M2 Brass Ball Joints
6. Distance Sensor: ST VL53L1X Time-of-Flight Laser Sensor (I2C)
7. Audio: TDK InvenSense INMP441 Omnidirectional Digital I2S MEMS Microphone
8. Emotion Lighting: 12x WS2812B 5050 Addressable RGB LEDs
9. Power Converter: Silergy SY8089 2A 1.5MHz Synchronous Step-Down Buck Converter (5V to 3.3V)
10. Capacitors: 470uF 16V Low-ESR Bulk Electrolytic + 2x 22uF X5R Ceramic Smoothing
11. Hardware: M3 Brass Threaded Inserts, M3 Brass Hex Standoffs, M2 Mounting Screws

## Pinout and Wiring

ESP32-S3 GPIO mapping for the carrier board:

| Function | GPIO Pin | Bus / Protocol | Notes |
|---|---|---|---|
| Servo Pan | GPIO 4 | LEDC PWM Ch0 | Base horizontal turntable |
| Servo Tilt Left | GPIO 5 | LEDC PWM Ch1 | Left pushrod actuator |
| Servo Tilt Right | GPIO 6 | LEDC PWM Ch2 | Right pushrod actuator |
| Left Eye MOSI | GPIO 11 | SPI2 (FSPI) | Shared SPI data line |
| Left Eye SCLK | GPIO 12 | SPI2 (FSPI) | Shared SPI clock line |
| Left Eye CS | GPIO 13 | GPIO Out | Chip select left display |
| Right Eye CS | GPIO 14 | GPIO Out | Chip select right display |
| Display DC | GPIO 9 | GPIO Out | Shared Data/Command select |
| Display RST | GPIO 10 | GPIO Out | Shared Hardware reset |
| Display Backlight | GPIO 21 | LEDC PWM Ch3 | 2N7002 MOSFET brightness control |
| I2C SDA (Sensors) | GPIO 47 | I2C Bus | 4.7k pull-up to 3.3V |
| I2C SCL (Sensors) | GPIO 48 | I2C Bus | 4.7k pull-up to 3.3V |
| VL53L1X XSHUT | GPIO 2 | GPIO Out | Sensor hardware enable |
| VL53L1X INT | GPIO 18 | GPIO Interrupt | New measurement alert |
| I2S Mic BCLK | GPIO 15 | I2S Audio | Bit clock line |
| I2S Mic LRCLK | GPIO 16 | I2S Audio | Word select line |
| I2S Mic DIN | GPIO 7 | I2S Audio | Serial data from INMP441 |
| RGB Halo Ring | GPIO 38 | RMT Peripheral | WS2812B 800kHz single-wire data |
| Touch Head Petting | GPIO 1 | Capacitive Touch | Touch 1 sensing through skull |
| I2S Speaker BCLK | GPIO 48 | I2S Audio Out | Bit clock line to MAX98357A amplifier |
| I2S Speaker LRCLK | GPIO 47 | I2S Audio Out | Word select line to MAX98357A amplifier |
| I2S Speaker DIN | GPIO 21 | I2S Audio Out | Serial PCM audio data to MAX98357A amplifier |

## Voice and Acoustic Subsystem

AURA 1 features a fully procedural voice engine and dedicated hardware acoustic subsystem:

1. Acoustic Hardware Enclosure:
   - 28mm 4-ohm 3W micro-speaker mounted with a 20-degree forward tilt toward the desktop surface.
   - 9-slot radial acoustic sound grill cut into the lower front perimeter of the base chassis to project sound outward without muffling.
   - Enclosed resonance chamber baffle providing back-volume isolation to eliminate acoustic phase cancellation and preserve warm low-mid harmonics.
   - Dedicated chassis floor boss mounting for a MAX98357A I2S Class D mono amplifier breakout board right beside the main circular carrier PCB.

2. Procedural Speech Synthesis (firmware/include/voice_synth.h):
   - Generates responsive robotic vocalizations in real time directly on the ESP32-S3 using mathematical frequency sweeps, harmonic overtone mixing, and ADSR amplitude envelopes.
   - Sound categories:
     * Happy Chirp: Ascending Wall-E style cheerful arpeggio trill with 22Hz vibrato.
     * Curious Trill: Inquisitive rising inflection for when objects are detected nearby.
     * Purr Warble: Warm 25Hz amplitude-modulated harmonic drone synced with physical servo petting.
     * Startled Squeak: Fast exponential downward drop for sudden loud noises or table impacts.
     * Confirm Chime: Clean two-tone musical bell (C6 to G6) acknowledging voice input.
     * Sleep Sigh: Slow descending filtered sine breath when the robot drifts to sleep.
   - Zero SD-card overhead: All audio is synthesized algorithmically in RAM and streamed over I2S DMA buffers.

3. Audio-Reactive Expressions:
   - Real-time amplitude envelope tracking (`getAudioLevel()`) and pitch detection (`getPitchRatio()`).
   - The dual GC9A01 eye displays dynamically squash and lift on high vocal notes.
   - The 12-LED neck halo ring flares in brightness synchronously with robot vocal volume.

## 3D Printing

All main structural parts are in the cad directory and ready to slice:

1. Base Chassis (`aura_base_chassis_printready.stl`):
   - Material: PETG (recommended for strength around the bearing pocket)
   - Layer height: 0.20mm
   - Perimeters: 3 or 4 walls
   - Infill: 25% gyroid
   - Note: Features 9 precision acoustic sound slots on the lower front skirt, an internal 28mm speaker pocket with sealing gasket ridge, mounting standoffs for the MAX98357A amplifier, and 4x M3 PCB standoffs matching EasyEDA coordinates (64.0mm x 48.0mm).

2. Head Skull (`aura_skull_head_printready.stl`):
   - Material: Tough PLA or PETG (White)
   - Layer height: 0.16mm or 0.20mm
   - Infill: 20% gyroid
   - Note: Features curved 7.5mm bevels for an aerodynamic helmet profile with recessed mounting pockets for the dual GC9A01 displays.

3. Internal Servo Bracket (`aura_servo_bracket_printready.stl`):
   - Material: Tough PLA or PETG
   - Layer height: 0.16mm
   - Infill: 30% gyroid
   - Note: Features 3x pockets to secure MG90S servos rigidly inside the neck with M2 screw bosses and a central cable channel for display ribbon cables.

4. Turntable Bearing Mount (`aura_turntable_bearing_printready.stl`):
   - Material: PETG
   - Layer height: 0.16mm
   - Infill: 40% gyroid for rigid bearing retention
   - Note: Houses the central 608ZZ radial bearing with a 6.0mm hollow center channel for display and sensor wiring pass-through.

## Building and Flashing Firmware

1. Install Visual Studio Code and the PlatformIO IDE extension.
2. Clone this repository:
   ```bash
   git clone https://github.com/Arfat-Ansari/Aura1.git
   ```
3. Open the `firmware` folder inside Visual Studio Code.
4. Connect the ESP32-S3 over native USB.
5. Build and upload using PlatformIO:
   ```bash
   pio run --target upload
   ```
6. Open the serial monitor at 115200 baud to view system initialization:
   ```bash
   pio device monitor -b 115200
   ```

## Motion and Kinematics

The neck uses an inverted spherical 3-DOF gimbal. A central 608ZZ bearing handles all radial and axial forces so the servo horns never carry the physical weight of the head.

Instead of snapping directly to target angles, the firmware in `servo_smooth.h` runs cubic ease-in and ease-out interpolation at 50Hz. This keeps the angular velocity under 180 degrees per second with exponential damping, preventing servo jitter and making head turns look organic and lifelike.

## Author

Arfat Ansari
Project for Maker Faire Shenzhen 2026 and Hack Club Fallen.
