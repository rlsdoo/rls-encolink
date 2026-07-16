/**
 * @file    write_register_param_outside_range.ino
 * @brief   Demonstrate out-of-range write error handling for `POS_OFFSET`.
 * @details Sequence:
 *          1) Initialize SPI and EncoLink.
 *          2) Repeatedly call read_encoder() for cyclic position updates.
 *          3) Trigger write_register_command(POS_OFFSET, value_outside_range).
 *          4) Poll get_write_register_status() and capture returned error.
 *          5) Optionally read back POS_OFFSET and print status/result.
 * @copyright Copyright (c) 2026 RLS d.o.o, Pod vrbami 2, 1218 Komenda, Slovenia
 * @license SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>
#include <SPI.h>
#include <encolink.h>

uint8_t ChipSelectPin = 10;
uint8_t my_read_write_frame(uint8_t *data, uint8_t rx_bytes, uint8_t tx_bytes);
void SPI_init();
// Create object EncoLink.
Encolink enco = Encolink(my_read_write_frame);

uint32_t       register_data       = 0;
uint8_t        ra_write_status	   = DATA_NOT_READY;
uint8_t        ra_read_status	   = DATA_NOT_READY;
uint8_t        ra_buffer		   = INVALID_ID;
bool           ra_trigger_read	   = false;
bool           ra_trigger_write	   = true;
uint8_t        crc_error		   = 0;
uint32_t       value_outside_range = 0xFFFFFFFF;

void setup()
{

    SPI_init();
	// Initialize serial communication (USB).
	Serial.begin(9600);
	
	// Initialize encoder
	while (!enco.initialize())
	{
		Serial.println("Encoder initialization not successful. ");
		delay(50);
		
	}
	Serial.println("Communication with EncoLink encoder established");
	
	// Reset encoder.
	enco.reset_encoder();
}						
							
void loop()					
{							
	// Use while loop inside loop for faster program.
	while (1)
	{
		//Read encoder command.
		crc_error = enco.read_encoder();
		Serial.print("Singleturn position: ");
		Serial.print(enco.data.singleturn_position);


		// Print warning message if CRC error happens.
		if (1 == crc_error)
		{
			Serial.print(" CRC Error !");
		}

		//A small delay for easier reading of COM port (User can remove this). 
		delay(50);

		//Write position offset command.
		if (true == ra_trigger_write) 		//Write position offset command is set to true after read process is finished (enco.get_read_register_data))
		{
			ra_buffer = enco.write_register_command(POS_OFFSET, value_outside_range); 
			ra_trigger_write = false;
			Serial.print(" Start of write register procedure");
		}
		//Read temperature command.
		if (true == ra_trigger_read) 		//ra_trigger_read is set to true when read process is finished (enco.get_read_register_data))
		{
			ra_buffer = enco.read_register_command(POS_OFFSET);
			ra_trigger_read = false;
			Serial.print(" Start of read register procedure");
		}

		// Check if write process is finished.
		ra_write_status = enco.get_write_register_status(ra_buffer);

		// Check if read process is finished.
		ra_read_status = enco.get_read_register_data(ra_buffer, register_data);


		// Print RA write status if write process has finished.
		if (DATA_NOT_READY != ra_write_status)
		{
			Serial.print("	Write register status:  ");
			Serial.print(enco.statusString(ra_write_status));
			ra_trigger_read = true;
		}

		// Print RA register value and read status is read process has finished.
		else if (DATA_NOT_READY != ra_read_status)
		{
			Serial.print("	Position offset: ");
			Serial.print(register_data);
			Serial.print("	Read register status:  ");
			Serial.print(enco.statusString(ra_read_status));
			ra_trigger_write = true;
		}

		// New line
		Serial.println();

	}
}
// Function responsible for data exchange with the encoder, at successful read/write it returns 0
uint8_t my_read_write_frame(uint8_t *data, uint8_t rx_bytes, uint8_t tx_bytes)
{
    SPI.beginTransaction(SPISettings(300000, MSBFIRST, SPI_MODE1));
    digitalWrite(ChipSelectPin, LOW);

    // SPI CS delay setting
    delayMicroseconds(5);

    SPI.transfer(data, rx_bytes);
    digitalWrite(ChipSelectPin, HIGH);
    SPI.endTransaction();

    return 0;
}

void SPI_init()
{
    pinMode(ChipSelectPin, OUTPUT);
    digitalWrite(ChipSelectPin, HIGH);
    SPI.begin();
}


