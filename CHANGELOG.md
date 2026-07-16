# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-07-16

First public release.

### Added

- Platform-independent C++ implementation of the EncoLink protocol
  (`Encolink` class with a user-supplied transport callback).
- Support for AksIM-4 (protocol 4) and AksIM-2 (protocol 2) encoders,
  selectable via `ENCO_PRODUCT_*` and `ENCO_PROTOCOL_*` defines;
  AksIM-4 with protocol 4 is the default when nothing is defined.
- Register access API with a 4-slot command queue running concurrently
  with cyclic position reads.
- Register address/size maps for AksIM-4 and AksIM-2 in
  `inc/RegisterAccess/`.
- Arduino library packaging (`library.properties`) with four example
  sketches: `read_register`, `read_register_error_codes`,
  `write_read_register`, and `write_register_param_outside_range`.
- Integration documentation for Arduino and non-Arduino (e.g. STM32 HAL)
  targets.
