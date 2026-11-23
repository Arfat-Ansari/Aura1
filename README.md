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
    aura_face_visor_printready.stl        # Magnetic front face visor cowling with 4x magnet pockets
    aura_skull_cap_printready.stl         # Upper skull dome cap with touch pad mount and rear vents
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
      gesture_engine.h                  # ST VL53L1X multi-zone optical gesture and peek-a-boo
      power_mgr.h                       # 18650 Li-ion battery ADC monitoring and power states
      behavior_engine.h                 # Autonomous desktop personality and sleep/wake engine
    src/
      main.cpp                          # Dual-core FreeRTOS main loop
  images/
    aura_day15_1_color_petting_purr_showcase.png # Full robot in happy petting state with amber glow
    aura_day15_2_color_skull_dome_rear_iso.png  # Color 3/4 rear isometric view of skull dome & vents
    aura_day15_3_color_head_complete_assembly.png # Beauty perspective shot of fully enclosed head
    aura_day15_4_design_skull_cap_wireframe.png # CAD top elevation & curvature analysis of skull dome
    aura_day15_5_design_touch_pad_cutaway.png   # CAD cutaway showing internal capacitive copper sensor
    aura_day15_6_design_head_snap_fit_iso.png   # CAD exploded view of 4x snap tabs and skull interface
    aura_day14_1_color_visor_showcase.png       # Full finished robot showcase with magnetic visor
    aura_day14_2_color_visor_detachment.png     # Perspective view showing magnetic snap pins and modularity
    aura_day14_3_color_deep_sleep_pose.png      # Color render of autonomous deep sleep pose with drooped head
    aura_day14_4_design_visor_front_wireframe.png # CAD front engineering layout of bezels and apertures
    aura_day14_5_design_magnet_pockets_iso.png  # CAD rear view of visor magnet pockets and locating pins
    aura_day14_6_design_exploded_head_assembly.png # CAD exploded view of skull, screens, magnets, and visor
    aura_day13_1_color_untethered_showcase.png  # Full untethered cordless robot showcase render
    aura_day13_2_color_usbc_rear_closeup.png    # Macro closeup of rear USB-C charging port and power switch
    aura_day13_3_color_low_battery_reaction.png # Color render of low-battery tired eye droop and amber warning
    aura_day13_4_design_battery_cradle_cutaway.png # CAD cutaway showing 18650 cell in cradle and TP4056
    aura_day13_5_design_power_stage_layout.png  # Top-down CAD engineering view of complete power stage layout
    aura_day13_6_design_rear_port_iso.png       # Isometric CAD view of rear chassis skirt and ports
    aura_day12_1_color_gesture_showcase.png     # Full robot interactive optical gesture showcase render
    aura_day12_2_color_optical_baffle_closeup.png # Macro closeup of forehead optical ToF sensor and septum
    aura_day12_3_design_optical_cutaway.png     # CAD cutaway showing internal VL53L1X board and gasket
    aura_day12_4_design_multizone_geometry.png  # Front engineering CAD view showing multi-zone optical geometry
    aura_day12_5_color_peekaboo_reaction.png    # Color render showing shy peek-a-boo interaction pose
    aura_day12_6_design_optical_module_iso.png  # CAD isometric engineering view of forehead optical module
    aura_day11_1_color_spatial_tracking.png     # Full showcase render with spatial sound tracking pose
    aura_day11_2_color_mic_port_closeup.png     # Macro closeup of stereo acoustic mic port on visor
    aura_day11_3_design_stereo_mic_cutaway.png  # CAD cutaway showing internal INMP441 board and gasket
    aura_day11_4_design_spatial_acoustic_baseline.png # Front engineering CAD view of 80mm acoustic baseline
    aura_day10_full_showcase.png          # Day 10 full robot showcase render with acoustic speaker grill
    aura_day10_speaker_closeup.png        # Day 10 macro view of 9-slot acoustic sound grill
    aura_day10_cad_acoustic_cavity.png    # Day 10 CAD cutaway showing internal 28mm speaker pocket
    aura_day10_cad_max98357a_mount.png    # Day 10 CAD view of MAX98357A amplifier floor standoffs
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
| I2S Mic SCK (Clock) | GPIO 40 | I2S Audio In | Shared bit clock for dual INMP441 mics |
| I2S Mic WS (L/R) | GPIO 38 | I2S Audio In | Shared word select line |
| I2S Mic SD (Data) | GPIO 39 | I2S Audio In | Time-multiplexed stereo I2S data in |
| Mic Left L/R Pin | GND | Hardware Tie | Configures left mic to left I2S slot |
| Mic Right L/R Pin | 3.3V | Hardware Tie | Configures right mic to right I2S slot |
| RGB Halo Ring | GPIO 48 | RMT Peripheral | WS2812B 800kHz single-wire data |
| Touch Head Petting | GPIO 1 | Capacitive Touch | Touch 1 sensing through skull |
| I2S Speaker BCLK | GPIO 42 | I2S Audio Out | Bit clock line to MAX98357A amplifier |
| I2S Speaker LRCLK | GPIO 41 | I2S Audio Out | Word select line to MAX98357A amplifier |
| I2S Speaker DIN | GPIO 21 | I2S Audio Out | Serial PCM audio data to MAX98357A amplifier |
| Battery Voltage Sense | GPIO 3 | ADC1_CH2 | 100k/100k precision voltage divider (2.0x ratio) |
| TP4056 Charge Status | GPIO 8 | GPIO In | Active-low charging indicator from TP4056 STAT |

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
   - Note: Features 9 precision acoustic sound slots on the lower front skirt, an internal 28mm speaker pocket with sealing gasket ridge, mounting standoffs for the MAX98357A amplifier, a horizontal 18650 Li-ion battery retaining cradle with snap-fit ribs, rear USB-C charging receptacle cutout, power slide switch pocket, and 4x M3 PCB standoffs matching EasyEDA coordinates (64.0mm x 48.0mm).

## Battery Power and Charging Subsystem

AURA 1 is completely untethered and cordless, powered by an internal rechargeable lithium-ion power stage:

1. 18650 Battery Cell and Retention:
   - Houses a standard 18650 3.7V 3000mAh cylindrical Li-ion cell seated horizontally across the center-rear floor of the base chassis.
   - The chassis floor features molded semi-circular cradle ribs and flexible snap-fit retention clips to secure the cell against high-velocity head pan inertia without rattling.
   - A dedicated rear skirt aperture accommodates a flush USB-C charging port and miniature SPDT slide power switch.

2. Charge Management and Power Architecture:
   - TP4056 1A constant-current / constant-voltage (CC/CV) lithium charge management IC with internal thermal regulation and automatic recharge threshold.
   - Powers the high-efficiency Silergy SY8089 synchronous step-down buck converter to step battery voltage (3.2V - 4.2V) to a stable 3.3V logic rail.
   - Dedicated 470uF low-ESR bulk reservoir capacitor absorbs dynamic current spikes drawn by the three metal-gear servos so the ESP32-S3 microcontroller never experiences brownout resets.

3. Voltage Sensing and Low-Battery Protection (firmware/include/power_mgr.h):
   - 100k / 100k precision 1% resistor voltage divider connects the battery rail to ESP32-S3 ADC1_CH2 (GPIO 3).
   - A 50Hz rolling exponential moving average filter (alpha = 0.05) filters out high-frequency servo noise and transient voltage sags.
   - Multi-segment linear interpolation calculates battery state-of-charge percentage (100% at 4.20V down to 0% at 3.25V).
   - State machine triggers low-power behaviors:
     * Normal State (> 3.42V): Full interactive animation, vibrant cyan neck halo.
     * Low Battery State (3.25V - 3.42V): Robot enters tired mood (`MOOD_LOW_BATTERY`), cocks head down, eyes droop with sleepy heavy squinting, and neck halo pulses a gentle amber warning.
     * Critical Cutoff (< 3.25V): Disables servo PWM signals, saves perception state, and enters deep sleep to prevent battery over-discharge damage.

2. Head Skull (`aura_skull_head_printready.stl`):
   - Material: Tough PLA or PETG (White)
   - Layer height: 0.16mm or 0.20mm
   - Infill: 20% gyroid
   - Note: Features curved 7.5mm bevels for an aerodynamic helmet profile with recessed mounting pockets for the dual GC9A01 displays, twin 1.8mm acoustic microphone ports on the visor temples at an 80mm baseline, internal silicone vibration damping gasket seats, and mounting slots for dual INMP441 MEMS sensor boards.

## Spatial Hearing and Directional Sound Localization

AURA 1 uses a stereo pair of TDK InvenSense INMP441 omnidirectional I2S digital MEMS microphones to track sounds and orient its head toward whoever is speaking:

1. Acoustic Baseline and Mechanical Isolation:
   - Microphones are spaced 80mm apart across the visor temples (X = -40.0mm and X = +40.0mm at eye height Z = 84.0mm).
   - Each microphone communicates with the outside world through a 1.8mm acoustic aperture pinhole surrounded by a protective bezel ring.
   - 2.5mm silicone vibration decoupling gaskets sit between the INMP441 PCB and the 3D-printed skull wall, isolating the MEMS sensing element from mechanical servo vibration transmitted through the neck frame.

2. Digital Signal Processing (firmware/include/audio_dsp.h):
   - Both microphones share the I2S clock lines (SCK on GPIO 40, WS on GPIO 38) and time-multiplex their 24-bit audio data onto a single shared SD line (GPIO 39).
   - An adaptive noise floor tracker continuously learns ambient background room noise using an exponential decay filter (decay factor 0.995).
   - Real-time Interaural Level Difference (ILD) calculation compares the root-mean-square energy between left and right channels:
     `ILD = (EnergyRight - EnergyLeft) / (EnergyRight + EnergyLeft)`
   - Computes sound arrival azimuth from -90 degrees (full left) to +90 degrees (full right).
   - A 0.14 ILD deadband threshold filters out ambient room reflections and servo motor hum so the head does not jitter unnecessarily.

3. Auditory Gaze Tracking (firmware/include/emotion_engine.h):
   - When a sudden sound, voice, or clap exceeds the noise floor threshold, the robot transitions to listening mode (`MOOD_LISTENING`).
   - The neck pan servo rotates smoothly to center the robot on the sound source.
   - The head cocks with an inquisitive roll angle (8.5 degrees tilt toward the sound origin).
   - The dual GC9A01 pupils dart toward the speaker (+-2.5mm saccadic pupil translation) before the neck finishes turning, replicating natural human auditory gaze reflexes.

## Optical Gesture Recognition and Multi-Zone ToF Interaction

AURA 1 uses the multi-zone Region of Interest (ROI) capabilities of the ST VL53L1X time-of-flight laser sensor on its forehead to achieve optical swipe and proximity gesture recognition without requiring an onboard camera:

1. Anti-Crosstalk Optical Baffle Hardware:
   - In real-world ToF ranging, infrared light from the 940nm VCSEL laser emitter can scatter through front cover materials directly into the SPAD receiver array, producing severe distance errors (optical cross-talk).
   - A dedicated 0.8mm vertical dividing septum is molded between the Tx and Rx optical channels at the forehead aperture, providing total physical photon isolation.
   - Chamfered conical aperture bezels (27-degree FOV matching the sensor cone) prevent internal chassis reflections.
   - An internal black silicone optical gasket seals the VL53L1X module against the inner skull wall.

2. Multi-Zone Spatial Scanning (firmware/include/gesture_engine.h):
   - The 16x16 SPAD array is dynamically re-addressed at 50Hz, alternating between a Left zone (center SPAD 199, 8x16 matrix) and a Right zone (center SPAD 231, 8x16 matrix).
   - By tracking temporal entry deltas between the two zones (40ms to 450ms window), the firmware identifies horizontal hand swipe directions:
     * Left-to-right swipe (`GESTURE_SWIPE_RIGHT`): Robot follows the user hand motion with a smooth pan right and curious double-chirp.
     * Right-to-left swipe (`GESTURE_SWIPE_LEFT`): Robot smoothly turns its neck left to follow the gesture.

3. Interactive Peek-a-boo Game:
   - When a user puts their hand over the robot eyes and forehead (< 60mm distance across both zones for > 250ms), the robot enters peek-a-boo cover state (`GESTURE_PEEKABOO_COVER`). The head lowers slightly and the dual GC9A01 eyes squint closed with a quiet inquisitive trill.
   - When the user suddenly pulls their hand away (`GESTURE_PEEKABOO_REVEAL`), the robot springs up (+12 degrees pitch), pupils dilate wide in excitement, the 12-LED neck halo pulses bright violet, and a celebratory Wall-E style chirp plays.

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

5. Magnetic Face Visor Cowling (`aura_face_visor_printready.stl`):
   - Material: Matte Dark PETG or Tough PLA
   - Layer height: 0.16mm
   - Infill: 30% gyroid
   - Note: Features 4x internal neodymium magnet pockets (3.0mm diameter x 1.5mm depth) for tool-less snap-fit removal, twin 26.5mm circular eye bezels with 45-degree chamfers, VL53L1X laser aperture slot, and dual microphone acoustic horn entry ports.

6. Upper Skull Dome Cap (`aura_skull_cap_printready.stl`):
   - Material: Tough White PLA or PETG
   - Layer height: 0.16mm
   - Infill: 25% gyroid
   - Note: Encloses the top crown and rear of the head with 4x internal snap-fit tabs, internal 42x34mm capacitive touch foil mounting pocket, and rear horizontal ventilation slots.

## Bill of Materials and Real-World Manufacturing Budget

AURA 1 is designed to be affordable to build for high-school students, makers, and hobbyists. Below is the itemized cost breakdown comparing in-house 3D printing against ordering parts from commercial 3D printing service bureaus in India.

### 1. 3D Printing Enclosure and Gimbals (5 Custom Parts)
- Total net model mass: ~182g (~220g including supports, brim, and purge).
- Path A - Commercial Online 3D Printing Service Bureau in India (Robu 3D / Think3D / local bureaus):
  * Material rate: Rs 7.50 to Rs 9.50 per gram for Tough PLA / PETG (~Rs 1,700 base).
  * 18% GST and tracked courier delivery included.
  * Turnkey commercial 3D printing cost: Rs 1,600 to Rs 2,100 INR (~$22 to $25 USD).
- Path B - Self-Printed at Home or Maker Lab:
  * Consumes ~220g from a 1kg spool of Tough PLA / PETG (Rs 1,150 per spool).
  * Pro-rated material cost: Rs 260 to Rs 320 INR (~$3.50 USD).

### 2. Itemized Electronics and Hardware Sourcing (Retail India)
- Compute and Displays: ESP32-S3 DevKit N16R8 (Rs 650) + 2x GC9A01 1.28-inch circular IPS displays (Rs 988) = Rs 1,638 INR ($19.55 USD).
- Motion and Kinematics: 3x TowerPro MG90S metal gear micro servos (Rs 780) + 608ZZ precision bearing (Rs 73) + M2 brass ball linkage tie rods (Rs 240) = Rs 1,093 INR ($13.00 USD).
- Sensors and Perception: ST VL53L1X time-of-flight laser ranging module (Rs 589) + 2x INMP441 I2S MEMS microphones (Rs 280) + copper touch foil electrode (Rs 60) = Rs 929 INR ($11.05 USD).
- Audio and Halo Lighting: 28mm 4-ohm 2W neodymium acoustic speaker (Rs 95) + MAX98357A I2S 3.2W amplifier (Rs 289) + WS2812B 12-LED RGB ring (Rs 110) = Rs 494 INR ($5.90 USD).
- Battery and Power Subsystem: 18650 3.7V Li-ion cell (Rs 260) + TP4056 Type-C charger (Rs 25) + mini SPDT slide switch (Rs 15) + mini 3.3V 2A buck regulator (Rs 175) + 470uF reservoir capacitor and divider resistors (Rs 45) = Rs 520 INR ($6.23 USD).
- Fasteners, Magnets, and Wiring: 8x N52 neodymium disc magnets (Rs 90) + M2/M3 stainless screws and brass heat-set inserts (Rs 180) + 30 AWG silicone wire and JST connectors (Rs 120) = Rs 390 INR ($4.68 USD).
- Total Hardware and Electronics: Rs 5,064 INR ($60.40 USD).

### 3. Total Project Cost Summary
- Self-Printed (Pro-rated material): Rs 5,490 INR (~$65.50 USD).
- Commercial 3D Printing Service Bureau (Turnkey India): Rs 7,064 INR (~$84.50 USD).

## Magnetic Face Visor and Cosmetic Cowling

A key aesthetic and functional milestone of AURA 1 is the modular front faceplate:

1. Tool-Less Magnetic Snap Retention:
   - The front face visor (`aura_face_visor_printready.stl`) mounts using four miniature nickel-plated N52 neodymium disc magnets (3mm diameter x 1.5mm thickness) pressed into matching rear cavities on the visor and front bosses on the skull frame.
   - Two tapered self-centering locating guide pins on the rear face eliminate rotational shear and ensure instant alignment without tools or exposed screw heads.

2. Optical Masking and Acoustic Horn Ports:
   - The GC9A01 circular IPS display modules use rectangular PCB carrier boards with exposed mounting ears. The visor provides 26.5mm circular openings with 45-degree inward chamfered bezels, seamlessly hiding the board corners while leaving the full active circular display area exposed.
   - A 16mm x 7.5mm pill-slot cutout aligns with the forehead VL53L1X ToF laser sensor and its anti-crosstalk septum.
   - Two 1.8mm acoustic horn ports with 45-degree sound entry chamfers route external audio cleanly to the internal INMP441 MEMS microphones.

## Autonomous Desktop Companion Behavior Engine

In addition to interactive reactive states, AURA 1 implements an autonomous desktop personality and tiered sleep cycle (`firmware/include/behavior_engine.h`):

1. Ambient Curiosity and Fidgets (`BEHAVIOR_IDLE_EXPLORE` / `BEHAVIOR_FIDGET`):
   - When no direct user input is detected, the robot executes organic micro-saccades, glancing across the desk, stretching its neck in subtle pan/pitch offsets (-14 to +14 degrees), and observing its environment.

2. Tiered Sleep Cycle (`BEHAVIOR_DROWSY` / `BEHAVIOR_DEEP_SLEEP`):
   - After 2 minutes of idle inactivity, the robot transitions into drowsy mode: eyelids droop by 50-80%, the head sags downward (-6 to -14 degrees), and the neck halo dims into a soft indigo glow.
   - After 4 minutes of inactivity, the robot enters deep sleep: eye screens display restful shut curves, head rests in full low posture, and the 12-LED neck halo pulses in a slow 0.1Hz sinusoidal cyan breathing wave.

3. Startled Wake Reflex (`BEHAVIOR_STARTLED_WAKE`):
   - High-energy audio transients (desk knock, clap, loud speech via INMP441) or sudden proximity deltas (> 180mm sudden hand approach via VL53L1X) instantly wake the robot.
   - The head snaps up (+12 degrees pitch) within 15ms, eyes dilate wide (1.35x), the halo flashes alert gold, and an audible wake chime sounds before settling into active engagement.

## Upper Skull Dome and Capacitive Touch Petting Subsystem

AURA 1 features an organic top skull dome shell that fully encloses the head while providing interactive tactile petting sensing (`firmware/include/petting_purr.h`):

1. Curved Head Enclosure (`aura_skull_cap_printready.stl`):
   - The dome mates seamlessly with the front magnetic face visor and skull frame, completing the head enclosure without visible external hardware.
   - Four internal snap-fit tabs lock into matching detents on the skull frame rim for tool-free assembly and maintenance.
   - Three rear horizontal ventilation slots (32mm x 2mm) provide thermal convective exhaust for the internal ESP32-S3 and GC9A01 LCD driver circuits.

2. Capacitive Touch Sensing and Baseline Calibration:
   - A 42mm x 34mm copper foil electrode is secured inside a recessed mounting pocket directly beneath the 2.2mm top skull dome wall, connected to ESP32-S3 Touch Pad GPIO 1.
   - Startup calibration takes 25 averaged readings to establish ambient baseline capacitance, with an adaptive low-pass filter (alpha = 0.22) and slow baseline tracking (0.003 adaptation rate) to compensate for ambient humidity and temperature changes.

3. Haptic Purr Flutter and Expressive Reactions:
   - When a human hand strokes the top of the skull dome, the touch reading drops by > 28%, triggering the petting engine.
   - The neck pitch and roll servos execute a 25Hz micro-dither flutter that ramps smoothly from +/-0.65 degrees up to +/-1.85 degrees over 1.5 seconds of sustained petting, creating a physical vibration that feels like a cat purring beneath the user hand.
   - The dual GC9A01 circular screens animate into happy anime squint arches, the 12-LED neck halo pulses warm golden-amber, and the I2S sound synthesizer emits a gentle warbling purr.

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
