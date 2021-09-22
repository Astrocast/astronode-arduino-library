/******************************************************************************************

  File:        astronode.cpp
  Author:      Raphael Valceschini
  E-mail:      valceschini.r@bluewin.ch
******************************************************************************************/
/****************************************************************************************

  Created on:       01.04.2021
  Supported Hardware: Arduino MKR 1400

  Firmware Version 1.3
  First version
****************************************************************************************/

#include "astronode.h"

uint8_t ASTRONODE::begin(Stream &serialPort)
{
  DEBUG_PRINTLN(F("ASTRONODE - Connecting to module"));

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

  //Send dummy command
  dummy_cmd();

  return ASN_NO_ERROR;
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
  DEBUG_PRINTLN(F("ASTRONODE - Set configuration"));

  //Set parameters
  uint8_t param_w[3] = {};

  if (with_pl_ack)
    param_w[0] |= 1 << 0;
  if (with_geoloc)
    param_w[0] |= 1 << 1;
  if (with_ephemeris)
    param_w[0] |= 1 << 2;
  if (with_deep_sleep)
    param_w[0] |= 1 << 3;
  if (with_ack_event_pin_mask)
    param_w[2] |= 1 << 0;
  if (with_reset_event_pin_mask)
    param_w[2] |= 1 << 1;

  //Send request
  if (encode_send_request(CFG_WR, param_w, sizeof(param_w)) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(NULL, 0) == CFG_WA)
    {
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::configuration_read(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Read configuration"));

  //Set parameters
  uint8_t param_a[8] = {};

  //Send request
  if (encode_send_request(CFG_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == CFG_RA)
    {
      config.product_id = param_a[0];
      config.hardware_rev = param_a[1];
      config.firmware_maj_ver = param_a[2];
      config.firmware_min_ver = param_a[3];
      config.firmware_rev = param_a[4];
      config.with_pl_ack = (param_a[5] & (1 << 0));
      config.with_geoloc = (param_a[5] & (1 << 1));
      config.with_ephemeris = (param_a[5] & (1 << 2));
      config.with_deep_sleep_en = (param_a[5] & (1 << 3));
      config.with_msg_ack_pin_en = (param_a[7] & (1 << 0));
      config.with_msg_reset_pin_en = (param_a[7] & (1 << 1));

      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::configuration_save(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Save configuration"));

  //Set parameters
  //None

  //Send request
  if (encode_send_request(CFG_SR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(NULL, 0) == CFG_SA)
    {
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::wifi_configuration_write(const char *wland_ssid, const char *wland_key, const char *auth_token)
{
  DEBUG_PRINTLN(F("ASTRONODE - Set Wifi configuration"));

  //Set parameters
  uint8_t param_w[194] = {'\0'};

  uint8_t wland_ssid_length = strlen(wland_ssid);
  uint8_t wland_key_length = strlen(wland_key);
  uint8_t auth_token_length = strlen(auth_token);

  memcpy(&param_w[0], wland_ssid, wland_ssid_length);
  memcpy(&param_w[33], wland_key, wland_key_length);
  memcpy(&param_w[97], auth_token, auth_token_length);

  //Send request
  if (encode_send_request(WIF_WR, param_w, sizeof(param_w)) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(NULL, 0) == WIF_WA)
    {
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::satellite_search_config_write(uint8_t search_period, bool force_search)
{
  DEBUG_PRINTLN(F("ASTRONODE - Set satellite search rate"));

  //Set parameters
  uint8_t param_w[2] = {};

  param_w[0] = search_period;

  if (force_search)
    param_w[1] |= 1 << 0;

  //Send request
  if (encode_send_request(SSC_WR, param_w, sizeof(param_w)) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(NULL, 0) == SSC_WA)
    {
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::geolocation_write(int32_t lat, int32_t lon)
{
  DEBUG_PRINT(F("ASTRONODE - Set geolocation lat:"));
  DEBUG_PRINT(lat);
  DEBUG_PRINT(F(", lon:"));
  DEBUG_PRINTLN(lon);

  //Set parameters
  uint8_t param_w[8] = {};

  memcpy(&param_w[0], &lat, sizeof(lat));
  memcpy(&param_w[4], &lon, sizeof(lon));

  //Send request
  if (encode_send_request(GEO_WR, param_w, sizeof(param_w)) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(NULL, 0) == GEO_WA)
    {
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::factory_reset(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Factory reset"));

  //Set parameters
  //None

  //Send request
  if (encode_send_request(CFG_FR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(NULL, 0) == CFG_FA)
    {
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::guid_read(String *guid)
{
  DEBUG_PRINTLN(F("ASTRONODE - Read GUID"));

  //Set parameters
  uint8_t param_a[36] = {};

  //Send request
  if (encode_send_request(MGI_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == MGI_RA)
    {
      for (uint8_t i = 0; i < sizeof(param_a); i++)
      {
        guid->concat((char)param_a[i]);
      }
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::serial_number_read(String *sn)
{
  DEBUG_PRINTLN(F("ASTRONODE - Read SN"));

  //Set parameters
  uint8_t param_a[16] = {};

  //Send request
  if (encode_send_request(MSN_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == MSN_RA)
    {
      for (uint8_t i = 0; i < sizeof(param_a); i++)
      {
        sn->concat((char)param_a[i]);
      }
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::product_number_read(String *pn)
{
  DEBUG_PRINTLN(F("ASTRONODE - Read PN"));

  //Set parameters
  uint8_t param_a[16] = {};

  //Send request
  if (encode_send_request(MPN_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == MPN_RA)
    {
      for (uint8_t i = 0; i < sizeof(param_a); i++)
      {
        pn->concat((char)param_a[i]);
      }
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::rtc_read(uint32_t *time)
{
  DEBUG_PRINTLN(F("ASTRONODE - Read RTC"));

  //Set parameters
  uint8_t param_a[4] = {};

  //Send request
  if (encode_send_request(RTC_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == RTC_RA)
    {
      uint32_t time_tmp = (((uint32_t)param_a[3]) << 24) + (((uint32_t)param_a[2]) << 16) + (((uint32_t)param_a[1]) << 8) + (uint32_t)(param_a[0]);

      *time = time_tmp + 1514764800; //2018-01-01T00:00:00Z (= Astrocast time)

      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::read_next_contact_opportunity(uint32_t *time_to_next_pass)
{
  DEBUG_PRINTLN(F("ASTRONODE - Read next contact opportunity"));

  //Set parameters
  uint8_t param_a[4] = {};

  //Send request
  if (encode_send_request(NCO_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == NCO_RA)
    {
      *time_to_next_pass = (((uint32_t)param_a[3]) << 24) + (((uint32_t)param_a[2]) << 16) + (((uint32_t)param_a[1]) << 8) + (uint32_t)(param_a[0]);
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::read_performance_counter(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Read performance counter"));

  //Set parameters
  uint8_t param_a[PER_CMD_LENGTH] = {};

  //Send request
  if (encode_send_request(PER_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == PER_RA)
    {
      uint8_t i = 0, k = 0;
      do
      {
        uint8_t type = param_a[i++];
        uint8_t length = param_a[i++];
        switch (type)
        {
        case PER_TYPE_SAT_SEARCH_PHASE_CNT:
          if (length == sizeof(per_struct.sat_search_phase_cnt))
            memcpy(&per_struct.sat_search_phase_cnt, &param_a[i], length);
          break;
        case PER_TYPE_SAT_DETECT_OPERATION_CNT:
          if (length == sizeof(per_struct.sat_detect_operation_cnt))
            memcpy(&per_struct.sat_detect_operation_cnt, &param_a[i], length);
          break;
        case PER_TYPE_SIGNAL_DEMOD_PHASE_CNT:
          if (length == sizeof(per_struct.signal_demod_phase_cnt))
            memcpy(&per_struct.signal_demod_phase_cnt, &param_a[i], length);
          break;
        case PER_TYPE_SIGNAL_DEMOD_ATTEMPS_CNT:
          if (length == sizeof(per_struct.signal_demod_attempt_cnt))
            memcpy(&per_struct.signal_demod_attempt_cnt, &param_a[i], length);
          break;
        case PER_TYPE_SIGNAL_DEMOD_SUCCESS_CNT:
          if (length == sizeof(per_struct.signal_demod_success_cnt))
            memcpy(&per_struct.signal_demod_success_cnt, &param_a[i], length);
          break;
        case PER_TYPE_ACK_DEMOD_ATTEMPT_CNT:
          if (length == sizeof(per_struct.ack_demod_attempt_cnt))
            memcpy(&per_struct.ack_demod_attempt_cnt, &param_a[i], length);
          break;
        case PER_TYPE_ACK_DEMOD_SUCCESS_CNT:
          if (length == sizeof(per_struct.ack_demod_success_cnt))
            memcpy(&per_struct.ack_demod_success_cnt, &param_a[i], length);
          break;
        case PER_TYPE_QUEUED_MSG_CNT:
          if (length == sizeof(per_struct.queued_msg_cnt))
            memcpy(&per_struct.queued_msg_cnt, &param_a[i], length);
          break;
        case PER_TYPE_DEQUEUED_UNACK_MSG_CNT:
          if (length == sizeof(per_struct.dequeued_unack_msg_cnt))
            memcpy(&per_struct.dequeued_unack_msg_cnt, &param_a[i], length);
          break;
        case PER_TYPE_ACK_MSG_CNT:
          if (length == sizeof(per_struct.ack_msg_cnt))
            memcpy(&per_struct.ack_msg_cnt, &param_a[i], length);
          break;
        case PER_TYPE_SENT_FRAGMENT_CNT:
          if (length == sizeof(per_struct.sent_fragment_cnt))
            memcpy(&per_struct.sent_fragment_cnt, &param_a[i], length);
          break;
        case PER_TYPE_ACK_FRAGMENT_CNT:
          if (length == sizeof(per_struct.ack_fragment_cnt))
            memcpy(&per_struct.ack_fragment_cnt, &param_a[i], length);
          break;
        case PER_TYPE_CMD_DEMOD_ATTEMPT_CNT:
          if (length == sizeof(per_struct.cmd_demod_attempt_cnt))
            memcpy(&per_struct.cmd_demod_attempt_cnt, &param_a[i], length);
          break;
        case PER_TYPE_CMD_DEMOD_SUCCESS_CNT:
          if (length == sizeof(per_struct.cmd_demod_success_cnt))
            memcpy(&per_struct.cmd_demod_success_cnt, &param_a[i], length);
          break;
        }
        i += length;
        k++;
      } while ((i + 2 < PER_CMD_LENGTH) && (k < PER_CMD_LENGTH));

      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::save_performance_counter(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Save performance counter"));

  //Set parameters
  //None

  //Send request
  if (encode_send_request(PER_SR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(NULL, 0) == PER_SA)
    {
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::clear_performance_counter(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Clear performance counter"));

  //Set parameters
  //None

  //Send request
  if (encode_send_request(PER_CR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(NULL, 0) == PER_CA)
    {
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::read_module_state(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Read module state"));

  //Set parameters
  uint8_t param_a[MST_CMD_LENGTH] = {};

  //Send request
  if (encode_send_request(MST_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == MST_RA)
    {
      uint8_t i = 0, k = 0;
      do
      {
        uint8_t type = param_a[i++];
        uint8_t length = param_a[i++];
        switch (type)
        {
        case MST_TYPE_MSG_IN_QUEUE:
          if (length == sizeof(mst_struct.msg_in_queue))
            memcpy(&mst_struct.msg_in_queue, &param_a[i], length);
          break;
        case MST_TYPE_ACK_MSG_QUEUE:
          if (length == sizeof(mst_struct.ack_msg_in_queue))
            memcpy(&mst_struct.ack_msg_in_queue, &param_a[i], length);
          break;
        case MST_TYPE_LAST_RST:
          if (length == sizeof(mst_struct.last_rst))
            memcpy(&mst_struct.last_rst, &param_a[i], length);
          break;
        }
        i += length;
      } while ((i + 2 < MST_CMD_LENGTH) && (k < MST_CMD_LENGTH));

      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::read_environment_details(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Read environment details"));

  //Set parameters
  uint8_t param_a[END_CMD_LENGTH] = {};

  //Send request
  if (encode_send_request(END_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == END_RA)
    {
      uint8_t i = 0, k = 0;
      do
      {
        uint8_t type = param_a[i++];
        uint8_t length = param_a[i++];
        switch (type)
        {
        case END_TYPE_LAST_MAC_RESULT:
          if (length == sizeof(end_struct.last_mac_result))
            memcpy(&end_struct.last_mac_result, &param_a[i], length);
          break;
        case END_TYPE_LAST_SAT_SEARCH_PEAK_RSSI:
          if (length == sizeof(end_struct.last_sat_search_peak_rssi))
            memcpy(&end_struct.last_sat_search_peak_rssi, &param_a[i], length);
          break;
        case END_TYPE_TIME_SINCE_LAST_SAT_SEARCH:
          if (length == sizeof(end_struct.time_since_last_sat_search))
            memcpy(&end_struct.time_since_last_sat_search, &param_a[i], length);
          break;
        }
        i += length;
      } while ((i + 2 < END_CMD_LENGTH) && (k < END_CMD_LENGTH));

      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::read_last_contact_details(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Read last contact details"));

  //Set parameters
  uint8_t param_a[LCD_CMD_LENGTH] = {};

  //Send request
  if (encode_send_request(LCD_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == LCD_RA)
    {
      uint8_t i = 0, k = 0;
      do
      {
        uint8_t type = param_a[i++];
        uint8_t length = param_a[i++];
        switch (type)
        {
        case LCD_TYPE_TIME_START_LAST_CONTACT:
          if (length == sizeof(lcd_struct.time_start_last_contact))
            memcpy(&lcd_struct.time_start_last_contact, &param_a[i], length);
          break;
        case LCD_TYPE_TIME_END_LAST_CONTACT:
          if (length == sizeof(lcd_struct.time_end_last_contact))
            memcpy(&lcd_struct.time_end_last_contact, &param_a[i], length);
          break;
        case LCD_TYPE_PEAK_RSSI_LAST_CONTACT:
          if (length == sizeof(lcd_struct.peak_rssi_last_contact))
            memcpy(&lcd_struct.peak_rssi_last_contact, &param_a[i], length);
          break;
        case LCD_TYPE_TIME_PEAK_RSSI_LAST_CONTACT:
          if (length == sizeof(lcd_struct.time_peak_rssi_last_contact))
            memcpy(&lcd_struct.time_peak_rssi_last_contact, &param_a[i], length);
          break;
        }
        i += length;
      } while ((i + 2 < LCD_CMD_LENGTH) && (k < LCD_CMD_LENGTH));

      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::enqueue_payload(uint8_t *data, uint8_t length, uint16_t id)
{
  DEBUG_PRINT(F("ASTRONODE - Enqueue payload:"));
  DEBUG_PRINT(length);
  DEBUG_PRINT(F("[B] with id "));
  DEBUG_PRINTLN(id);

  //Set parameters
  uint8_t param_w[160 + 2] = {};
  uint8_t param_a[2] = {};

  param_w[0] = (uint8_t)id;
  param_w[1] = (uint8_t)(id >> 8);
  memcpy(&param_w[2], data, length);

  //Send request
  if (encode_send_request(PLD_ER, param_w, length + 2) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == PLD_EA)
    {
      //Check that enqueued payload has the correct ID
      uint16_t id_check = (((uint16_t)param_a[1]) << 8) + ((uint16_t)param_a[0]);
      if (id == id_check)
      {
        return ASN_NO_ERROR;
      }
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::dequeue_payload(uint16_t *id)
{
  DEBUG_PRINT(F("ASTRONODE - Dequeue payload with id:"));
  DEBUG_PRINTLN(*id);

  //Set parameters
  uint8_t param_a[2] = {};

  //Send request
  if (encode_send_request(PLD_DR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == PLD_DA)
    {
      *id = (((uint16_t)param_a[1]) << 8) + ((uint16_t)param_a[0]);

      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::clear_free_payloads(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Clear all payloads"));

  //Set parameters
  //None

  //Send request
  if (encode_send_request(PLD_FR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(NULL, 0) == PLD_FA)
    {
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::event_read(uint8_t *event_type)
{
  DEBUG_PRINTLN(F("ASTRONODE - Read event"));

  //Set parameters
  uint8_t param_a;

  //Send request
  if (encode_send_request(EVT_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(&param_a, sizeof(param_a)) == EVT_RA)
    {
      if (param_a & (1 << 0))
      {
        *event_type = EVENT_MSG_ACK;
      }
      else if (param_a & (1 << 1))
      {
        *event_type = EVENT_RESET;
      }
      else if (param_a & (1 << 2))
      {
        *event_type = EVENT_CMD_RECEIVED;
      }
      else if (param_a & (1 << 3))
      {
        *event_type = EVENT_CMD_ACK;
      }
      else
      {
        *event_type = EVENT_NO_EVENT;
      }
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::read_satellite_ack(uint16_t *id)
{
  DEBUG_PRINT(F("ASTRONODE - Read satellite ack for id "));
  DEBUG_PRINTLN(*id);

  //Set parameters
  uint8_t param_a[2] = {};

  //Send request
  if (encode_send_request(SAK_RR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(param_a, sizeof(param_a)) == SAK_RA)
    {
      *id = (((uint16_t)param_a[1]) << 8) + (uint16_t)(param_a[0]);

      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::clear_satellite_ack(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Clear satellite ack event"));

  //Set parameters
  //None

  //Send request
  if (encode_send_request(SAK_CR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(NULL, 0) == SAK_CA)
    {
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

uint8_t ASTRONODE::clear_reset_event(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Clear reset event"));

  //Set parameters
  //None

  //Send request
  if (encode_send_request(RES_CR, NULL, 0) == ASN_NO_ERROR)
  {
    if (receive_decode_answer(NULL, 0) == RES_CA)
    {
      return ASN_NO_ERROR;
    }
  }
  return ASN_ERROR_FAILED;
}

void ASTRONODE::dummy_cmd(void)
{
  DEBUG_PRINTLN(F("ASTRONODE - Dummy command (will return error code)"));

  encode_send_request(0x00, NULL, 0);
  receive_decode_answer(NULL, 0);
}

uint8_t ASTRONODE::encode_send_request(uint8_t reg, uint8_t *param, uint8_t param_length)
{
  //Clear buffers (TODO: should not be here)
  for (int i = 0; i < COMMAND_MAX_SIZE + 2; i++)
    com_buf_astronode[i] = 0x00;

  for (int i = 0; i < 2 * (COMMAND_MAX_SIZE + 2) + 2; i++)
    com_buf_astronode_hex[i] = 0x00;

  uint16_t index_buf_cmd = 0, index_buf_cmd_hex = 0;

  //Copy command in buffer
  com_buf_astronode[index_buf_cmd++] = reg;
  memcpy(&com_buf_astronode[index_buf_cmd], param, param_length);
  index_buf_cmd += param_length;

  //Compute CRC
  uint16_t cmd_crc = crc_compute(com_buf_astronode, index_buf_cmd, 0xFFFF);
  memcpy(&com_buf_astronode[index_buf_cmd], &cmd_crc, sizeof(cmd_crc));
  index_buf_cmd += sizeof(cmd_crc);

  //DEBUG_PRINTLN(F("asset -> terminal (+ CRC): "));
  //print_array_to_hex(com_buf_astronode, index_buf_cmd);

  //Add escape characters
  com_buf_astronode_hex[index_buf_cmd_hex++] = STX;

  //Translate to hexadecimal
  byte_array_to_hex_array(com_buf_astronode, index_buf_cmd, &com_buf_astronode_hex[index_buf_cmd_hex]);
  index_buf_cmd_hex += index_buf_cmd << 1; //Double size of data field with conversion

  //Add escape characters
  com_buf_astronode_hex[index_buf_cmd_hex++] = ETX;

  //DEBUG_PRINTLN(F("asset -> terminal (+ CRC + HEX encoding): "));
  //print_array_to_hex(com_buf_astronode_hex, index_buf_cmd_hex);

  //Write command
  if (_serialPort->write(com_buf_astronode_hex, index_buf_cmd_hex) == (size_t)(index_buf_cmd_hex))
  {
    return ASN_NO_ERROR;
  }
  return ASN_ERROR_FAILED;
}

uint16_t ASTRONODE::receive_decode_answer(uint8_t *param, uint8_t param_length)
{
  //Read answer
  size_t rx_length = _serialPort->readBytesUntil(ETX, (char *)com_buf_astronode_hex, 2 * COMMAND_MAX_SIZE);

  if (rx_length)
  {

    //DEBUG_PRINTLN(F("terminal -> asset (+ CRC + HEX encoding): "));
    //print_array_to_hex(com_buf_astronode_hex, rx_length);

    //Translate to binary
    hex_array_to_byte_array(&com_buf_astronode_hex[1], rx_length, com_buf_astronode); // Skip STX, ETX not in buffer

    //DEBUG_PRINTLN(F("terminal -> asset (+ CRC): "));
    //print_array_to_hex(com_buf_astronode, rx_length >> 1);

    //Verify CRC
    uint16_t msg_length = (rx_length >> 1) - 2; //Divid by 2 and remove CRC (2 bytes)
    uint16_t cmd_crc = crc_compute(com_buf_astronode, msg_length, 0xFFFF);
    uint16_t cmd_crc_check = (((uint16_t)com_buf_astronode[msg_length + 1]) << 8) + (uint16_t)(com_buf_astronode[msg_length]);

    if (cmd_crc == cmd_crc_check)
    {
      if (com_buf_astronode[0] == ERR_RA)
      {
        //Process error code from terminal
        uint16_t error_code = (((uint16_t)com_buf_astronode[2]) << 8) + (uint16_t)(com_buf_astronode[1]);
        print_error_code(error_code);
      }
      else
      {
        //Extract parameters
        memcpy(param, &com_buf_astronode[1], param_length);
      }

      //Return reply from terminal
      return (uint16_t)com_buf_astronode[0];
    }
    else
    {
      DEBUG_PRINTLN(F("ASTRONODE - Failed to check CRC - frame is not valid"));
    }
  }
  else
  {
    DEBUG_PRINTLN(F("ASTRONODE - Failed to receive data from astronode before timeout"));
  }
  return ASN_ERROR_FAILED;
}

void ASTRONODE::print_error_code(uint16_t code)
{
  switch (code)
  {
  case CRC_NOT_VALID:
    DEBUG_PRINTLN(F("ASTRONODE - Discrepancy between provided CRC and expected CRC."));
    break;
  case LENGTH_NOT_VALID:
    DEBUG_PRINTLN(F("ASTRONODE - Message exceeds the maximum length for a frame."));
    break;
  case OPCODE_NOT_VALID:
    DEBUG_PRINTLN(F("ASTRONODE - Invalid Operation Code used."));
    break;
  case FORMAT_NOT_VALID:
    DEBUG_PRINTLN(F("ASTRONODE - At least one of the fields (SSID, password, token) is not composed of exclusively printable standard ASCII characters (0x20 to 0x7E)."));
    break;
  case FLASH_WRITING_FAILED:
    DEBUG_PRINTLN(F("ASTRONODE - Failed to write the Wi-Fi settings (SSID, password, token) to the flash."));
    break;
  case PERIOD_INVALID:
    DEBUG_PRINTLN(F("ASTRONODE - The Satellite Search Config period enumeration value is not valid."));
    break;
  case BUFFER_FULL:
    DEBUG_PRINTLN(F("ASTRONODE - Failed to queue the payload because the sending queue is already full."));
    break;
  case DUPLICATE_ID:
    DEBUG_PRINTLN(F("ASTRONODE - Failed to queue the payload because the Payload ID provided by the asset is already in use in the terminal queue."));
    break;
  case BUFFER_EMPTY:
    DEBUG_PRINTLN(F("ASTRONODE - Failed to dequeue a payload from the buffer because the buffer is empty."));
    break;
  case INVALID_POS:
    DEBUG_PRINTLN(F("ASTRONODE - Invalid position."));
    break;
  case NO_ACK:
    DEBUG_PRINTLN(F("ASTRONODE - No satellite acknowledgement available for any payload."));
    break;
  case NO_CLEAR:
    DEBUG_PRINTLN(F("ASTRONODE - No payload ack to clear, or it was already cleared."));
    break;
  case NO_COMMAND:
    DEBUG_PRINTLN(F("ASTRONODE - No command is available."));
    break;
  case NO_COMMAND_CLEAR:
    DEBUG_PRINTLN(F("ASTRONODE - No command to clear, or it was already cleared."));
    break;
  default:
  {
    DEBUG_PRINT(F("ASTRONODE - Unknown error code: "));
    DEBUG_PRINTHEX(code);
  }
  }
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
  Serial.println("uint8_t message[] = {");
  for (size_t i = 0; i < length; i++)
  {
    Serial.print("0x");
    if ((data[i] >> 4) == 0)
    {
      Serial.print("0"); // print preceeding high nibble if it's zero
    }
    Serial.print(data[i], HEX);
    if (i < (length - 1))
    {
      Serial.print(", ");
    }
    if ((31 - i) % 16 == 0)
    {
      Serial.println();
    }
  }
  Serial.println("};\n\r");
}
