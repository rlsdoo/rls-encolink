/**
 * @file    read_register.ino
 * @brief   Read `TEMPERATURE` while continuously streaming position data.
 * @details Sequence:
 *          1) Initialize SPI and EncoLink.
 *          2) Repeatedly call read_encoder() for cyclic position updates.
 *          3) Trigger read_register_command(TEMPERATURE).
 *          4) Poll get_read_register_data() until status is not DATA_NOT_READY.
 *          5) Print register value and status, then repeat.
 * @copyright Copyright (c) 2026 RLS d.o.o, Pod vrbami 2, 1218 Komenda, Slovenia
 * @license SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>
#include <SPI.h>

// Include EncoLink library. 
#include <encolink.h>

uint8_t ChipSelectPin = 10;
uint8_t my_read_write_frame(uint8_t *data, uint8_t rx_bytes, uint8_t tx_bytes);
void SPI_init();

// Create object EncoLink, with read/write function given as a constructir parameter.
Encolink enco = Encolink(my_read_write_frame);

uint32_t       register_data   = 0;    // Variable for storing register data.
bool           ra_trigger_read = true; // Used for reading process.
uint8_t        crc_error       = 0;    // Variable for storing crc error.
uint8_t        ra_buffer       = INVALID_ID;
uint8_t        ra_read_status  = DATA_NOT_READY;

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

		// Read encoder command.
		crc_error = enco.read_encoder();
		Serial.print("Singleturn position: ");
		Serial.print(enco.data.singleturn_position);

		// Channel 1 detailed status.
		Serial.print("   Detailed status:  ");
		Serial.print(enco.data.latched_detailed_status, HEX);

		// Print warning message if CRC error happens.
		if (1 == crc_error)
		{
			Serial.print(" CRC Error !");
		}

		// A small delay for easier reading of COM port (User can remove this). 
		delay(10);

	    // Read temperature command.
		if (ra_trigger_read == true) //ra_trigger_read is set to true when read process is finished (enco.get_read_register_data))
		{
			ra_buffer = enco.read_register_command(TEMPERATURE);
			ra_trigger_read = false;
		}
		
		// Check if read process executed.
		ra_read_status = enco.get_read_register_data(ra_buffer, register_data); 

		// If read process has executed, print register value and RA read status.
		if (DATA_NOT_READY != ra_read_status)
		{
			Serial.print("  Temperature: ");
			Serial.print(register_data);
			Serial.print("  Register Access status:  ");
			Serial.print(enco.statusString(ra_read_status));
			ra_trigger_read = true;

		}

		// New line 
		Serial.println();

	}
}

// User implementation of function responsible for data exchange with the encoder, at successful read/write it returns 0
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


