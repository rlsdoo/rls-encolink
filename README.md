# EncoLink C++ Library

Platform-independent C++ implementation of the EncoLink protocol with product and protocol configuration for RLS encoders.

Arduino-specific setup, wiring, and example workflows are documented in [examples/README.md](examples/README.md).

## Repository Layout

- `inc/encolink.h`: Core EncoLink API and protocol state machines.
- `inc/protocol_config.h`: Protocol version selection (`ENCO_PROTOCOL_*`).
- `inc/RegisterAccess/product_config.h`: Product selection (`ENCO_PRODUCT_*`).
- `inc/RegisterAccess/product_aksim4_map.h`: AksIM-4 register address/size macros.
- `inc/RegisterAccess/product_aksim2_map.h`: AksIM-2 register address/size macros.
- `src/encolink.cpp`: Core implementation; `src/encolink.h` is a shim that forwards to `inc/encolink.h` so the Arduino build (which only sees `src/`) finds the headers.
- `examples/`: Arduino-focused examples and usage notes.

## Documentation Split

- `README.md`: Core library architecture and platform-independent integration.
- `examples/README.md`: Arduino setup, wiring, runtime flow, and example behavior.

## Integration Overview (Non-Arduino)

1. Add `inc` folder as project include
2. Implement the transport callback (`Read_write_fun`) used by `Encolink`. Note that function must return 0 upon success and positive non-zero value upon failure. 
3. Select the product and protocol version as preprocessor define symbols (`-D` flag) or by editing the defaults in the config headers (`protocol_config.h` and `product_config.h`). When nothing is defined, AksIM-4 with protocol 4 is used.
4. Create an `Encolink` instance with your callback.
5. Run `initialize()` and then `reset_encoder()` before regular operation.
6. Call `read_encoder()` cyclically.

## SPI configuration
Make sure to configure SPI as MSB first, CPOL = 0 (low), CPHA = 1 (2 edge)


Example transport wiring for STM HAL library:

```cpp
#include "encolink.h"


uint32_t st;
uint16_t mt;

uint8_t encolink_rx_tx(uint8_t * data, uint8_t rx_bytes, uint8_t tx_bytes)
{
    HAL_StatusTypeDef ret;

    if (data == NULL)
    {
        return 1;
    }

    ENCOLINK_CS_LOW();

	ret = HAL_SPI_TransmitReceive(&hspi2, data, data, rx_bytes, HAL_MAX_DELAY);
	if (ret != HAL_OK)
	{
		ENCOLINK_CS_HIGH();
		return 1;
	}

    ENCOLINK_CS_HIGH();

    return 0;
}

int main(void)
{
	Encolink encoder(encolink_rx_tx);

	HAL_Init();

	SystemClock_Config();

	MX_GPIO_Init();
	MX_SPI2_Init();

	if (!encoder.initialize())
	{
		while(1); // NOTE: transmission error occured
	}

	encoder.reset_encoder();

	while (1)
	{
		encoder.read_encoder();
		st = encoder.data.singleturn_position;
		mt = encoder.data.multiturn_position;

		HAL_Delay(100);
	}
}

```

## Core API

- `initialize()`
- `read_encoder_ident()`
- `reset_encoder()`
- `read_encoder()`
- `read_register_command(uint16_t num_of_bytes, uint32_t address)`
- `get_read_register_data(uint8_t id, uint32_t &read_data)`
- `write_register_command(uint16_t num_of_bytes, uint32_t address, uint32_t data)`
- `get_write_register_status(uint8_t id)`
- `statusString(uint8_t status)`

## Register Access Notes

- Register access uses a 4-slot queue and runs concurrently with position reads.
- Command APIs return a buffer ID (0-3) or `0xFF` when all slots are busy.
- Poll read/write completion through `get_read_register_data()` or `get_write_register_status()`.

## Register Access Status Codes

| Value | Description |
|-------|-------------|
| `0xFF` | In progress (data not ready) |
| `0x09` | Success |
| `0x26` | Invalid parameter ID |
| `0x56` | Parameter outside valid range (write) |
| `0x96` | Access denied (write) |
| `0xEE` | Insufficient number of bytes |
| `0xF6` | Configuration locked (write) |
| `0xF9` | Write CRC invalid (write) |
| `0xE4` | Read CRC invalid (read) |

## Detailed Status Bits (Condensed)

`data.latched_detailed_status` is a latched bitfield reported by the encoder.

| Bit | Meaning |
|-----|---------|
| b15 | Multiturn counter mismatch |
| b14-b13 | Signal amplitude too high (error/warning) |
| b12 | Magnetic sensor error |
| b11 | Sensor reading error |
| b10 | Encoder configuration error |
| b9 | Position invalid error |
| b8 | Warning: near operational limits |
| b7-b6 | Signal amplitude warning (high/low) |
| b5 | Signal lost |
| b4 | Temperature warning |
| b3 | Power supply error |
| b2 | System error |
| b1 | Magnetic pattern error |
| b0 | Acceleration error |

## Register Access References

For full register catalog details, use the product register map headers in `inc/RegisterAccess/` and the official application notes.

Commonly used examples in this repository:

| Register | Address | Bytes | Typical usage |
|----------|---------|-------|---------------|
| `POS_OFFSET` | `0x0000` (AksIM-2), `0x0080` (AksIM-4) | 4 | Read/write offset parameter |
| `TEMPERATURE` | `0x004C` (AksIM-2), `0x0000` (AksIM-4) | 2 | Read-only temperature value |
| `COMMAND` | `0x0049` (AksIM-2), `0x00BD` (AksIM-4) | 1 | Device command register |

Common `COMMAND` register values (protocol/application dependent):

- `0x63`: Save configuration parameters
- `0x72`: Reset configuration parameters
- `0x41`: Start self-calibration

For Arduino-specific register-access flows and runnable examples, see [examples/README.md](examples/README.md).

## External Protocol Documentation

- [EncoLink communication protocol - AksIM-2 (MBD08)](https://www.rls.si/cn/fileuploader/download/download/?d=0&id=311&title=Application+note%3A+Programming+AksIM-2+encoders+with+EncoLink+communication+protocol+%28MBD08%29)
- [AksIM-4 EncoLink Register Access (APP04)](https://www.rls.si/eng/fileuploader/download/download/?d=1&id=497&title=Application+note%3A+AksIM-4+EncoLink+Register+Access+%28APP04%29)

## License

Copyright (C) 2026 RLS d.o.o, Pod vrbami 2, 1218 Komenda, Slovenia.

Released under Apache License 2.0. See [LICENSE](LICENSE).


