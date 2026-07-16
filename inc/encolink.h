/**
@attention
* <h2><center>&copy; COPYRIGHT(c) 2026 RLS d.o.o, Pod vrbami 2, 1218 Komenda, Slovenia</center></h2>
******************************************************************************
@file    encolink.h
@brief

@changes

Terms of Use:
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
******************************************************************************
*/

#ifndef _ENCOLINK_H_
#define _ENCOLINK_H_

#include <stdint.h>
#include <string.h>

#include "protocol_config.h"
#include "RegisterAccess/product_config.h"

#define Registers                                \
    X(eERROR_NO_ERRORS, 0x09)                    \
    X(eERROR_DATABASE_INVALID_PARAM_ID, 0x26)    \
    X(eERROR_DATABASE_PARAM_OUTSIDE_RANGE, 0x56) \
    X(eERROR_ACCESS_DENIED, 0x96)                \
    X(eERROR_INSUFFICIENT_NUM_OF_BYTES, 0xEE)    \
    X(eERROR_CONFIGURATION_LOCKED, 0xF6)         \
    X(eERROR_READ_CRC_INVALID, 0xE4)             \
    X(eERROR_WRITE_CRC_INVALID, 0xF9)

enum class Register
{
#define X(ID, VALUE) ID = VALUE,
    Registers
        INVALID
#undef X
};

#define DATA_NOT_READY 0xFF // Register access status if data is not ready.
#define INVALID_ID 0xFF     // Invalid buffer ID if buffer is not empty.


/**
 * @brief Function for communication with encoder.
 *
 * Function that has to be able to send and recive data to/from encoder via UART or SPI interface
 * Must be implemented by user
 *
 * @param data Pointer to the data buffer; will be overwritten with the encoder's response.
 * @param rx_bytes Number of bytes to be received.
 * @param tx_bytes Number of bytes to be transmitted.
 * @return uint8_t Status code: 0 on success, non-zero on error.
 */
typedef uint8_t (*Read_write_fun)(uint8_t *data, uint8_t rx_bytes, uint8_t tx_bytes);

class Encolink
{
public:
    Encolink(Read_write_fun fun); ///< class encolink constructor
    ~Encolink();                  ///< class encolink destructor

    /**
    @brief enum  for boolean assigning 0 to _FALSE and 1 to _TRUE
    */
    enum bool_t : uint8_t
    {
        _FALSE = 0,
        _TRUE = !_FALSE
    };

    /**
    @brief enum for version string without inverted CRC.
    */
    enum version_string_length : uint8_t
    {
        VERSION_STRING_LENGTH = 18, ///< Version string length without inverted CRC.
    };

    /**
    @brief Channel 0 and channel 1 encoder variables.
    */
    class T_Encoder
    {
    public:
        uint8_t identification_data[VERSION_STRING_LENGTH]; ///< Encoder identifier string.
        uint32_t singleturn_position = 0;                   ///< Singleturn position
        uint16_t multiturn_position = 0;                    ///< Multiturn position
        uint8_t error_warning = 0;                          ///< Error/warning byte
        bool_t crc_error_ch0 = _FALSE;                      ///< CRC error status for Channel 0.
        bool_t crc_error_ch1 = _FALSE;                      ///< CRC error status for Channel 1.
        uint32_t latched_singleturn_position = 0;           ///< singleturn position value (channel 1) is latched on every first byte of channel 1
        uint32_t latched_singleturn_position_ch0 = 0;       ///< Compare singleturn position (Channel 0), set on every first byte of channel 1.
        uint16_t latched_multiturn_position_ch0 = 0;        ///< Compare multiturn position (Channel 0), set on every first byte of channel 1.
        uint16_t latched_multiturn_position = 0;            ///< multiturn position value (channel 1) is latched on every first byte of channel 1
        uint16_t latched_detailed_status = 0;               ///< detailed status of encoder; value is latched on every first byte of channel
    };

    /**
    @brief Encoder product identifier (number of bytes, multiturn, singleturn resolution, ident array).
    */
    class T_EncoderIdent
    {
    public:
        uint8_t protocol_version = 0;         ///< Protocol version - valid between 1 and 4
        uint8_t num_of_bytes_ch0 = 0;         ///< Number of bytes in channel 0 frame, different for multiturn/no multiturn encoder
        uint8_t singleturn_resolution = 0;    ///< Singleturn resolution
        uint8_t multiturn_resolution = 0;     ///< Multiturn resolution
        bool_t is_multiturn = _FALSE;         ///< _TRUE if encoder is multiturn.
        uint8_t ident[VERSION_STRING_LENGTH]; ///< Encoder identifier string.
    };

    T_Encoder data;              // Ch0, Ch1 encoder variables.
    T_Encoder g_TempEncoderData; // Ch0, Ch1 temporary encoder variables.
    T_EncoderIdent dataIdent;    // Encoder identification.

    Read_write_fun read_write_frame = NULL; // Function for communication with encoder
    /**
    /////////////////////////////////////////////////
    PUBLIC FUNCTIONS USED IN ENCOLINK ARDUINO LIBRARY
    /////////////////////////////////////////////////
    */

    /**
    PRIMARY FUNCTIONS
    */
    bool_t initialize();         // Initializes encoder Arduino SPI or Arduino UART driver depending on input partnumber.
    bool_t read_encoder_ident(); // Read encoder ident command.  Returns 1 if encoder is initialized.
    uint8_t read_encoder();      // Read encoder command.        Returns channel 0 CRC error (0 if  CRC ok, 1 if CRC error) or 2 if there was error with R/W function
    void reset_encoder();        // Resets encolink master and sets reset encoder (slave) command to master byte 1.

    /**
    READ REGISTER
    */
    uint8_t read_register_command(uint16_t num_of_bytes, uint32_t address); // Read register function
    uint8_t get_read_register_data(uint8_t id, uint32_t &read_data);        // Get read register  data  for given ID. Returns 255 if read data not ready or failed.

    /**
    WRITE REGISTER
    */
    uint8_t write_register_command(uint16_t num_of_bytes, uint32_t address, uint32_t data); // Write register function
    uint8_t get_write_register_status(uint8_t id);                                          // Returns write register status.
    uint8_t get_channel_two_byte();                                                         // Returns channel 2 byte.

    const char *statusString(uint8_t a); // Returns a human readable status string
    /**
    /////////////////////////////////////////////////
    END
    /////////////////////////////////////////////////
    */

private:
    /**
    @brief enum for first master command byte (CONFIG byte sent to encoder)
    */
    enum master_command_config : uint8_t
    {
        POSITION_COMMAND = 0x00,       ///< position request, normal operation (at SPI communication, MOSI line could be connected to GND and encoder will still return position).
        RESET_CHANNELS_COMMAND = 0xAA, ///< position request, reset of all communication channels is performed.
        VERSION_COMMAND = 0xBB,        ///< version request, reset of all communication channels is performed. When version is requested from master, no position is available in next frame. Invalid master commands will be ignored.
    };

    /**
    @brief enum for second master command byte (RA byte sent to encoder)
    */
    enum master_command_ra : uint8_t
    {
        WRITE_CMD = 0x58, ///<  RA Write command sent to encoder after KEY sequence
        READ_CMD = 0x59,  ///<  RA Read  command sent to encoder after KEY sequence
    };

    /**
    @brief regiser access status bytes sent from encoder
    */
    enum master_commands : uint8_t
    {
        IDLE_char = 0x33,        ///<  When master does not perform any WR or RR procedure, 0x33 (IDLE ) byte is returned.
        BUSY_char = 0x66,        ///<  After master start executing WR procedure, encoder returns byte 0x66 (BUSY)via channel 2.
        READ_READY_char = 0x88,  ///<  After master executes RR procedure, encoder will return byte 0x88 (READ_READY) via channel 2 (READ_READY byte is returned after reading procedure on encoder is completed and data is prepared).
        WRITE_READY_char = 0x99, ///<  When write procedure is completed and write status is ready, encoder returns byte 0x99 (WRITE_READY)
    };

    /**
    @brief master register access status for register access read/write procedure to specific buffer
    */
    enum E_REG_ACCESS_STATUS : uint8_t
    {
        EMPTY = 0xFF,             ///< Buffer is empty, no read/write procedure is executing.
        WRITE_PENDING = 1,        ///< Write pending status for specific buffer
        WRITE_IN_PROGRESS = 2,    ///< Write in progress for buffer.
        WRITE_PROC_COMPLETED = 3, ///< Write process (KEY command, RA command, num of write bytes, data, address, crc sending) is completed.
        WRITE_STATUS_READY = 4,   ///< Write status is ready when encoder sends write ready const character.
        READ_PENDING = 5,         ///< Read pending status for specific buffer
        READ_IN_PROGRESS = 6,     ///< Read in progress for buffer.
        READ_PROC_COMPLETED = 7,  ///< Read process (KEY command, RA command, num of read bytes, address, crc sending) is completed.
        READ_STATUS_READY = 8,    ///< Read status is ready when encoder sends read ready const character.
        READ_DATA_READY = 9,      ///< Data is ready to be read from buffer.
    };

    /**
    @brief buffer size for read and write buffer (4 bytes) and number of buffers.
    */
    enum buffer_constants : uint16_t
    {
        WRITE_DATA_BUFFER_LENGTH = 0x64, ///< Write data buffer length
        READ_DATA_BUFFER_LENGTH = 0x64,   ///< Read data buffer length
        RA_NUM_OF_QUEUE_ELEMENTS = 4,    ///< Number of register access buffers
        RX_TX_DATA_MAX_LEN = 20
    };

    /**
    @brief communication selection
    */
    enum communication : uint8_t
    {
        COM_SPI = 0,
        COMM_UART = 1,
    };

    /**
    @brief Register access buffer attributes.
    */
    class T_RegisterAccessQueue
    {
    public:
        uint8_t read_write_command = 0;                ///<  RA command for reading or writing.
        uint16_t num_of_bytes = 0;                     ///< Number of read/write bytes.
        uint32_t address = 0;                          ///< Register address
        uint32_t data = 0;                             ///< Register data
        uint8_t read_write_status = 0;                 ///< Read/write status returned from encoder.
        E_REG_ACCESS_STATUS reg_access_status = EMPTY; ///< Master register access status for read/write procedure.
    };

    /**
    @brief arrays used in this library.
    */
    const uint8_t crc8_lut[256] = {0x00, 0x97, 0xB9, 0x2E, 0xE5, 0x72, 0x5C, 0xCB, 0x5D, 0xCA, 0xE4, 0x73, 0xB8, 0x2F, 0x01, 0x96,
                                   0xBA, 0x2D, 0x03, 0x94, 0x5F, 0xC8, 0xE6, 0x71, 0xE7, 0x70, 0x5E, 0xC9, 0x02, 0x95, 0xBB, 0x2C,
                                   0xE3, 0x74, 0x5A, 0xCD, 0x06, 0x91, 0xBF, 0x28, 0xBE, 0x29, 0x07, 0x90, 0x5B, 0xCC, 0xE2, 0x75,
                                   0x59, 0xCE, 0xE0, 0x77, 0xBC, 0x2B, 0x05, 0x92, 0x04, 0x93, 0xBD, 0x2A, 0xE1, 0x76, 0x58, 0xCF,
                                   0x51, 0xC6, 0xE8, 0x7F, 0xB4, 0x23, 0x0D, 0x9A, 0x0C, 0x9B, 0xB5, 0x22, 0xE9, 0x7E, 0x50, 0xC7,
                                   0xEB, 0x7C, 0x52, 0xC5, 0x0E, 0x99, 0xB7, 0x20, 0xB6, 0x21, 0x0F, 0x98, 0x53, 0xC4, 0xEA, 0x7D,
                                   0xB2, 0x25, 0x0B, 0x9C, 0x57, 0xC0, 0xEE, 0x79, 0xEF, 0x78, 0x56, 0xC1, 0x0A, 0x9D, 0xB3, 0x24,
                                   0x08, 0x9F, 0xB1, 0x26, 0xED, 0x7A, 0x54, 0xC3, 0x55, 0xC2, 0xEC, 0x7B, 0xB0, 0x27, 0x09, 0x9E,
                                   0xA2, 0x35, 0x1B, 0x8C, 0x47, 0xD0, 0xFE, 0x69, 0xFF, 0x68, 0x46, 0xD1, 0x1A, 0x8D, 0xA3, 0x34,
                                   0x18, 0x8F, 0xA1, 0x36, 0xFD, 0x6A, 0x44, 0xD3, 0x45, 0xD2, 0xFC, 0x6B, 0xA0, 0x37, 0x19, 0x8E,
                                   0x41, 0xD6, 0xF8, 0x6F, 0xA4, 0x33, 0x1D, 0x8A, 0x1C, 0x8B, 0xA5, 0x32, 0xF9, 0x6E, 0x40, 0xD7,
                                   0xFB, 0x6C, 0x42, 0xD5, 0x1E, 0x89, 0xA7, 0x30, 0xA6, 0x31, 0x1F, 0x88, 0x43, 0xD4, 0xFA, 0x6D,
                                   0xF3, 0x64, 0x4A, 0xDD, 0x16, 0x81, 0xAF, 0x38, 0xAE, 0x39, 0x17, 0x80, 0x4B, 0xDC, 0xF2, 0x65,
                                   0x49, 0xDE, 0xF0, 0x67, 0xAC, 0x3B, 0x15, 0x82, 0x14, 0x83, 0xAD, 0x3A, 0xF1, 0x66, 0x48, 0xDF,
                                   0x10, 0x87, 0xA9, 0x3E, 0xF5, 0x62, 0x4C, 0xDB, 0x4D, 0xDA, 0xF4, 0x63, 0xA8, 0x3F, 0x11, 0x86,
                                   0xAA, 0x3D, 0x13, 0x84, 0x4F, 0xD8, 0xF6, 0x61, 0xF7, 0x60, 0x4E, 0xD9, 0x12, 0x85, 0xAB, 0x3C};

    T_RegisterAccessQueue reg_access_buffer[RA_NUM_OF_QUEUE_ELEMENTS]; // Register access queue (buffers)
    uint8_t write_data[WRITE_DATA_BUFFER_LENGTH + 1];                  // Write data array.
    uint8_t read_data_arr[READ_DATA_BUFFER_LENGTH + 1];                 // Read data.
    uint8_t encoder_ident[VERSION_STRING_LENGTH];                      // Encoder identification.
    uint8_t transmitted_data[RX_TX_DATA_MAX_LEN];                      // Transmitted data (master)
    uint8_t received_data[RX_TX_DATA_MAX_LEN];                         // Received data (encoder slave)

    // Variable definitions
    bool_t first_read;                // Set to _FALSE on first position read.
    bool_t encoder_initialized;       // _TRUE if encoder is initialized.
    uint8_t queue_set_index;          // Set index for RA queue.
    uint8_t queue_get_index;          // Get index for RA queue.
    uint8_t fsm_state_1;              // Finite state machine channel 1 state.
    uint8_t fsm_state_2;              // Finite state machine channel 2 state.
    uint8_t fsm_state_write;          // Finite state machine write state.
    uint8_t fsm_state_read;           // Finite state machine read state.
    uint8_t fsm_write_temp_index;     // Tracks active queue slot in write_register_fsm.
    uint8_t fsm_write_crc;            // Running CRC in write_register_fsm.
    uint8_t fsm_read_temp_index;      // Tracks active queue slot in read_register_fsm.
    uint8_t fsm_read_crc;             // Running CRC in read_register_fsm.
    uint8_t calculated_crc_ch1;       // Calculated CRC in channel 1.
    uint8_t received_crc_ch1;         // Received CRC in channel 1.
    uint8_t received_crc_ch0;         // Received CRC in channel 0.
    uint64_t gdw_latched_position;    // Variable for reading latched multiturn + singleturn.
    uint16_t latched_detailed_status; // Latched detailed status, channel 1.
    uint8_t detailed_status_byte;     // Detailed status byte
    uint8_t master_byte1;             // Master command byte 1.
    uint8_t master_byte2;             // Master command byte 2.
    uint8_t comm_channel_1_rx_byte;   // Communication channel 1 receive byte.
    uint8_t comm_channel_2_rx_byte;   // Communication channel 2 receive byte.
    uint8_t rx_bytes;                 // Used for lower level driver input. (Receive frame number of bytes)
    uint8_t tx_bytes;                 // Used for lower level driver output. (TX frame number of bytes)
    bool_t init_successful;

    // Write register access
    bool_t write_register_in_progress; // True if write progress.
    uint32_t write_address;            // Write register state machine Write address.
    uint16_t num_of_write_bytes;       // Write register state machine number of write bytes.
    uint16_t write_index;              // Write index.
    uint8_t write_status;              // Write status.

    // Read register access
    bool_t read_register_in_progress; // True if read register in  progress.
    uint32_t read_address;            // Read register state machine read address.
    uint16_t num_of_read_bytes;       // Read register state machine number of read bytes
    uint16_t read_index;              // Read index.
    uint8_t read_status;              // Read status.

    // Store register data
    uint32_t read_data;              //  Register data
    uint8_t read_write_ready_status; //  Used for testing. Saves current buffer number if finished buffer RA write/read.
    bool compare_multiturn_status;   //  Compare multiturn status variable.
    bool compare_singleturn_status;  // Compare singleturn status variable.
    uint8_t last_byte = 0;
    uint8_t status_read = 0xFF;

    // Private functions
    void comm_channel_1_st_fsm(void); // Communication channel 1 Singleturn finite state machine.
    void comm_channel_1_mt_fsm(void); // Communication channel 1 Multiturn finite state machine.
    void comm_channel_2_fsm(void);    // Communication channel 2 finite state machine.

    uint8_t write_delete_queue_item(uint8_t id); // Called when register access status is Write status ready. Deletes/Resets buffer elements
    uint8_t read_delete_queue_item(uint8_t id);  // Called when register access status is Read status ready. Deletes/Resets buffer elements

    uint8_t inc_queue_set_index(void);                                      //  Increments Queue index when read register or write register command is triggered.
    uint8_t inc_queue_get_index(void);                                      //  Increments queue_get_index when data is read/written.
    uint8_t crc_97_8bit(uint8_t input_data, uint8_t init_CRC, bool_t last); //  CRC 0x97 Polynomial, 8-bit input data Calculate CRC with input data from slave.
    uint8_t crc_spi_97_64bit(uint64_t input_data);                          //  Calculate  64bit CRC with input data from slave.
    void write_register_fsm(void);                                          //  Finite State Machine for Master Write to register command.
    void read_register_fsm(void);                                           //  Finite State Machine for Master Read register command.
    uint8_t is_encoder_initialized(void);                                   // Check if encoder is initialized.

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Functions for internal use
    // Getters
    uint16_t get_detailed_status(); // Returns detailed status.
    uint8_t get_channel_one_byte(); // Returns channel 1 byte.
    uint8_t get_last_byte();
    uint8_t get_register_buffer_address(uint8_t id);
    uint8_t get_register_access_status(uint8_t id); // Returns register access status for a buffer ID.
    uint8_t current_buffer_id();
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
};

#endif // _ENCOLINK_H_


