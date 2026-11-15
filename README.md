# AURA 1 - Autonomous Desktop Companion Robot

Author: Arfat Ansari
Target: Maker Faire Shenzhen 2026 / Hack Club Fallen

This is AURA 1, my autonomous companion robot project. I am building this because I wanted an animatronic companion sitting on my desk that actually feels responsive and alive when I am working or talking, instead of just sitting there like a static piece of plastic.

Most commercial desktop robots either cost hundreds of dollars or their neck movements look so jerky and robotic like a toy servo snapping around. My goal with AURA 1 is to build an open hardware companion robot with smooth organic motion, expressive circular color screens, real time laser distance sensing, and sound reactivity, all running on an ESP32-S3 microcontroller.

## What It Does

- 3-DOF Kinematic Head: The head moves in 3 axes (pan left/right, pitch up/down, roll tilt sideways) driven by three TowerPro MG90S metal gear servos linked with carbon fiber pushrods and brass ball joints on a central 608ZZ bearing turntable.
- Dual Round Color Eyes: Two GC9A01 1.28 inch 240x240 circular IPS LCD screens running at 60 FPS over dual SPI channels. The eyes have animated pupils that dilate and track movement.
- Laser Distance Perception: An ST VL53L1X time-of-flight laser distance sensor on the forehead reads ranges up to 4 meters, so the robot knows when someone is sitting in front of it and turns to look at them.
- Sound Reactive: An INMP441 I2S digital MEMS microphone streams audio over DMA with real time FFT beat detection, allowing the robot to nod or react to music and voices.
- Emotion Halo: A 12-LED WS2812B RGB halo ring around the neck turntable gives ambient emotional color feedback.
- Custom Power Stage: High efficiency Silergy SY8089 synchronous step-down buck converter stepping 5V down to a stable 3.3V logic rail with under 2mV ripple, paired with a 470uF low-ESR bulk reservoir capacitor and Schottky diode isolation so servo stall current spikes never drop the logic rail or reset the microcontroller.

## Project Structure

```text
Aura1/
  cad/
    aura_base_chassis_printready.stl   # Base chassis with PCB standoffs and inspection cutout
    aura_skull_head_printready.stl     # Head skull frame with eye recesses and ToF mount
    aura_assembly.blend                # Full Blender 4.2 parametric 3D CAD assembly
  firmware/
    platformio.ini                     # PlatformIO configuration for ESP32-S3
    include/
      servo_smooth.h                 # Cubic easing servo motion engine
      display_eyes.h                 # Dual GC9A01 60 FPS eye rendering
      kinematics.h                   # 3-DOF spherical pushrod kinematics math
      emotion_engine.h               # State machine for idle, alert, and tracking behaviors
      sensors.h                      # VL53L1X ToF and capacitive touch polling
      halo_leds.h                    # 12-LED WS2812B RGB halo animations
      audio_dsp.h                    # INMP441 I2S microphone sampling and FFT
    src/
      main.cpp                       # Dual-core FreeRTOS main loop
  images/
    aura_showcase_full.png             # Full 3/4 CAD showcase render
    aura_head_closeup.png              # Head and eye display close-up
    aura_base_curvature.png            # Smooth 128-segment circular base close-up
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

## 3D Printing

Both main body files are in the cad directory and ready to slice:

1. Base Chassis (`aura_base_chassis_printready.stl`):
   - Material: PETG (recommended for strength around the bearing pocket)
   - Layer height: 0.20mm
   - Perimeters: 3 or 4 walls
   - Infill: 25% gyroid
   - Note: The circular base was rebuilt with 128 segments and curved fillets on the rims so it prints completely smooth without vertical square ridges or flat facet lines.

2. Head Skull (`aura_skull_head_printready.stl`):
   - Material: Tough PLA or PETG (White)
   - Layer height: 0.16mm or 0.20mm
   - Infill: 20% gyroid
   - Note: Features curved 7.5mm bevels for an aerodynamic helmet profile with recessed mounting pockets for the dual GC9A01 displays.

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
