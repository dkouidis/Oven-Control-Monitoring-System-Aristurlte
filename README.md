The project is split into two primary hardware and firmware configurations based on oven scale and thermal application:
1. oven_thermistors (Chassis Baking Oven):Purpose: Used for thermal curing and post-curing cycles of the team's aluminium chassis.   
Focus: Multi-point temperature monitoring and uniform thermal gradient verification across large structural surfaces using distributed thermistors.

2. little_oven (Motor Development Heating Oven):Purpose: Dedicated thermal chamber used during prototyping and component fitting of custom electric powertrain motors.   Focus: Localized heating control, thermal soak routines, and rapid feedback loop execution.


Requirements:
Arduino IDE (version 1.8.x or 2.x) or PlatformIO

ESP32 Arduino Core: Install via Boards Manager: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json