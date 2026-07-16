# Arduino Examples Guide

This document contains Arduino-specific setup and example workflows for this repository.

For core (platform-independent) library documentation, see [../README.md](../README.md).

## Supported Boards

- Arduino Due
- Arduino Mega 2560

## Prerequisites

- Arduino IDE installed
- This library installed in Arduino IDE
- AksIM encoder connected over SPI

## Install Library In Arduino IDE

1. Download this repository as a ZIP archive.
2. In Arduino IDE, select: `Sketch -> Include Library -> Add .ZIP Library...`
3. Select the ZIP file.
4. Restart Arduino IDE.
5. Open examples under: `File -> Examples -> EncoLink`

## Basic SPI Wiring (Reference)

| Encoder IO | Arduino IO | Signal |
|------------|------------|--------|
| Pin 1 | SPI header 5 V | 5 V supply |
| Pin 2 | SPI header GND | GND |
| Pin 5 | SPI header SCK | SCK |
| Pin 6 | D10 | NCS (chip select) |
| Pin 7 | SPI header MISO | MISO |
| Pin 8 | SPI header MOSI | MOSI |

Always verify exact encoder pinout from the official encoder datasheet.

## Example Projects

| Example | Source | What it demonstrates |
|---------|--------|----------------------|
| read_register | [read_register/read_register.ino](read_register/read_register.ino) | Continuous position read plus register read (`TEMPERATURE`) |
| read_register_error_codes | [read_register_error_codes/read_register_error_codes.ino](read_register_error_codes/read_register_error_codes.ino) | Read-access error handling and status reporting |
| write_read_register | [write_read_register/write_read_register.ino](write_read_register/write_read_register.ino) | Write register and read it back (`POS_OFFSET`) |
| write_register_param_outside_range | [write_register_param_outside_range/write_register_param_outside_range.ino](write_register_param_outside_range/write_register_param_outside_range.ino) | Write out-of-range value and inspect error response |

## Common Runtime Workflow

All examples follow this pattern:

1. Initialize SPI peripheral and serial output.
2. Construct `Encolink` with the board-specific read/write callback.
3. Run initialization loop until `initialize()` succeeds.
4. Call `reset_encoder()` once.
5. In `loop()`, continuously call `read_encoder()`.
6. Issue register access commands when trigger conditions are met.
7. Poll status APIs until result is ready (`status != 0xFF`).

## Register and Status Reference

Register-access status codes, condensed detailed-status bits, and register references are documented in [../README.md](../README.md).

## Troubleshooting

- If initialization does not complete, verify SPI mode, pin mapping, and power.
- If register operations never complete, ensure `read_encoder()` is called continuously.
- If every register command returns `0xFF`, wait until a queue slot is free and retry.
- If status reports errors (for example `0x26`, `0x56`), validate register macro and payload size.
