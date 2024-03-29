/******************************************************************************************
 * File:        astronode.cpp
 * Author:      Raphael Valceschini
 * Compagny:    Astrocast SA
 * Website:     https://www.astrocast.com/
 * E-mail:      rvalceschini@astrocast.com
 ******************************************************************************************/
/****************************************************************************************
 * Created on: 			01.04.2021
 * Supported Hardware: Arduino MKR 1400
 *
 * Firmware Version 1.0
 ****************************************************************************************/

#include "astronode.h"

ans_status_e ASTRONODE::begin(Stream &serialPort)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Connecting to module"));
  }

  _serialPort = &serialPort;

  // Set-up UART
  _serialPort->setTimeout(TIMEOUT_SERIAL);

  // Clear buffer
  while (_serialPort->available() > 0)
  {
    _serialPort->read();
  }

  // Send dummy command (known bug in Astronode S)
  dummy_cmd();

  // Read module state
  return read_module_state();
}

void ASTRONODE::end()
{
  // Empty
}

void ASTRONODE::enableDebugging(Stream &debugPort,
                                bool printFullDebug)
{
  _debugSerial = &debugPort; // Grab which port the user wants us to use for debugging
  if (printFullDebug == true)
  {
    _printFullDebug = true;
  }
  _printDebug = true;
}

void ASTRONODE::disableDebugging(void)
{
  _printDebug = false;
  _printFullDebug = false;
}

ans_status_e ASTRONODE::configuration_write(bool with_pl_ack,
                                            bool with_geoloc,
                                            bool with_ephemeris,
                                            bool with_deep_sleep,
                                            bool with_ack_event_pin_mask,
                                            bool with_reset_event_pin_mask,
                                            bool with_cmd_event_pin_mask,
                                            bool with_tx_pend_event_pin_mask)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Set configuration"));
  }

  // Set parameters
  uint8_t param_w[3] = {};

  if (with_pl_ack)
    param_w[0] |= 1 << 0; // Satellite Acknowledgement
  if (with_geoloc)
    param_w[0] |= 1 << 1; // Add Geolocation
  if (with_ephemeris)
    param_w[0] |= 1 << 2; // Enable Ephemeris
  if (with_deep_sleep)
    param_w[0] |= 1 << 3; // Deep Sleep Mode
  if (with_ack_event_pin_mask)
    param_w[2] |= 1 << 0; // Satellite Ack Event Pin Mask
  if (with_reset_event_pin_mask)
    param_w[2] |= 1 << 1; // Reset Notification Event Pin Mask
  if (with_cmd_event_pin_mask)
    param_w[2] |= 1 << 2; // Command Available Event Pin Mask
  if (with_tx_pend_event_pin_mask)
    param_w[2] |= 1 << 3; // Message Transmission (Tx) Pending Event Pin Mask

  // Send request
  uint8_t reg = CFG_WR;
  ans_status_e ret_val = encode_send_request(reg, param_w, sizeof(param_w));
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == CFG_WA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::configuration_read(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read configuration"));
  }

  // Set parameters
  uint8_t param_a[8] = {};

  // Send request
  uint8_t reg = CFG_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == CFG_RA)
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

      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::configuration_save(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Save configuration"));
  }

  // Set parameters
  // None

  // Send request
  uint8_t reg = CFG_SR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);

  // delay(TIMEOUT_FLASH); // Not ideal location

  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == CFG_SA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::wifi_configuration_write(const char *wland_ssid,
                                                 const char *wland_key,
                                                 const char *auth_token)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Set Wifi configuration"));
  }

  // Set parameters
  uint8_t param_w[194] = {'\0'};

  uint8_t wland_ssid_length = strlen(wland_ssid);
  uint8_t wland_key_length = strlen(wland_key);
  uint8_t auth_token_length = strlen(auth_token);

  memcpy(&param_w[0], wland_ssid, wland_ssid_length);
  memcpy(&param_w[33], wland_key, wland_key_length);
  memcpy(&param_w[97], auth_token, auth_token_length);

  // Send request
  uint8_t reg = WIF_WR;
  ans_status_e ret_val = encode_send_request(reg, param_w, sizeof(param_w));
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == WIF_WA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::satellite_search_config_write(uint8_t search_period,
                                                      bool force_search)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->print(F("ASTRONODE: Set satellite search rate: rate = "));
    _debugSerial->print(search_period);
    _debugSerial->print(F(", force = "));
    _debugSerial->println(force_search);
  }

  // Set parameters
  uint8_t param_w[2] = {};

  param_w[0] = search_period;

  if (force_search)
    param_w[1] |= 1 << 0;

  // Send request
  uint8_t reg = SSC_WR;
  ans_status_e ret_val = encode_send_request(reg, param_w, sizeof(param_w));
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == SSC_WA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::geolocation_write(int32_t lat,
                                          int32_t lon)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->print(F("ASTRONODE: Set geolocation lat:"));
    _debugSerial->print(lat);
    _debugSerial->print(F(", lon:"));
    _debugSerial->println(lon);
  }

  // Set parameters
  uint8_t param_w[8] = {};

  memcpy(&param_w[0], &lat, sizeof(lat));
  memcpy(&param_w[4], &lon, sizeof(lon));

  // Send request
  uint8_t reg = GEO_WR;
  ans_status_e ret_val = encode_send_request(reg, param_w, sizeof(param_w));
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == GEO_WA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::factory_reset(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Factory reset"));
  }

  // Set parameters
  // None

  // Send request
  uint8_t reg = CFG_FR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);

  // delay(TIMEOUT_FLASH); // Not ideal location

  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == CFG_FA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::guid_read(String *guid)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read GUID"));
  }

  // Set parameters
  uint8_t param_a[36] = {};

  // Send request
  uint8_t reg = MGI_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == MGI_RA)
    {
      for (uint8_t i = 0; i < sizeof(param_a); i++)
      {
        guid->concat((char)param_a[i]);
      }
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::serial_number_read(String *sn)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read SN"));
  }

  // Set parameters
  uint8_t param_a[16] = {};

  // Send request
  uint8_t reg = MSN_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == MSN_RA)
    {
      for (uint8_t i = 0; i < sizeof(param_a); i++)
      {
        sn->concat((char)param_a[i]);
      }
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::product_number_read(String *pn)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read PN"));
  }

  // Set parameters
  uint8_t param_a[16] = {};

  // Send request
  uint8_t reg = MPN_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == MPN_RA)
    {
      for (uint8_t i = 0; i < sizeof(param_a); i++)
      {
        pn->concat((char)param_a[i]);
      }
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::rtc_read(uint32_t *time)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read RTC"));
  }

  // Set parameters
  uint8_t param_a[4] = {};

  // Send request
  uint8_t reg = RTC_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == RTC_RA)
    {
      uint32_t time_tmp = (((uint32_t)param_a[3]) << 24) +
                          (((uint32_t)param_a[2]) << 16) +
                          (((uint32_t)param_a[1]) << 8) +
                          (((uint32_t)param_a[0]) << 0);
      *time = time_tmp + ASTROCAST_REF_UNIX_TIME;
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::read_next_contact_opportunity(uint32_t *delay)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read next contact opportunity"));
  }

  // Set parameters
  uint8_t param_a[4] = {};

  // Send request
  uint8_t reg = NCO_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == NCO_RA)
    {
      uint32_t delay_tmp = (((uint32_t)param_a[3]) << 24) +
                           (((uint32_t)param_a[2]) << 16) +
                           (((uint32_t)param_a[1]) << 8) +
                           (((uint32_t)param_a[0] << 0));
      *delay = delay_tmp;
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::read_performance_counter(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read performance counter"));
  }

  // Set parameters
  uint8_t param_a[PER_CMD_LENGTH] = {};

  // Send request
  uint8_t reg = PER_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == PER_RA)
    {
      uint8_t i = 0;
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
      } while (i < PER_CMD_LENGTH);
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::save_performance_counter(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Save performance counter"));
  }

  // Set parameters
  // None

  // Send request
  uint8_t reg = PER_SR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == PER_SA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::clear_performance_counter(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Clear performance counter"));
  }

  // Set parameters
  // None

  // Send request
  uint8_t reg = PER_CR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == PER_CA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::read_module_state(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read module state"));
  }

  // Set parameters
  uint8_t param_a[MST_CMD_LENGTH] = {};

  // Send request
  uint8_t reg = MST_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == MST_RA)
    {
      uint8_t i = 0;
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
        case MST_UPTIME:
          if (length == sizeof(mst_struct.uptime))
            memcpy(&mst_struct.uptime, &param_a[i], length);
          break;
        }
        i += length;
      } while (i < MST_CMD_LENGTH);
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::read_environment_details(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read environment details"));
  }

  // Set parameters
  uint8_t param_a[END_CMD_LENGTH] = {};

  // Send request
  uint8_t reg = END_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == END_RA)
    {
      uint8_t i = 0;
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
      } while (i < END_CMD_LENGTH);
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::read_last_contact_details(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read last contact details"));
  }

  // Set parameters
  uint8_t param_a[LCD_CMD_LENGTH] = {};

  // Send request
  uint8_t reg = LCD_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == LCD_RA)
    {
      uint8_t i = 0;
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
      } while (i < LCD_CMD_LENGTH);
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::enqueue_payload(uint8_t *data,
                                        uint8_t length,
                                        uint16_t id)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->print(F("ASTRONODE: Enqueue payload:"));
    _debugSerial->print(length);
    _debugSerial->print(F("[B] with id "));
    _debugSerial->println(id);
  }

  ans_status_e ret_val;
  if (length <= ASN_MAX_MSG_SIZE)
  {
    // Set parameters
    uint8_t param_w[160 + 2] = {};
    uint8_t param_a[2] = {};

    param_w[0] = (uint8_t)id;
    param_w[1] = (uint8_t)(id >> 8);

    memcpy(&param_w[2], data, length);

    // Send request
    uint8_t reg = PLD_ER;
    ret_val = encode_send_request(reg, param_w, length + 2);
    if (ret_val == ANS_STATUS_DATA_SENT)
    {
      ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
      if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == PLD_EA)
      {
        // Check that enqueued payload has the correct ID
        uint16_t id_check = (((uint16_t)param_a[1]) << 8) + ((uint16_t)param_a[0]);
        if (id == id_check)
        {
          ret_val = ANS_STATUS_SUCCESS;
        }
        else
        {
          ret_val = ANS_STATUS_PAYLOD_ID_CHECK_FAILED;
        }
      }
    }
  }
  else
  {
    ret_val = ANS_STATUS_PAYLOAD_TOO_LONG;
  }

  return ret_val;
}

ans_status_e ASTRONODE::dequeue_payload(uint16_t *id)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->print(F("ASTRONODE: Dequeue payload with id:"));
    _debugSerial->println(*id);
  }

  // Set parameters
  uint8_t param_a[2] = {};

  // Send request
  uint8_t reg = PLD_DR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == PLD_DA)
    {
      *id = (((uint16_t)param_a[1]) << 8) + ((uint16_t)param_a[0]);
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::clear_free_payloads(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Clear all payloads"));
  }

  // Set parameters
  // None

  // Send request
  uint8_t reg = PLD_FR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == PLD_FA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::event_read(uint8_t *event_type)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read event"));
  }

  // Set parameters
  uint8_t param_a;

  // Send request
  uint8_t reg = EVT_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, &param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == EVT_RA)
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
        *event_type = EVENT_MSG_PENDING;
      }
      else
      {
        *event_type = EVENT_NO_EVENT;
      }
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::read_satellite_ack(uint16_t *id)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->print(F("ASTRONODE: Read satellite ack for id "));
  }

  // Set parameters
  uint8_t param_a[2] = {};

  // Send request
  uint8_t reg = SAK_RR;
  ans_status_e ret_val = encode_send_request(SAK_RR, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == SAK_RA)
    {
      *id = (((uint16_t)param_a[1]) << 8) + (uint16_t)(param_a[0]);
      if ((_printDebug == true) || (_printFullDebug == true))
      {
        _debugSerial->println(*id);
      }
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::clear_satellite_ack(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Clear satellite ack event"));
  }

  // Set parameters
  // None

  // Send request
  uint8_t reg = SAK_CR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == SAK_CA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::clear_reset_event(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Clear reset event"));
  }

  // Set parameters
  // None

  // Send request
  uint8_t reg = RES_CR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == RES_CA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::read_command_8B(uint8_t data[DATA_CMD_8B_SIZE],
                                        uint32_t *createdDate)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read command"));
  }

  // Set parameters
  uint8_t param_a[12] = {};

  // Send request
  uint8_t reg = CMD_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == CMD_RA)
    {
      uint32_t time_tmp = (((uint32_t)param_a[3]) << 24) +
                          (((uint32_t)param_a[2]) << 16) +
                          (((uint32_t)param_a[1]) << 8) +
                          (((uint32_t)param_a[0]) << 0);
      *createdDate = time_tmp + ASTROCAST_REF_UNIX_TIME;
      memcpy(data, &param_a[4], DATA_CMD_8B_SIZE);
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::read_command_40B(uint8_t data[DATA_CMD_40B_SIZE],
                                         uint32_t *createdDate)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Read command"));
  }

  // Set parameters
  uint8_t param_a[44] = {};

  // Send request
  uint8_t reg = CMD_RR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, param_a, sizeof(param_a));
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == CMD_RA)
    {
      uint32_t time_tmp = (((uint32_t)param_a[3]) << 24) +
                          (((uint32_t)param_a[2]) << 16) +
                          (((uint32_t)param_a[1]) << 8) +
                          (((uint32_t)param_a[0]) << 0);
      *createdDate = time_tmp + ASTROCAST_REF_UNIX_TIME;
      memcpy(data, &param_a[4], DATA_CMD_40B_SIZE);
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

ans_status_e ASTRONODE::clear_command(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Clear command"));
  }

  // Set parameters
  // None

  // Send request
  uint8_t reg = CMD_CR;
  ans_status_e ret_val = encode_send_request(reg, NULL, 0);
  if (ret_val == ANS_STATUS_DATA_SENT)
  {
    ret_val = receive_decode_answer(&reg, NULL, 0);
    if (ret_val == ANS_STATUS_DATA_RECEIVED && reg == CMD_CA)
    {
      ret_val = ANS_STATUS_SUCCESS;
    }
  }
  return ret_val;
}

void ASTRONODE::dummy_cmd(void)
{
  if ((_printDebug == true) || (_printFullDebug == true))
  {
    _debugSerial->println(F("ASTRONODE: Dummy command (will return error code)"));
  }

  uint8_t reg = 0x00;
  encode_send_request(reg, NULL, 0);
  receive_decode_answer(&reg, NULL, 0);
}

ans_status_e ASTRONODE::encode_send_request(uint8_t reg,
                                            uint8_t *param,
                                            uint8_t param_length)
{
  ans_status_e ret_val;

  // Compute CRC
  uint16_t cmd_crc = crc_compute(reg, param, param_length, 0xFFFF);

  if ((_printDebug == true) && (_printFullDebug == true))
  {
    _debugSerial->print(F("asset -> terminal (+ CRC): CRC = "));
    _debugSerial->print(cmd_crc, HEX);
    _debugSerial->print(F("; Param. length = "));
    _debugSerial->print(param_length);
    _debugSerial->println(F(" [bytes]; Param. data = "));
    print_array_to_hex(param, param_length);
  }

  // Add escape characters
  uint8_t *com_buf_astronode_hex = (uint8_t *)calloc(STX_L + 2 * (REG_L + param_length + CRC_L) + ETX_L, sizeof(uint8_t));
  if (com_buf_astronode_hex == NULL)
  {
    if ((_printDebug == true) || (_printFullDebug == true))
    {
      _debugSerial->println(F("ASTRONDOE: Not enought memory could be allocated in asset."));
    }
    ret_val = ANS_STATUS_HW_ERR;
  }
  else
  {
    uint16_t index_buf_cmd_hex = 0;

    // Add escape characters
    com_buf_astronode_hex[index_buf_cmd_hex++] = STX;

    // Translate to hexadecimal
    byte_array_to_hex_array(&reg, REG_L, &com_buf_astronode_hex[index_buf_cmd_hex]);
    index_buf_cmd_hex += 2 * REG_L;
    byte_array_to_hex_array(param, param_length, &com_buf_astronode_hex[index_buf_cmd_hex]);
    index_buf_cmd_hex += 2 * param_length;
    byte_array_to_hex_array((uint8_t *)&cmd_crc, CRC_L, &com_buf_astronode_hex[index_buf_cmd_hex]);
    index_buf_cmd_hex += 2 * CRC_L;

    // Add escape characters
    com_buf_astronode_hex[index_buf_cmd_hex++] = ETX;

    if ((_printDebug == true) && (_printFullDebug == true))
    {
      _debugSerial->println(F("asset -> terminal (+ CRC + HEX encoding): data = "));
      print_array_to_hex(com_buf_astronode_hex, index_buf_cmd_hex);
    }

    // Write command
    if (_serialPort->write(com_buf_astronode_hex, index_buf_cmd_hex) == (size_t)(index_buf_cmd_hex))
    {
      ret_val = ANS_STATUS_DATA_SENT;
    }
    else
    {
      ret_val = ANS_STATUS_HW_ERR;
    }

    if ((_printDebug == true) || (_printFullDebug == true))
    {
      print_error_code_string(ret_val);
    }

    free(com_buf_astronode_hex);
  }

  return ret_val;
}

ans_status_e ASTRONODE::receive_decode_answer(uint8_t *reg,
                                              uint8_t *param,
                                              uint8_t param_length)
{
  ans_status_e ret_val;

  // Read answer
  uint16_t max_rx_length = STX_L + 2 * (REG_L + param_length + CRC_L) + ETX_L;
  if (max_rx_length < (STX_L + 2 * (REG_L + PERR_L + CRC_L) + ETX_L))
  {
    max_rx_length = STX_L + 2 * (REG_L + PERR_L + CRC_L) + ETX_L; // Account at least for error code
  }
  uint8_t *com_buf_astronode_hex = (uint8_t *)calloc(max_rx_length, sizeof(uint8_t));
  if (com_buf_astronode_hex == NULL)
  {
    if ((_printDebug == true) || (_printFullDebug == true))
    {
      _debugSerial->println(F("ASTRONDOE: Not enought memory could be allocated in asset."));
    }
    ret_val = ANS_STATUS_HW_ERR;
  }
  else
  {
    uint16_t index_buf_cmd_hex = 0;

    size_t rx_length = _serialPort->readBytesUntil(ETX, (char *)com_buf_astronode_hex, max_rx_length);

    if (rx_length >= (STX_L + 2 * (REG_L + CRC_L))) // At least STX (1), REG(2), CRC (4) (ETX ignored by function)
    {
      if ((_printDebug == true) && (_printFullDebug == true))
      {
        _debugSerial->println(F("terminal -> asset (+ CRC + HEX encoding): "));
        print_array_to_hex(com_buf_astronode_hex, rx_length);
      }

      // Translate to binary
      uint16_t cmd_crc_check = 0xFFFF, cmd_crc = 0xFFFF;
      index_buf_cmd_hex += STX_L;

      // Register extraction
      hex_array_to_byte_array(&com_buf_astronode_hex[index_buf_cmd_hex], 2 * REG_L, reg); // Skip STX, ETX not in buffer
      index_buf_cmd_hex += 2 * REG_L;

      // Parameter extraction (handle error cases)
      if (*reg == ERR_RA)
      {
        uint8_t param_err[PERR_L]; // handle case where param = NULL
        hex_array_to_byte_array(&com_buf_astronode_hex[index_buf_cmd_hex], 2 * PERR_L, param_err);
        index_buf_cmd_hex += 2 * PERR_L;
        cmd_crc = crc_compute(*reg, param_err, PERR_L, 0xFFFF);
        ret_val = (ans_status_e)((((uint16_t)param_err[1]) << 8) + (uint16_t)(param_err[0]));
      }
      else
      {
        hex_array_to_byte_array(&com_buf_astronode_hex[index_buf_cmd_hex], 2 * param_length, param);
        index_buf_cmd_hex += 2 * param_length;
        cmd_crc = crc_compute(*reg, param, param_length, 0xFFFF);
        ret_val = ANS_STATUS_DATA_RECEIVED;

        if ((_printDebug == true) && (_printFullDebug == true))
        {
          _debugSerial->print(F("terminal -> asset (+ CRC): CRC = "));
          _debugSerial->print(cmd_crc_check, HEX);
          _debugSerial->print(F("; Length = "));
          _debugSerial->print(param_length);
          _debugSerial->println(F(" [bytes]; data = "));
          print_array_to_hex(param, param_length);
        }
      }

      // CRC extraction
      hex_array_to_byte_array(&com_buf_astronode_hex[index_buf_cmd_hex], 2 * PERR_L, (uint8_t *)&cmd_crc_check);
      index_buf_cmd_hex += 2 * PERR_L;

      // Verify CRC
      if (cmd_crc != cmd_crc_check)
      {
        ret_val = ANS_STATUS_CRC_NOT_VALID;
      }
    }
    else
    {
      ret_val = ANS_STATUS_TIMEOUT;
    }

    if ((_printDebug == true) || (_printFullDebug == true))
    {
      print_error_code_string(ret_val);
    }
    free(com_buf_astronode_hex);
    //_serialPort->flush();  // Not implemented in NeoStream
    while (_serialPort->available()) // Consume remaining bytes in the buffer if any
      _serialPort->read();
  }

  return ret_val;
}

void ASTRONODE::print_error_code_string(uint16_t code)
{
  switch (code)
  {
  case ANS_STATUS_CRC_NOT_VALID:
    _debugSerial->print(F("ASTRONODE: Discrepancy between provided CRC and expected CRC.\n"));
    break;
  case ANS_STATUS_LENGTH_NOT_VALID:
    _debugSerial->print(F("ASTRONODE: Message exceeds the maximum length for a frame.\n"));
    break;
  case ANS_STATUS_OPCODE_NOT_VALID:
    _debugSerial->print(F("ASTRONODE: Invalid Operation Code used.\n"));
    break;
  case ANS_STATUS_ARG_NOT_VALID:
    _debugSerial->print(F("ASTRONODE: Invalid argument used.\n"));
    break;
  case ANS_STATUS_FLASH_WRITING_FAILED:
    _debugSerial->print(F("ASTRONODE: Failed to write to the flash.\n"));
    break;
  case ANS_STATUS_DEVICE_BUSY:
    _debugSerial->print(F("ASTRONODE: Device is busy.\n"));
    break;
  case ANS_STATUS_FORMAT_NOT_VALID:
    _debugSerial->print(F("ASTRONODE: At least one of the fields (SSID, password, token) is not composed of exclusively printable standard ASCII characters (0x20 to 0x7E).\n"));
    break;
  case ANS_STATUS_PERIOD_INVALID:
    _debugSerial->print(F("ASTRONODE: The Satellite Search Config period enumeration value is not valid.\n"));
    break;
  case ANS_STATUS_BUFFER_FULL:
    _debugSerial->print(F("ASTRONODE: Failed to queue the payload because the sending queue is already full.\n"));
    break;
  case ANS_STATUS_DUPLICATE_ID:
    _debugSerial->print(F("ASTRONODE: Failed to queue the payload because the Payload ID provided by the asset is already in use in the terminal queue.\n"));
    break;
  case ANS_STATUS_BUFFER_EMPTY:
    _debugSerial->print(F("ASTRONODE: Failed to dequeue a payload from the buffer because the buffer is empty.\n"));
    break;
  case ANS_STATUS_INVALID_POS:
    _debugSerial->print(F("ASTRONODE: Invalid position.\n"));
    break;
  case ANS_STATUS_NO_ACK:
    _debugSerial->print(F("ASTRONODE: No satellite acknowledgement available for any payload.\n"));
    break;
  case ANS_STATUS_NO_ACK_CLEAR:
    _debugSerial->print(F("ASTRONODE: No payload ack to clear, or it was already cleared.\n"));
    break;
  case ANS_STATUS_NO_COMMAND:
    _debugSerial->print(F("ASTRONODE: No command is available.\n"));
    break;
  case ANS_STATUS_NO_COMMAND_CLEAR:
    _debugSerial->print(F("ASTRONODE: No command to clear, or it was already cleared.\n"));
    break;
  case ANS_STATUS_MAX_TX_REACHED:
    _debugSerial->print(F("ASTRONODE: Failed to test Tx due to the maximum number of transmissions being reached.\n"));
    break;
  case ANS_STATUS_TIMEOUT:
    _debugSerial->print(F("ASTRONODE: Failed to receive data from astronode before timeout.\n"));
    break;
  case ANS_STATUS_HW_ERR:
    _debugSerial->print(F("ASTRONODE: Failed to send data to the terminal.\n"));
    break;
  case ANS_STATUS_SUCCESS:
    _debugSerial->print(F(""));
    break;
  case ANS_STATUS_DATA_SENT:
    _debugSerial->print(F(""));
    break;
  case ANS_STATUS_DATA_RECEIVED:
    _debugSerial->print(F(""));
    break;
  default:
    _debugSerial->print(F("ASTRONODE: Unknown error code.\n"));
    break;
  }
}

uint16_t ASTRONODE::crc_compute(uint8_t reg,
                                uint8_t *param,
                                uint16_t param_length,
                                uint16_t init)
{
  uint16_t x;
  uint16_t crc = init;

  x = crc >> 8 ^ reg;
  x ^= x >> 4;
  crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ (x);

  for (uint8_t i = 0; i < param_length; i++)
  {
    x = crc >> 8 ^ *param++;
    x ^= x >> 4;
    crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ (x);
  }
  return crc;
}

void ASTRONODE::byte_array_to_hex_array(uint8_t *in,
                                        uint8_t length,
                                        uint8_t *out)
{
  for (int i = 0; i < length; i++)
  {
    uint8_t nibble_h = (in[i] & 0xF0) >> 4;
    uint8_t nibble_l = in[i] & 0x0F;

    out[i << 1] = nibble_to_hex(nibble_h);
    out[(i << 1) + 1] = nibble_to_hex(nibble_l);
  }
}

void ASTRONODE::hex_array_to_byte_array(uint8_t *in,
                                        uint8_t length,
                                        uint8_t *out)
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

void ASTRONODE::print_array_to_hex(uint8_t data[],
                                   size_t length)
{
  _debugSerial->println("uint8_t message[] = {");
  for (size_t i = 0; i < length; i++)
  {
    _debugSerial->print("0x");
    if ((data[i] >> 4) == 0)
    {
      _debugSerial->print("0"); // print preceeding high nibble if it's zero
    }
    _debugSerial->print(data[i], HEX);
    if (i < (length - 1))
    {
      _debugSerial->print(", ");
    }
    if ((31 - i) % 16 == 0)
    {
      _debugSerial->println();
    }
  }
  _debugSerial->println("};\n\r");
}
