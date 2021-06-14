/******************************************************************************************

  File:        astronode.cpp
  Author:      Raphael Valceschini
  E-mail:      valceschini.r@bluewin.ch
******************************************************************************************/
/****************************************************************************************

  Created on:       01.04.2021
  Supported Hardware: Arduino MKR 1400

  Firmware Version 1.1
  First version
****************************************************************************************/

#include "astronode.h"

uint8_t ASTRONODE::begin(Stream &serialPort)
{
  _serialPort = &serialPort;

  //Set-up UART
  _serialPort->setTimeout(TIMEOUT_SERIAL);

  //Clear buffer
  while (_serialPort->available() > 0)
  {
    _serialPort->read();
  }

  //Wait for boot to complete
  delay(BOOT_TIME);

  return SUCCESS;
}

void ASTRONODE::end()
{
  //Empty
}

uint8_t ASTRONODE::configuration_write(bool with_pl_ack,
                                       bool with_geoloc,
                                       bool with_ephemeris,
                                       bool with_deep_sleep,
                                       bool with_ack_event_pin_mask,
                                       bool with_reset_event_pin_mask)
{
  //Set parameters
  uint8_t param_w[3];

  if (with_pl_ack)        param_w[0] |= 1 << 0;
  if (with_geoloc)        param_w[0] |= 1 << 1;
  if (with_ephemeris)       param_w[0] |= 1 << 2;
  if (with_deep_sleep)      param_w[0] |= 1 << 3;
  if (with_ack_event_pin_mask)  param_w[2] |= 1 << 0;
  if (with_reset_event_pin_mask)  param_w[2] |= 1 << 1;

  DEBUG_PRINT("Set config: ");

  //Send request
  if (encode_send_request(CFG_WR, param_w, sizeof(param_w)))
  {
    if (receive_decode_answer(NULL, 0) == CFG_WA)
    {
      DEBUG_PRINTLN("SUCCESS");

      return SUCCESS;
    }
    else
    {
      DEBUG_PRINTLN("FAILED");
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::wifi_configuration_write(const char *wland_ssid, const char *wland_key, const char *auth_token)
{
  //Set parameters
  uint8_t param_w[194] = {'\0'};

  uint8_t wland_ssid_length   = strlen(wland_ssid);
  uint8_t wland_key_length  = strlen(wland_key);
  uint8_t auth_token_length   = strlen(auth_token);

  memcpy(&param_w[0], wland_ssid, wland_ssid_length);
  memcpy(&param_w[33], wland_key, wland_key_length);
  memcpy(&param_w[97], auth_token, auth_token_length);

  DEBUG_PRINT("Wifi set config: ");

  //Send request
  if (encode_send_request(WIF_WR, param_w, sizeof(param_w)))
  {
    switch (receive_decode_answer(NULL, 0))
    {
      case WIF_WA:
        {
          DEBUG_PRINTLN("SUCCESS");

          return SUCCESS;
        }
        break;
      case FORMAT_NOT_VALID:
        {
          DEBUG_PRINTLN("FAILED - At least one of the fields (SSID, password, token) is not composed of exclusively printable standard ASCII characters (0x20 to 0x7E).");
        }
        break;
      case FLASH_WRITING_FAILED:
        {
          DEBUG_PRINTLN("FAILED - Failed to write the Wi-Fi settings (SSID, password, token) to the flash.");
        }
        break;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::configuration_read(void)
{
  //Set parameters
  uint8_t param_a[8];

  //Send request
  if (encode_send_request(CFG_RR, NULL, 0))
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == CFG_RA)
    {
      config.product_id        = param_a[0];
      config.hardware_rev      = param_a[1];
      config.firmware_maj_ver    = param_a[2];
      config.firmware_min_ver    = param_a[3];
      config.firmware_rev      = param_a[4];
      config.with_pl_ack       = (param_a[5] & (1 << 0));
      config.with_geoloc       = (param_a[5] & (1 << 1));
      config.with_ephemeris    = (param_a[5] & (1 << 2));
      config.with_deep_sleep_en    = (param_a[5] & (1 << 3));
      config.with_msg_ack_pin_en   = (param_a[7] & (1 << 0));
      config.with_msg_reset_pin_en = (param_a[7] & (1 << 1));

      DEBUG_PRINTLN("terminal configuration: ");
      DEBUG_PRINT("    product_id: ");
      if (config.product_id == TYPE_ASTRONODE_S)
      {
        DEBUG_PRINTLN("Commercial Satellite Astronode");
      }
      else if (config.product_id == TYPE_WIFI_DEVKIT)
      {
        DEBUG_PRINTLN("Commercial Wi-Fi Dev Kit");
      }
      DEBUG_PRINT("    hardware revision: ");
      DEBUG_PRINTLN(config.hardware_rev);
      DEBUG_PRINT("    firmware version: ");
      DEBUG_PRINT(config.firmware_maj_ver);
      DEBUG_PRINT(".");
      DEBUG_PRINT(config.firmware_min_ver);
      DEBUG_PRINT(".");
      DEBUG_PRINTLN(config.firmware_rev);
      DEBUG_PRINT("    with payload ack (1=yes,0=no): ");
      DEBUG_PRINTLN(config.with_pl_ack);
      DEBUG_PRINT("    with geolocation (1=yes,0=no): ");
      DEBUG_PRINTLN(config.with_geoloc);
      DEBUG_PRINT("    with ephemeris (1=yes,0=no): ");
      DEBUG_PRINTLN(config.with_ephemeris);
      DEBUG_PRINT("    with deep sleep mode enable (1=yes,0=no): ");
      DEBUG_PRINTLN(config.with_deep_sleep_en);
      DEBUG_PRINT("    with message ack pin enable (1=yes,0=no): ");
      DEBUG_PRINTLN(config.with_msg_ack_pin_en);
      DEBUG_PRINT("    with message reset pin enable (1=yes,0=no): ");
      DEBUG_PRINTLN(config.with_msg_reset_pin_en);

      return SUCCESS;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::enqueue_payload(uint8_t *data, uint8_t length, uint16_t id)
{
  //Set parameters
  uint8_t param_w[160 + 2];
  uint8_t param_a[2];

  DEBUG_PRINT("Queuing payload with ID #");
  DEBUG_PRINT(id);
  DEBUG_PRINT(": ");

  param_w[0] = (uint8_t)id;
  param_w[1] = (uint8_t)(id >> 8);
  memcpy(&param_w[2], data, length);

  //Send request
  if (encode_send_request(PLD_ER, param_w, length + 2))
  {
    switch (receive_decode_answer(param_a, sizeof(param_a)))
    {
      case PLD_EA:
        {
          //Check that enqueued payload has the correct ID
          uint16_t id_check = (((uint16_t)param_a[1]) << 8) + ((uint16_t)param_a[0]);

          if (id == id_check)
          {
            DEBUG_PRINTLN("SUCCESS");

            return SUCCESS;
          }
          else
          {
            DEBUG_PRINTLN("FAILED - ID mismatch terminal <-> asset. Please dequeue payload first...");
          }
        }
        break;
      case DUPLICATE_ID:
        {
          DEBUG_PRINTLN("FAILED - Failed to queue the payload because the Payload ID provided by the asset is already in use in the terminal queue.");
        }
        break;
      case LENGTH_NOT_VALID:
        {
          DEBUG_PRINTLN("FAILED - Message exceeds the maximum length for a frame.");
        }
        break;
      case BUFFER_FULL:
        {
          DEBUG_PRINTLN("FAILED - Failed to queue the payload because the sending queue is already full.");
        }
        break;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::dequeue_payload(uint16_t *id)
{
  //Set parameters
  uint8_t param_a[2];

  DEBUG_PRINTLN("Remove oldest payload from queue: ");

  //Send request
  if (encode_send_request(PLD_DR, NULL, 0))
  {
    switch (receive_decode_answer(param_a, sizeof(param_a)))
    {
      case PLD_DA:
        {
          *id = (((uint16_t)param_a[1]) << 8) + ((uint16_t)param_a[0]);

          DEBUG_PRINT("SUCCESS - Removed payload with ID #");
          DEBUG_PRINT(*id);
          DEBUG_PRINT("from terminal's queue");

          return SUCCESS;
        }
        break;
      case BUFFER_EMPTY:
        {
          DEBUG_PRINTLN("FAILED - Failed to dequeue a payload from the buffer because the buffer is empty.");
        }
        break;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::clear_free_payloads(void)
{
  //Set parameters
  //None

  DEBUG_PRINT("Clear/free entire terminal's queue: ");

  //Send request
  if (encode_send_request(PLD_FR, NULL, 0))
  {
    if (receive_decode_answer(NULL, 0) == PLD_FA)
    {
      DEBUG_PRINTLN("SUCCESS");

      return SUCCESS;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::geolocation_write(int32_t lat, int32_t lon)
{
  //Set parameters
  uint8_t param_w[8];

  memcpy(&param_w[0], &lat, sizeof(lat));
  memcpy(&param_w[4], &lon, sizeof(lon));

  DEBUG_PRINT("Set geolocation: ");

  //Send request
  if (encode_send_request(GEO_WR, param_w, sizeof(param_w)))
  {
    switch (receive_decode_answer(NULL, 0))
    {
      case GEO_WA:
        {
          DEBUG_PRINTLN("SUCCESS");

          return SUCCESS;
        }
        break;
      case INVALID_POS:
        {
          DEBUG_PRINTLN("FAILED - Invalid position");
        }
        break;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::read_satellite_ack(uint16_t *id)
{
  //Set parameters
  uint8_t param_a[2];

  DEBUG_PRINT("Satellite acknowledge received with ID #: ");

  //Send request
  if (encode_send_request(SAK_RR, NULL, 0))
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == SAK_RA)
    {
      *id = (((uint16_t)param_a[1]) << 8) + (uint16_t)(param_a[0]);

      DEBUG_PRINTLN(*id);

      return SUCCESS;
    }
    else
    {
      DEBUG_PRINTLN("FAILED - no message acknowledged");
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::clear_satellite_ack(void)
{
  //Set parameters
  //None

  DEBUG_PRINT("Clear sat ACK: ");

  //Send request
  if (encode_send_request(SAK_CR, NULL, 0))
  {
    if (receive_decode_answer(NULL, 0) == SAK_CA)
    {
      DEBUG_PRINTLN("Satellite acknowledge cleared.");

      return SUCCESS;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::clear_reset_event(void)
{
  //Set parameters
  //None

  DEBUG_PRINT("Clear RESET event: ");

  //Send request
  if (encode_send_request(RES_CR, NULL, 0))
  {
    if (receive_decode_answer(NULL, 0) == RES_CA)
    {
      DEBUG_PRINTLN("RESET event cleared.");

      return SUCCESS;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::event_read(uint8_t *event_type)
{
  //Set parameters
  uint8_t param_a;

  DEBUG_PRINT("Read EVENT: ");

  //Send request
  if (encode_send_request(EVT_RR, NULL, 0))
  {
    if (receive_decode_answer(&param_a, sizeof(param_a)) == EVT_RA)
    {
      if (param_a & (1 << 0))
      {
        DEBUG_PRINTLN("Satellite Acknowledgment (SAK) Available.");

        *event_type = EVENT_SAK;
      }
      else if (param_a & (1 << 1))
      {
        DEBUG_PRINTLN("Terminal has reset.");

        *event_type = EVENT_RESET;
      }
      else
      {
        DEBUG_PRINTLN("No event");

        *event_type = EVENT_NO_EVENT;
      }
      return SUCCESS;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::guid_read(String *guid)
{
  //Set parameters
  uint8_t param_a[36];

  DEBUG_PRINT("GUID: ");

  //Send request
  if (encode_send_request(DGI_RR, NULL, 0))
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == DGI_RA)
    {
      for (uint8_t i = 0; i < sizeof(param_a); i++)
      {
        guid->concat((char)param_a[i]);
      }

      DEBUG_PRINTLN(*guid);

      return SUCCESS;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::serial_number_read(String *sn)
{
  //Set parameters
  uint8_t param_a[16];

  DEBUG_PRINT("Serial number: ");

  //Send request
  if (encode_send_request(DSN_RR, NULL, 0))
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == DSN_RA)
    {
      for (uint8_t i = 0; i < sizeof(param_a); i++)
      {
        sn->concat((char)param_a[i]);
      }

      DEBUG_PRINTLN(*sn);

      return SUCCESS;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::rtc_read(uint32_t *time)
{
  //Set parameters
  uint8_t param_a[4];

  DEBUG_PRINT("RTC time: ");

  //Send request
  if (encode_send_request(RTC_RR, NULL, 0))
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == RTC_RA)
    {
      uint32_t time_tmp = (((uint32_t)param_a[3]) << 24) + (((uint32_t)param_a[2]) << 16) + (((uint32_t)param_a[1]) << 8) + (uint32_t)(param_a[0]);

      *time = time_tmp + 1514761200; //2018-01-01T00:00:00Z (= Astrocast time)

      DEBUG_PRINTLN(*time);

      return SUCCESS;
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::factory_reset(void)
{
  //Set parameters
  //None

  DEBUG_PRINT("Save config: ");

  //Send request
  if (encode_send_request(CFG_FR, NULL, 0))
  {
    if (receive_decode_answer(NULL, 0) == CFG_FA)
    {
      DEBUG_PRINTLN("SUCCES");

      return SUCCESS;
    }
    else
    {
      DEBUG_PRINTLN("FAILED");
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::configuration_save(void)
{
  //Set parameters
  //None

  DEBUG_PRINT("Save config: ");

  //Send request
  if (encode_send_request(CFG_SR, NULL, 0))
  {
    if (receive_decode_answer(NULL, 0) == CFG_SA)
    {
      DEBUG_PRINTLN("SUCCES");

      return SUCCESS;
    }
    else
    {
      DEBUG_PRINTLN("FAILED");
    }
  }
  return ERROR;
}

uint8_t ASTRONODE::encode_send_request(uint8_t reg, uint8_t *param, uint8_t param_length)
{
  //Copy command in buffer
  command_to_astronode[0] = reg;
  memcpy(&command_to_astronode[1], param, param_length);

  //Compute CRC
  uint16_t cmd_crc = crc_compute(command_to_astronode, param_length + 1, 0xFFFF);
  memcpy(&command_to_astronode[1 + param_length], &cmd_crc, sizeof(cmd_crc));

  /*
    DEBUG_PRINT("asset -> terminal (+ CRC): ");
    print_array_to_hex(command_to_astronode, 1 + param_length + 2);
  */

  //Translate to hexadecimal
  byte_array_to_hex_array(command_to_astronode, 1 + param_length + 2, &command_to_astronode_hex[1]);

  //Add escape characters
  command_to_astronode_hex[0] = STX;
  command_to_astronode_hex[2 * (1 + param_length + 2) + 1] = ETX;

  /*
    DEBUG_PRINT("asset -> terminal (+ CRC + HEX encoding): ");
    print_array_to_hex(command_to_astronode_hex, 2 * (1 + param_length + 2) + 2);
  */

  //Write command
  if (_serialPort->write(command_to_astronode_hex, 2 * (1 + param_length + 2) + 2) == (size_t)(2 * (1 + param_length + 2) + 2))
  {
    return SUCCESS;
  }
  return ERROR;
}

uint16_t ASTRONODE::receive_decode_answer(uint8_t *param, uint8_t param_length)
{
  //Read answer
  _serialPort->readBytesUntil(STX, answer_from_astronode_hex, 2 * RESPONSE_MAX_SIZE); //Only start reading from STX character (careful, also consume STX)
  answer_from_astronode_hex[0] = STX;
  size_t rx_length = _serialPort->readBytesUntil(ETX, &answer_from_astronode_hex[1], 2 * RESPONSE_MAX_SIZE);

  if (rx_length)
  {
    /*
    DEBUG_PRINT("terminal -> asset (+ CRC + HEX encoding): ");
    print_array_to_hex(answer_from_astronode_hex, rx_length);
    */

    //Translate to binary
    hex_array_to_byte_array(&answer_from_astronode_hex[1], rx_length, answer_from_astronode);

    /*
      DEBUG_PRINT("terminal -> asset (+ CRC): ");
      print_array_to_hex(answer_from_astronode, rx_length >> 1);
    */

    //Verify CRC
    uint16_t cmd_crc = crc_compute(answer_from_astronode, (rx_length >> 1) - 2, 0xFFFF);
    uint16_t cmd_crc_check = (((uint16_t)answer_from_astronode[(rx_length >> 1) - 2 + 1]) << 8) + (uint16_t)(answer_from_astronode[(rx_length >> 1) - 2]);

    if (cmd_crc == cmd_crc_check)
    {
      if (answer_from_astronode[0] == ERR_RA)
      {
        //Return error code from terminal
        return (((uint16_t)answer_from_astronode[2]) << 8) + (uint16_t)(answer_from_astronode[1]);
      }
      else
      {
        //Extract parameters
        memcpy(param, &answer_from_astronode[1], param_length);

        //Return reply from terminal
        return (uint16_t)answer_from_astronode[0];
      }
    }
    else
    {
      DEBUG_PRINTLN("Failed to check CRC - frame is not valid");
    }
  }
  else
  {
    DEBUG_PRINTLN("Failed to receive data from astronode before timeout");
  }
  return ERROR;
}

uint16_t ASTRONODE::crc_compute(uint8_t *data, uint16_t data_length, uint16_t init)
{
  uint16_t x;
  uint16_t crc = init;

  while (data_length--)
  {
    x = crc >> 8 ^ *data++;
    x ^= x >> 4;
    crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ (x);
  }
  return crc;
}

void ASTRONODE::byte_array_to_hex_array(uint8_t *in, uint8_t length, uint8_t *out)
{
  for (int i = 0; i < length; i++)
  {
    uint8_t nibble_h = (in[i] & 0xF0) >> 4;
    uint8_t nibble_l = in[i] & 0x0F;

    out[i << 1] = nibble_to_hex(nibble_h);
    out[(i << 1) + 1] = nibble_to_hex(nibble_l);
  }
}

void ASTRONODE::hex_array_to_byte_array(uint8_t *in, uint8_t length, uint8_t *out)
{
  for (int i = 0; i < length; i += 2)
  {
    uint8_t nibble_h = hex_to_nibble(in[i]);
    uint8_t nibble_l = hex_to_nibble(in[i + 1]);

    out[i >> 1] = (nibble_h << 4) + nibble_l;
  }
}

uint8_t ASTRONODE::nibble_to_hex(uint8_t nibble)
{
  if (nibble < 10)
  {
    return nibble + 0x30;
  }
  else
  {
    return (nibble % 0x0A) + 0x41;
  }
}

uint8_t ASTRONODE::hex_to_nibble(uint8_t hex)
{
  if (hex < 0x41)
  {
    return hex - 0x30;
  }
  else
  {
    return (hex - 0x41) + 0x0A; //-0x37
  }
}

void ASTRONODE::print_array_to_hex(uint8_t data[], size_t length)
{
  DEBUG_PRINTLN("uint8_t message[] = {");
  for (size_t i = 0; i < length; i++)
  {
    DEBUG_PRINT("0x");
    if ((data[i] >> 4) == 0)
      DEBUG_PRINT("0"); // print preceeding high nibble if it's zero
    DEBUG_PRINTHEX(data[i]);
    if (i < (length - 1))
      DEBUG_PRINT(", ");
    if ((31 - i) % 16 == 0)
      DEBUG_PRINTLN();
  }
  DEBUG_PRINTLN("};\n\r");
}