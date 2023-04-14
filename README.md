# astronode-arduino-library
Arduino Library for the Astronode S.

[![Lint library](https://github.com/Astrocast/astronode-arduino-library/actions/workflows/lint_library.yml/badge.svg)](https://github.com/Astrocast/astronode-arduino-library/actions/workflows/lint_library.yml)
[![Build examples](https://github.com/Astrocast/astronode-arduino-library/actions/workflows/build_examples.yml/badge.svg)](https://github.com/Astrocast/astronode-arduino-library/actions/workflows/build_examples.yml)

# Examples

Use the examples below as reference designs to develop your own application using the astronode-arduino-library.

## hello_Astronode
This sketch enqueues a single message in Astronode S queue. It checks periodically if a new event is available. If a "satellite acknowledge" event is received, a new message is enqueued in the module.

### Hardware setup
An [Astronode S devkit](https://docs.astrocast.com/docs/products/astronode-devkit/product-brief) (sat or wifi) is required. Additionally, you will need one of these boards:
- [Nucleo-64 STM32l476](https://www.st.com/en/evaluation-tools/nucleo-l476rg.html) (TX -> D2(PA10), RX -> D8(PA9), GND -> GND, 3V3 -> 3V3)
- [Arduino MKR1400](https://docs.arduino.cc/hardware/mkr-gsm-1400) (TX -> D13(RX), RX -> D14 (TX), GND -> GND, 3V3 -> VCC)
- [Arduino UNO](https://docs.arduino.cc/hardware/uno-rev3) (TX -> D2(with level shifter), RX -> D3(with level shifter), GND -> GND, 3V3 -> VCC)

## hk_logger_sd
This sketch periodically logs all housekeeping from the module on an SD card (peformance counters, env. variables, etc.):
- It enqueues a single message in the queue of the Astronode S.
- It checks periodically if a new event is available.
- If a "satellite acknowledge" event is receieved, the ACK is cleared and a new message is enqueued in the module.
- If a "command received" event is receieved, the data contained in the command are written to the SD card and the command is cleared.
- When all tasks are done, the arduino goes in deep sleep.

### Hardware setup
- [Adafruit Feather M0 Adalogger](https://www.adafruit.com/product/2796) (TX -> D16(RX), RX -> D15 (TX), GND -> GND, 3V3 -> 3V3)
- [Astronode S devkit](https://docs.astrocast.com/docs/products/astronode-devkit/product-brief) (sat or wifi)

### Software tools
- plot_sdlogger.py: plot the satellite passes from the SD CSV data
