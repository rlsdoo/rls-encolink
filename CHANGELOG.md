# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed

- AksIM-2 encoders report EncoLink protocol 1, not 2, so `initialize()` always
  failed for AksIM-2. Replaced `ENCO_PROTOCOL_2` with `ENCO_PROTOCOL_1`;
  defining `ENCO_PROTOCOL_2` now stops the build with a hint to use
  `ENCO_PROTOCOL_1`.
- README documents which product and protocol defines belong together.

## [1.0.1] - 2026-08-03

Bugfixes applied.

### Fixed

- Fixed register reads returning stale OR'ed data when a buffer slot is reused.
- Fixed byte-to-word assembly casting so 4-byte register reads work on boards with
  16-bit int (Uno, Mega).

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


