/**
@attention
* <h2><center>&copy; COPYRIGHT(c) 2026 RLS d.o.o, Pod vrbami 2, 1218 Komenda, Slovenia</center></h2>
******************************************************************************
@file    encolink.cpp
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
#include "encolink.h"

/**
 * @brief Constructor
 * @param[in] Read_write_fun -> function for encoder communication
 */
Encolink::Encolink(Read_write_fun fun)
{
    memset(write_data, 0, sizeof(write_data));
    memset(read_data_arr, 0, sizeof(read_data_arr));
    first_read = _TRUE;
    encoder_initialized = _FALSE;
    queue_set_index = 0;
    queue_get_index = 0;
    fsm_state_1 = 0;
    fsm_state_2 = 0;
    fsm_state_write = 0;
    fsm_state_read = 0;
    fsm_write_temp_index = 0;
    fsm_write_crc = 0;
    fsm_read_temp_index = 0;
    fsm_read_crc = 0;
    calculated_crc_ch1 = 0;
    received_crc_ch1 = 0;
    received_crc_ch0 = 0;
    gdw_latched_position = 0;
    latched_detailed_status = 0;
    master_byte1 = 0;
    master_byte2 = 0;
    comm_channel_1_rx_byte = 0;
    comm_channel_2_rx_byte = 0;
    // Write register access
    write_register_in_progress = _FALSE;
    write_address = 0;
    num_of_write_bytes = 0;
    write_index = 0;
    write_status = 0;
    // Read register access
    read_register_in_progress = _FALSE;
    read_address = 0;
    num_of_read_bytes = 0;
    read_index = 0;
    read_status = 0;
    rx_bytes = 0;
    tx_bytes = 2;
    init_successful = _FALSE;
    read_write_ready_status = 255;
    read_data = 0;

    for (uint8_t index = 0; index < RX_TX_DATA_MAX_LEN; index++)
    {
        transmitted_data[index] = 0;
        received_data[index] = 0;
    }

    // Encoder parameters initialization.
    read_write_frame = fun;
}

/**
 * @brief: Destructor
 */
Encolink::~Encolink()
{
}

/****************************/
/********** PUBLIC **********/
/****************************/

/**
 * @brief      Initialize encoder
 * @return     bool_t
 */
Encolink::bool_t Encolink::initialize()
{
    if (Encolink::read_write_frame == NULL)
    {
        encoder_initialized = _FALSE;
    }
    else
    {
        encoder_initialized = this->read_encoder_ident();
    }
    return encoder_initialized;
}

/**
 * @brief      Read encoder ident command.
 * @note       Sends master request (transmitted_data:
 *             [VERSION COMMAND, dummy bytes]
 *             Saves received bytes to received_data.
 * @param[in]  void
 * @return     bool_t
 */
Encolink::bool_t Encolink::read_encoder_ident()
{
    if (Encolink::read_write_frame == NULL)
    {
        return init_successful;
    }
    transmitted_data[0] = VERSION_COMMAND;
    transmitted_data[1] = 0;
    transmitted_data[2] = 0;
    transmitted_data[3] = 0;

    // Read frame (20 bytes)
    if (0 != read_write_frame(transmitted_data, RX_TX_DATA_MAX_LEN, tx_bytes))
    {
        return init_successful;
    }

    // Reset transmitted_data
    for (uint8_t index = 0; index < RX_TX_DATA_MAX_LEN; index++)
    {
        transmitted_data[index] = 0;
    }

    // set mode back to default (POSITION_COMMAND) for next frames
    transmitted_data[0] = POSITION_COMMAND;

    // Read frame (20 bytes)
    if (0 != read_write_frame(transmitted_data, RX_TX_DATA_MAX_LEN, tx_bytes))
    {
        return init_successful;
    }
    memcpy(received_data, transmitted_data, RX_TX_DATA_MAX_LEN);

    for (uint8_t index = 0; index < RX_TX_DATA_MAX_LEN; index++)
    {
        transmitted_data[index] = 0;
    }

    // memcpy
    memcpy(encoder_ident, received_data, VERSION_STRING_LENGTH);

    uint8_t received_crc = received_data[VERSION_STRING_LENGTH];

    for (uint8_t index = 0; index < RX_TX_DATA_MAX_LEN; index++)
    {
        received_data[index] = 0;
    }

    // check CRC here
    uint8_t calculated_crc = 0;

    for (uint8_t index = 0; index < VERSION_STRING_LENGTH - 1; index++)
    {
        calculated_crc = crc_97_8bit(encoder_ident[index], calculated_crc, _FALSE);
    }
    calculated_crc = crc_97_8bit(encoder_ident[VERSION_STRING_LENGTH - 1], calculated_crc, _TRUE);

    // Allow only the configured protocol version.
    if ((((uint8_t)~calculated_crc) == received_crc) && (ENCOLINK_PROTOCOL_VERSION == encoder_ident[0]))
    {
        // ST
        if (5 == encoder_ident[1])
        {
            dataIdent.num_of_bytes_ch0 = 5;
            rx_bytes = 5; // redundant
            dataIdent.is_multiturn = _FALSE;
            dataIdent.multiturn_resolution = 0;

            init_successful = _TRUE;
        }
        // MT
        else if (7 == encoder_ident[1])
        {
            dataIdent.num_of_bytes_ch0 = 7;
            rx_bytes = 7; // redundant
            dataIdent.is_multiturn = _TRUE;
            dataIdent.multiturn_resolution = 16;

            init_successful = _TRUE;
        }

        if (_TRUE == init_successful)
        {
            dataIdent.protocol_version = encoder_ident[0];
            dataIdent.singleturn_resolution = (encoder_ident[10] - '0') * 10 + (encoder_ident[11] - '0');
            memcpy(dataIdent.ident, encoder_ident, VERSION_STRING_LENGTH);
            memcpy(data.identification_data, encoder_ident, VERSION_STRING_LENGTH);
        }
    }

    return init_successful;
}

/**
 * @brief      Read encoder position command.
 *             Sends master request (transmitted_data:
 *             [config. byte, RA byte, dummy bytes]
 *             Saves received bytes to received_data.
 * @param[in]  void
 * @return     uint8_t
 * @retval     0:  Channel 0 CRC OK.
 * @retval     1:  Channel 0 CRC error.
 * @retval     2:  read_write_frame error.
 */
uint8_t Encolink::read_encoder()
{
    uint64_t CRC_input_data = 0;
    uint8_t calculated_crc_ch0 = 0;

    // check if read_encoder_ident() was called before as some this function depends on some parameter values, which are initialized there
    if (!encoder_initialized)
    {
        data.crc_error_ch0 = _TRUE;
    }
    else
    {
        Encolink::write_register_fsm();
        read_register_fsm();

        transmitted_data[0] = master_byte1;
        transmitted_data[1] = master_byte2;
        transmitted_data[2] = 0;
        transmitted_data[3] = 0;

        // Read frames
        if (0 != read_write_frame(transmitted_data, dataIdent.num_of_bytes_ch0, tx_bytes))
        {
            return 2;
        }
        memcpy(received_data, transmitted_data, dataIdent.num_of_bytes_ch0);

        for (uint8_t index = 0; index < RX_TX_DATA_MAX_LEN; index++)
        {
            transmitted_data[index] = 0;
        }

        if (_FALSE == dataIdent.is_multiturn)
        {
            CRC_input_data = (received_data[0] << 16) + (received_data[1] << 8) + received_data[2];
            calculated_crc_ch0 = crc_spi_97_64bit(CRC_input_data);

            // CRC error
            received_crc_ch0 = received_data[3];
            if ((uint8_t)~calculated_crc_ch0 != received_crc_ch0)
            {
                data.crc_error_ch0 = _TRUE;
            }
            else
            {
                data.crc_error_ch0 = _FALSE;
            }

            data.singleturn_position = ((received_data[0] << 16) + (received_data[1] << 8) + received_data[2]) >> (24 - dataIdent.singleturn_resolution);
            data.error_warning = ~received_data[2] & 0x3;
        }
        else
        {
            CRC_input_data = ((uint64_t)received_data[0] << 32) + ((uint64_t)received_data[1] << 24) +
                             ((uint64_t)received_data[2] << 16) + ((uint64_t)received_data[3] << 8) +
                             ((uint64_t)received_data[4] << 0);

            calculated_crc_ch0 = crc_spi_97_64bit(CRC_input_data);

            // CRC error
            received_crc_ch0 = received_data[5];
            if ((uint8_t)~calculated_crc_ch0 != received_crc_ch0)
            {
                data.crc_error_ch0 = _TRUE;
            }
            else
            {
                data.crc_error_ch0 = _FALSE;
            }

            data.singleturn_position = (((uint32_t)received_data[2] << 16) + ((uint32_t)received_data[3] << 8) +
                                        (uint32_t)received_data[4]) >>
                                       (24 - dataIdent.singleturn_resolution);
            data.multiturn_position = (received_data[0] << 8) + (received_data[1] << 0);
            data.error_warning = ((uint8_t)~received_data[4]) & 0x3;
        }

        if (_FALSE == first_read)
        {
            if (_FALSE == dataIdent.is_multiturn)
            {
                comm_channel_1_rx_byte = received_data[4];
            }
            else
            {
                comm_channel_1_rx_byte = received_data[6];
            }

            if ((master_byte1 != RESET_CHANNELS_COMMAND) && (master_byte1 != VERSION_COMMAND))
            {
                if (_FALSE == dataIdent.is_multiturn)
                {
                    comm_channel_1_st_fsm();
                }
                else
                {
                    comm_channel_1_mt_fsm();
                }
            }
        }

        master_byte1 = POSITION_COMMAND;
        first_read = _FALSE;
    }
    return data.crc_error_ch0;
};

/**
 * @brief      Resets internal variables/states
 * @param[in]  void
 * @return     void
 */
void Encolink::reset_encoder()
{
    for (uint8_t index = 0; index < RA_NUM_OF_QUEUE_ELEMENTS; index++)
    {
        reg_access_buffer[index].read_write_command = 0;
        reg_access_buffer[index].read_write_status = 0;
        reg_access_buffer[index].reg_access_status = EMPTY;
        reg_access_buffer[index].address = 0;
        reg_access_buffer[index].num_of_bytes = 0;
        reg_access_buffer[index].data = 0;
    }

    queue_set_index = 0;
    queue_get_index = 0;

    fsm_state_1 = 0;
    fsm_state_2 = 0;
    fsm_state_write = 0;
    fsm_state_read = 0;
    calculated_crc_ch1 = 0;
    received_crc_ch1 = 0;
    gdw_latched_position = 0;
    latched_detailed_status = 0;

    master_byte1 = 0;
    master_byte2 = 0;
    comm_channel_1_rx_byte = 0;
    comm_channel_2_rx_byte = 0;

    // Write register access
    write_register_in_progress = _FALSE;
    write_address = 0;
    num_of_write_bytes = 0;
    write_index = 0;
    write_status = 0;

    // Read register access
    read_register_in_progress = _FALSE;
    read_address = 0;
    num_of_read_bytes = 0;
    read_index = 0;
    read_status = 0;

    master_byte1 = RESET_CHANNELS_COMMAND;
    first_read = _TRUE;
}

/**
 * @brief      API function for read register command,
 *             fills a buffer if empty, returns buffer ID.
 * @param[in]  num_of_bytes number of bytes for given address.
 * @param[in]  address address of the register we want to read from.
 * @return     uint8_t status
 * @retval     buffer ID for reading the data from register
 */
uint8_t Encolink::read_register_command(uint16_t num_of_bytes, uint32_t address)
{
    uint8_t temp_index = 0;
    uint8_t status = 0xFF;

    if (num_of_bytes == 0 || num_of_bytes > WRITE_DATA_BUFFER_LENGTH)
    {
        return status;
    }

    for (uint8_t index = 0; index < RA_NUM_OF_QUEUE_ELEMENTS; index++)
    {
        temp_index = queue_set_index;

        if (EMPTY == reg_access_buffer[temp_index].reg_access_status)
        {
            reg_access_buffer[temp_index].address = address;
            reg_access_buffer[temp_index].read_write_command = READ_CMD;
            reg_access_buffer[temp_index].num_of_bytes = num_of_bytes;
            reg_access_buffer[temp_index].data = 0;
            reg_access_buffer[temp_index].reg_access_status = READ_PENDING;

            status = temp_index;
            inc_queue_set_index();
            break;
        }
        inc_queue_set_index();
    }

    return status;
}

/**
 * @brief      API function for write register command,
 *             fills a buffer if empty, returns buffer ID.
 * @param[in]  num_of_bytes number of bytes for given address.
 * @param[in]  address address of the register we want to read from.
 * @param[in]  data data to write to register.
 * @return     uint8_t status
 * @retval     buffer ID for writing the data to register
 */
uint8_t Encolink::write_register_command(uint16_t num_of_bytes, uint32_t address, uint32_t data)
{
    uint8_t temp_index = 0;
    uint8_t status = 0xFF;

    if (num_of_bytes == 0 || num_of_bytes > WRITE_DATA_BUFFER_LENGTH)
    {
        return status;
    }

    for (uint8_t index = 0; index < RA_NUM_OF_QUEUE_ELEMENTS; index++)
    {
        temp_index = queue_set_index;

        if (EMPTY == reg_access_buffer[temp_index].reg_access_status)
        {
            reg_access_buffer[temp_index].address = address;
            reg_access_buffer[temp_index].read_write_command = WRITE_CMD;
            reg_access_buffer[temp_index].num_of_bytes = num_of_bytes;
            reg_access_buffer[temp_index].data = data;
            reg_access_buffer[temp_index].reg_access_status = WRITE_PENDING;

            status = temp_index;
            inc_queue_set_index();
            break;
        }
        inc_queue_set_index();
    }

    return status;
}

/**
 * @brief      Get read registers data for given buffer ID.
 * @param[in]  id ID of parameter
 * @param[in]  read_data pointer to read data
 * @return     uint8_t status
 * @retval     result of read status
 */
uint8_t Encolink::get_read_register_data(uint8_t id, uint32_t &read_data)
{
    if (id >= RA_NUM_OF_QUEUE_ELEMENTS)
    {
        return DATA_NOT_READY;
    }
    status_read = read_delete_queue_item(id);
    if (status_read != DATA_NOT_READY)
    {
        read_data = reg_access_buffer[id].data;
    }
    return status_read;
}

/**
 * @brief      API function returns status of write proccedure
 * @param[in]  id ID of parameter
 * @return     uint8_t status
 * @retval     result of write status (0xFF means that get_write_status is not executed properly)
 */
uint8_t Encolink::get_write_register_status(uint8_t id)
{
    if (id >= RA_NUM_OF_QUEUE_ELEMENTS)
    {
        return DATA_NOT_READY;
    }
    return write_delete_queue_item(id);
}

/**
 * @brief        Get CH2 byte
 * @param[in]    void
 * @return       uint8_t comm_channel_2_rx_byte
 * @retval       Communication channel 2 receive byte
 */
uint8_t Encolink::get_channel_two_byte()
{
    return comm_channel_2_rx_byte;
}

/**
 * @brief        Status string
 * @param[in]    uint8_t a Status code of executed operation
 * @return       const char * Readable string explaining the status of the executed operation
 */
const char *Encolink::statusString(uint8_t a)
{
    Register reg = static_cast<Register>(a);
    switch (reg)
    {
#define X(ID, VALUE)   \
    case Register::ID: \
        return #ID;
        Registers
#undef X
    default:
        return "UNKNOWN";
    };
}

/***************************/
/********* PRIVATE *********/
/***************************/

/**
 * @brief        Finite State Machine for communication (singleturn) channel one.
 * @note         Slave data is saved to variables.
 *               Data every 7th byte from channel one frame gets added
 *               to channel one until channel one buffer is full.
 *               Channel one frame :
 *               [ST POS, ST POS, ST POS, DETAILED STAT, DETAILED STAT, Ch2 Byte, ~CRC]
 * @param[in]    void
 * @return       void
 */
void Encolink::comm_channel_1_st_fsm(void)
{
    switch (fsm_state_1)
    {
    case 0:
        calculated_crc_ch1 = 0;
        data.crc_error_ch1 = _FALSE;
        gdw_latched_position = 0;
        gdw_latched_position |= ((comm_channel_1_rx_byte & 0xFF) << 16);
        data.latched_singleturn_position_ch0 = data.singleturn_position;
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 1:
        gdw_latched_position |= ((comm_channel_1_rx_byte & 0xFF) << 8);
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 2:
        gdw_latched_position |= ((comm_channel_1_rx_byte & 0xFF) << 0);
        data.latched_singleturn_position = gdw_latched_position >> (24 - dataIdent.singleturn_resolution);
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 3:
        latched_detailed_status = 0;
        latched_detailed_status |= ((comm_channel_1_rx_byte & 0xFF) << 8);
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 4:
        latched_detailed_status |= ((comm_channel_1_rx_byte & 0xFF) << 0);
        data.latched_detailed_status = latched_detailed_status;
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 5:
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _TRUE);
        comm_channel_2_rx_byte = comm_channel_1_rx_byte;
        comm_channel_2_fsm();
        fsm_state_1++;
        break;

    case 6:
        received_crc_ch1 = comm_channel_1_rx_byte;
        // CRC error
        if ((uint8_t)~calculated_crc_ch1 != (uint8_t)received_crc_ch1)
        {
            data.crc_error_ch1 = _TRUE;
        }
        fsm_state_1 = 0;
        break;

    default:
        break;
    }
}

/**
 * @brief        Finite State Machine for communication (multiturn) channel one.
 * @note         Slave data is saved to variables.
 *               Data every 9th byte from channel one frame gets added
 *               to channel one until channel one buffer is full.
 *               Channel one frame :
 *               [MT POS, MT POS, ST POS, ST POS, ST POS, DETAILED STAT, DETAILED STAT, Ch2 Byte, ~CRC]
 * @param[in]    void
 * @return       void
 */
void Encolink::comm_channel_1_mt_fsm(void)
{
    switch (fsm_state_1)
    {
    case 0:
        calculated_crc_ch1 = 0;
        data.crc_error_ch1 = _FALSE;
        gdw_latched_position = 0;
        gdw_latched_position = ((comm_channel_1_rx_byte & 0xFF) << 8);
        data.latched_multiturn_position_ch0 = data.multiturn_position;
        data.latched_singleturn_position_ch0 = data.singleturn_position;
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 1:
        gdw_latched_position |= ((comm_channel_1_rx_byte & 0xFF) << 0);
        data.latched_multiturn_position = gdw_latched_position;
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 2:
        gdw_latched_position = 0;
        gdw_latched_position |= ((comm_channel_1_rx_byte & 0xFF) << 16);
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 3:
        gdw_latched_position |= ((comm_channel_1_rx_byte & 0xFF) << 8);
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 4:
        gdw_latched_position |= ((comm_channel_1_rx_byte & 0xFF) << 0);
        data.latched_singleturn_position = gdw_latched_position >> (24 - dataIdent.singleturn_resolution);
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 5:
        latched_detailed_status = 0;
        latched_detailed_status |= ((comm_channel_1_rx_byte & 0xFF) << 8);
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 6:
        latched_detailed_status |= ((comm_channel_1_rx_byte & 0xFF) << 0);
        data.latched_detailed_status = latched_detailed_status;
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _FALSE);
        fsm_state_1++;
        break;

    case 7:
        comm_channel_2_rx_byte = comm_channel_1_rx_byte;
        calculated_crc_ch1 = crc_97_8bit(comm_channel_1_rx_byte, calculated_crc_ch1, _TRUE);

        comm_channel_2_fsm();
        fsm_state_1++;
        break;

    case 8:
        received_crc_ch1 = comm_channel_1_rx_byte;

        // CRC error
        if ((uint8_t)~calculated_crc_ch1 != (uint8_t)received_crc_ch1)
        {
            data.crc_error_ch1 = _TRUE;
        }

        fsm_state_1 = 0;
        break;

    default:
        break;
    }
}

/**
 * @brief        Finite State Machine for communication channel 2.
 * @note         Slave data is saved to variables.
 *               Penultimate (one before the last) byte of channel 1 represents first byte of channel 2, and so on.
 *               Channel 2 frame :
 *               [IDLE, IDLE, BUSY, READY, RA STATUS, DATA, IDLE,IDLE]
 * @param[in]    void
 * @return       void
 */
void Encolink::comm_channel_2_fsm(void)
{
    switch (fsm_state_2)
    {
    case 0:
        // READ READY
        if (((IDLE_char == last_byte) || (BUSY_char == last_byte)) && (READ_READY_char == comm_channel_2_rx_byte))
        {
            fsm_state_2++;
        }
        // WRITE READY
        else if (((IDLE_char == last_byte) || (BUSY_char == last_byte)) && (WRITE_READY_char == comm_channel_2_rx_byte))
        {
            fsm_state_2++;
        }

        last_byte = comm_channel_2_rx_byte;

        break;

    case 1:
        // READ STATUS
        if (READ_READY_char == last_byte)
        {
            read_status = comm_channel_2_rx_byte;
            reg_access_buffer[queue_get_index].reg_access_status = READ_STATUS_READY;
            reg_access_buffer[queue_get_index].read_write_status = read_status;
            fsm_state_2++;
        }
        // WRITE STATUS
        else if (WRITE_READY_char == last_byte)
        {
            write_status = comm_channel_2_rx_byte;
            reg_access_buffer[queue_get_index].reg_access_status = WRITE_STATUS_READY;
            reg_access_buffer[queue_get_index].read_write_status = write_status;
            write_register_in_progress = _FALSE;
            read_write_ready_status = queue_get_index;
            inc_queue_get_index();

            fsm_state_2 = 0;
        }

        break;

    case 2:
        // last data byte received
        if ((num_of_read_bytes - 1) == read_index)
        {
            read_data_arr[read_index] = comm_channel_2_rx_byte;

            if (4 == num_of_read_bytes)
            {
                reg_access_buffer[queue_get_index].data |= ((uint32_t)read_data_arr[0] << 24);
                reg_access_buffer[queue_get_index].data |= ((uint32_t)read_data_arr[1] << 16);
                reg_access_buffer[queue_get_index].data |= ((uint32_t)read_data_arr[2] << 8);
                reg_access_buffer[queue_get_index].data |= ((uint32_t)read_data_arr[3] << 0);
            }
            else if (2 == num_of_read_bytes)
            {
                reg_access_buffer[queue_get_index].data |= ((uint32_t)read_data_arr[0] << 8);
                reg_access_buffer[queue_get_index].data |= ((uint32_t)read_data_arr[1] << 0);
            }
            else if (1 == num_of_read_bytes)
            {
                reg_access_buffer[queue_get_index].data |= ((uint32_t)read_data_arr[0] << 0);
            }

            reg_access_buffer[queue_get_index].reg_access_status = READ_DATA_READY;
            read_register_in_progress = _FALSE;
            read_write_ready_status = queue_get_index;
            inc_queue_get_index();

            fsm_state_2 = 0;
            num_of_read_bytes = 0;
            read_index = 0;
        }
        else
        {
            if (num_of_read_bytes != 0)
            {
                read_data_arr[read_index++] = comm_channel_2_rx_byte;
            }
            else
            {
                fsm_state_2 = 0;
                num_of_read_bytes = 0;
                read_index = 0;
            }
        }

        break;

    default:
        break;
    }
}

/**
 * @brief        Delete buffer elements if register access status is: Write status ready.
 * @param[in]    id
 * @return       Sets status of buffer back to 0xFF (Empty buffer status).
 */
uint8_t Encolink::write_delete_queue_item(uint8_t id)
{
    uint8_t status = 0xFF;

    if (WRITE_STATUS_READY == reg_access_buffer[id].reg_access_status)
    {
        read_write_ready_status = 255;
        status = reg_access_buffer[id].read_write_status;
        reg_access_buffer[id].read_write_command = 0;
        reg_access_buffer[id].read_write_status = 0;
        reg_access_buffer[id].reg_access_status = EMPTY;
        reg_access_buffer[id].address = 0;
        reg_access_buffer[id].data = 0;
        reg_access_buffer[id].num_of_bytes = 0;
    }

    return status;
}

/**
 * @brief        Delete buffer elements if register access status is: Read status ready.
 * @param[in]    id
 * @return       Sets status of buffer back to 0xFF (Empty buffer status).
 */
uint8_t Encolink::read_delete_queue_item(uint8_t id)
{
    uint8_t status = 0xFF;

    if (READ_DATA_READY == reg_access_buffer[id].reg_access_status)
    {
        read_write_ready_status = 255;
        status = reg_access_buffer[id].read_write_status;
        reg_access_buffer[id].read_write_command = 0;
        reg_access_buffer[id].read_write_status = 0;
        reg_access_buffer[id].reg_access_status = EMPTY;
        reg_access_buffer[id].address = 0;
        reg_access_buffer[id].num_of_bytes = 0;
    }

    return status;
}

/**
 * @brief        Increments Queue index when read register or write register command is triggered.
 * @param[in]    void
 * @return       queue_set_index
 */
uint8_t Encolink::inc_queue_set_index(void)
{
    if (queue_set_index < (RA_NUM_OF_QUEUE_ELEMENTS - 1))
    {
        queue_set_index++;
    }
    else
    {
        queue_set_index = 0;
    }

    return queue_set_index;
}

/**
 * @brief        Increments Queue index when data is read/written.
 * @param[in]    void
 * @return       queue_get_index
 */
uint8_t Encolink::inc_queue_get_index(void)
{
    if (queue_get_index < (RA_NUM_OF_QUEUE_ELEMENTS - 1))
    {
        queue_get_index++;
    }
    else
    {
        queue_get_index = 0;
    }

    return queue_get_index;
}

/**
 * @brief        CRC 0x97 Polynomial, 8-bit input data
 * @note         Calculate CRC with input data from slave.
 * @param[in]    input_data 8-bit data from which to calculate CRC
 * @param[in]    init_CRC initial CRC value
 * @param[in]    last boolean - true if last byte in CRC calculation sequence
 * @return       8-bit CRC value
 */
uint8_t Encolink::crc_97_8bit(uint8_t input_data, uint8_t init_CRC, bool_t last)
{
    uint8_t index = 0;
    uint8_t CRC = 0;

    index = init_CRC;
    CRC = input_data ^ crc8_lut[index];

    if (_TRUE == last)
    {
        CRC = crc8_lut[CRC];
    }

    return CRC;
}

/**
 * @brief        Calculate CRC with input data from slave.
 * @param[in]    input_data 64-bit data from which to calculate CRC
 * @return       8-bit CRC value
 */
uint8_t Encolink::crc_spi_97_64bit(uint64_t input_data)
{
    uint8_t index = 0;
    uint8_t CRC = 0;

    index = (uint8_t)((input_data >> 56u) & (uint64_t)0x000000FFu);

    CRC = (uint8_t)((input_data >> 48u) & (uint64_t)0x000000FFu);
    index = CRC ^ crc8_lut[index];

    CRC = (uint8_t)((input_data >> 40u) & (uint64_t)0x000000FFu);
    index = CRC ^ crc8_lut[index];

    CRC = (uint8_t)((input_data >> 32u) & (uint64_t)0x000000FFu);
    index = CRC ^ crc8_lut[index];

    CRC = (uint8_t)((input_data >> 24u) & (uint64_t)0x000000FFu);
    index = CRC ^ crc8_lut[index];

    CRC = (uint8_t)((input_data >> 16u) & (uint64_t)0x000000FFu);
    index = CRC ^ crc8_lut[index];

    CRC = (uint8_t)((input_data >> 8u) & (uint64_t)0x000000FFu);
    index = CRC ^ crc8_lut[index];

    CRC = (uint8_t)(input_data & (uint64_t)0x000000FFu);
    index = CRC ^ crc8_lut[index];

    CRC = crc8_lut[index];

    return CRC;
}

/**
 * @brief        Finite State Machine for Master Write to register command.
 * @note         Machine ends as Master writes following sequence:
 *               [Keys, Write CMD, Number of bytes, Address, Data, CRC]
 * @param[in]    void
 * @return       void
 */
void Encolink::write_register_fsm(void)
{

    switch (fsm_state_write)
    {
    case 0:
        if ((_TRUE == read_register_in_progress) || (_TRUE == write_register_in_progress))
        {
            break;
        }

        master_byte2 = 0;

        for (uint8_t index = 0; index < RA_NUM_OF_QUEUE_ELEMENTS; index++)
        {
            fsm_write_temp_index = queue_get_index;

            if (WRITE_PENDING == reg_access_buffer[fsm_write_temp_index].reg_access_status)
            {
                num_of_write_bytes = reg_access_buffer[fsm_write_temp_index].num_of_bytes;
                write_address = reg_access_buffer[fsm_write_temp_index].address;

                for (uint16_t i = 0; i < num_of_write_bytes; i++)
                {
                    write_data[i] = (reg_access_buffer[fsm_write_temp_index].data >> (8 * (num_of_write_bytes - 1 - i))) & 0xFF;
                }

                write_register_in_progress = _TRUE;
                reg_access_buffer[fsm_write_temp_index].reg_access_status = WRITE_IN_PROGRESS;

                fsm_state_write++;

                break;
            }
            else
            {
                inc_queue_get_index();
            }
        }
        break;

    case 1:
        master_byte2 = 0;
        fsm_state_write++;
        break;

    case 2:
        master_byte2 = 0xCD;
        fsm_state_write++;
        break;

    case 3:
        master_byte2 = 0xEF;
        fsm_state_write++;
        break;

    case 4:
        master_byte2 = 0x89;
        fsm_state_write++;
        break;

    case 5:
        master_byte2 = 0xAB;
        fsm_state_write++;
        break;

    case 6:
        master_byte2 = WRITE_CMD;
        fsm_state_write++;
        break;

    case 7:
        fsm_write_crc = 0;
        master_byte2 = num_of_write_bytes >> 8;
        fsm_write_crc = crc_97_8bit(master_byte2, fsm_write_crc, _FALSE);
        fsm_state_write++;
        break;

    case 8:
        master_byte2 = num_of_write_bytes >> 0;
        fsm_write_crc = crc_97_8bit(master_byte2, fsm_write_crc, _FALSE);
        fsm_state_write++;
        break;

    case 9:
        master_byte2 = write_address >> 24;
        fsm_write_crc = crc_97_8bit(master_byte2, fsm_write_crc, _FALSE);
        fsm_state_write++;
        break;

    case 10:
        master_byte2 = write_address >> 16;
        fsm_write_crc = crc_97_8bit(master_byte2, fsm_write_crc, _FALSE);
        fsm_state_write++;
        break;

    case 11:
        master_byte2 = write_address >> 8;
        fsm_write_crc = crc_97_8bit(master_byte2, fsm_write_crc, _FALSE);
        fsm_state_write++;
        break;

    case 12:
        master_byte2 = write_address >> 0;
        fsm_write_crc = crc_97_8bit(master_byte2, fsm_write_crc, _FALSE);
        fsm_state_write++;
        break;

    case 13:
        // last data byte written
        if ((num_of_write_bytes - 1) == write_index)
        {
            master_byte2 = write_data[write_index];
            fsm_write_crc = crc_97_8bit(master_byte2, fsm_write_crc, _TRUE);

            fsm_state_write++;
        }
        else
        {
            if (num_of_write_bytes != 0)
            {
                master_byte2 = write_data[write_index++];
                fsm_write_crc = crc_97_8bit(master_byte2, fsm_write_crc, _FALSE);
            }
            else
            {
                fsm_state_write++;
            }
        }
        break;

    case 14:
        master_byte2 = (uint8_t)~fsm_write_crc;
        fsm_state_write++;
        break;

    case 15:
        master_byte2 = 0;
        num_of_write_bytes = 0;
        write_index = 0;
        write_status = 0;
        fsm_state_write = 0;
        reg_access_buffer[fsm_write_temp_index].reg_access_status = WRITE_PROC_COMPLETED;
        break;

    default:
        break;
    }
}

/**
 * @brief        Finite State Machine for Master read register command.
 * @note         Machine ends as Master writes following sequence:
 *               [Keys, Read CMD, Number of bytes, Address, CRC]
 * @param[in]    void
 * @return       void
 */
void Encolink::read_register_fsm(void)
{

    switch (fsm_state_read)
    {
    case 0:
        if ((_TRUE == read_register_in_progress) || (_TRUE == write_register_in_progress))
        {
            break;
        }

        master_byte2 = 0;

        for (uint8_t index = 0; index < RA_NUM_OF_QUEUE_ELEMENTS; index++)
        {
            fsm_read_temp_index = queue_get_index;

            if (READ_PENDING == reg_access_buffer[fsm_read_temp_index].reg_access_status)
            {
                num_of_read_bytes = reg_access_buffer[fsm_read_temp_index].num_of_bytes;
                read_address = reg_access_buffer[fsm_read_temp_index].address;
                reg_access_buffer[fsm_read_temp_index].reg_access_status = READ_IN_PROGRESS;
                read_register_in_progress = _TRUE;
                fsm_state_read++;

                break;
            }
            else
            {
                inc_queue_get_index();
            }
        }
        break;

    case 1:
        master_byte2 = 0;
        fsm_state_read++;
        break;

    case 2:
        master_byte2 = 0xCD;
        fsm_state_read++;
        break;

    case 3:
        master_byte2 = 0xEF;
        fsm_state_read++;
        break;

    case 4:
        master_byte2 = 0x89;
        fsm_state_read++;
        break;

    case 5:
        master_byte2 = 0xAB;
        fsm_state_read++;
        break;

    case 6:
        master_byte2 = READ_CMD;
        fsm_state_read++;
        break;

    case 7:
        fsm_read_crc = 0;
        master_byte2 = num_of_read_bytes >> 8;
        fsm_read_crc = crc_97_8bit(master_byte2, fsm_read_crc, _FALSE);
        fsm_state_read++;
        break;

    case 8:
        master_byte2 = num_of_read_bytes >> 0;
        fsm_read_crc = crc_97_8bit(master_byte2, fsm_read_crc, _FALSE);
        fsm_state_read++;
        break;

    case 9:
        master_byte2 = read_address >> 24;
        fsm_read_crc = crc_97_8bit(master_byte2, fsm_read_crc, _FALSE);
        fsm_state_read++;
        break;

    case 10:
        master_byte2 = read_address >> 16;
        fsm_read_crc = crc_97_8bit(master_byte2, fsm_read_crc, _FALSE);
        fsm_state_read++;
        break;

    case 11:
        master_byte2 = read_address >> 8;
        fsm_read_crc = crc_97_8bit(master_byte2, fsm_read_crc, _FALSE);
        fsm_state_read++;
        break;

    case 12:
        master_byte2 = read_address >> 0;
        fsm_read_crc = crc_97_8bit(master_byte2, fsm_read_crc, _TRUE);
        fsm_state_read++;
        break;

    case 13:
        master_byte2 = (uint8_t)~fsm_read_crc;
        fsm_state_read++;
        break;

    case 14:
        master_byte2 = 0;
        read_index = 0;
        read_status = 0;
        fsm_state_read = 0;
        reg_access_buffer[fsm_read_temp_index].reg_access_status = READ_PROC_COMPLETED;
        break;

    default:
        break;
    }
}

/**
 * @brief        API function returns status of protocol initialization
 * @note         Machine ends as Master writes following sequence:
 *               [Keys, Read CMD, Number of bytes, Address, CRC]
 * @param[in]    void
 * @return       encoder_initialized - result of initialization status
 */
uint8_t Encolink::is_encoder_initialized(void)
{
    return encoder_initialized;
}

/**
 * @brief        Detailed status getter function.
 * @param[in]    void
 * @return       detailed status
 */
uint16_t Encolink::get_detailed_status()
{
    return data.latched_detailed_status;
}

/**
 * @brief        Get CH1 byte
 * @param[in]    void
 * @return       uint8_t
 */
uint8_t Encolink::get_channel_one_byte()
{
    return comm_channel_1_rx_byte;
}

/**
 * @brief        Get CH2 last byte
 * @param[in]    void
 * @return       uint8_t
 */
uint8_t Encolink::get_last_byte()
{
    return last_byte;
}

/**
 * @brief        Get register access status for given buffer ID.
 * @param[in]    id buffer index
 * @return       uint8_t
 */
uint8_t Encolink::get_register_access_status(uint8_t id)
{
    return reg_access_buffer[id].reg_access_status;
}

/**
 * @brief        Get register access buffer address for given buffer ID.
 * @param[in]    id buffer index
 * @return       uint8_t
 */
uint8_t Encolink::get_register_buffer_address(uint8_t id)
{
    return reg_access_buffer[id].address;
}

/**
 * @brief        Get current buffer ID
 * @param[in]    void
 * @return       uint8_t
 */
uint8_t Encolink::current_buffer_id()
{
    return read_write_ready_status;
}


