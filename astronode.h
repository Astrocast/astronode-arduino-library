/******************************************************************************************

  File:        astronode.h
  Author:      Raphael Valceschini
  E-mail:      valceschini.r@bluewin.ch
******************************************************************************************/
/****************************************************************************************

  Created on:       01.04.2021
  Supported Hardware: Arduino MKR 1400

  Firmware Version 1.1
  First version
****************************************************************************************/

#ifndef _ASTRONODE_h
#define _ASTRONODE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

//Debug output
#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(x)  Serial.print(x)
#define DEBUG_PRINTHEX(x) Serial.print (x, HEX)
#define DEBUG_PRINTLN(x)  Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

//Timeout
#define TIMEOUT_SERIAL 100  //ms
#define BOOT_TIME 700   //ms

//REQUEST (Asset => Terminal)
#define CFG_WR 0x05 //Write configuration, and store in non-volatile memory
#define WIF_WR 0x06 //Write Wi-Fi settings, and store non-volatile memory (Wi-Fi only)
#define CFG_SR 0x10 //Save configuration in NVM request
#define CFG_FR 0x11 //Factory reset configuration request
#define CFG_RR 0x15 //Read configuration from non-volatile memory
#define RTC_RR 0x17 //Real Time Clock read request
#define DGI_RR 0x19 //Module GUID read request
#define DSN_RR 0x1A //Module Serial Number read request
#define PLD_ER 0x25 //Enqueue uplink payload in non-volatile memory
#define PLD_DR 0x26 //Dequeue uplink payload from non-volatile memory
#define PLD_FR 0x27 //Clear (Free) all queued payloads from non-volatile memory
#define GEO_WR 0x35 //Write geolocation longitude and latitude, and store in non-volatile memory
#define SAK_RR 0x45 //Read Acknowledgment
#define SAK_CR 0x46 //Confirm to the terminal that Acknowledgment was properly decoded and can be deleted by the terminal
#define RES_CR 0x55 //Clear reset event
#define EVT_RR 0x65 //Reads the event register

//ANSWER (Terminal => Asset)
#define CFG_WA 0x85 //Answer last configuration write operation with status
#define WIF_WA 0x86 //Answer last Wi-Fi settings write operation with status (Wi-Fi only)
#define CFG_SA 0x90 //Answer last configuration save requests with status
#define CFG_FA 0x91 //Answer last factory reset request with status
#define CFG_RA 0x95 //Answer last configuration read operation with value
#define RTC_RA 0x97 //Answer last RTC read request with module time
#define DGI_RA 0x99 //Answer last GUID read with module GUID
#define DSN_RA 0x9A //Answer last Serial Number read with module Serial Number
#define PLD_EA 0xA5 //Answer last uplink payload enqueue operation with status
#define PLD_DA 0xA6 //Answer last uplink payload dequeue operation with status
#define PLD_FA 0xA7 //Answer last free queued payloads operation with status
#define GEO_WA 0xB5 //Answer last geolocation write operation with status
#define SAK_RA 0xC5 //Answer with Acknowledgment information
#define SAK_CA 0xC6 //Answer last SAK_CR confirmation
#define RES_CA 0xD5 //Answer the reset clear request
#define EVT_RA 0xE5 //Answer indicates which events are currently pending
#define ERR_RA 0xFF //Answer a request reporting an error

//Error code list
#define CRC_NOT_VALID 0x0001    //Discrepancy between provided CRC and expected CRC.
#define LENGTH_NOT_VALID 0x0011   //Message exceeds the maximum length for a frame.
#define OPCODE_NOT_VALID 0x0121   //Invalid Operation Code used.
#define FORMAT_NOT_VALID 0x0601   //At least one of the fields (SSID, password, token) is not composed of exclusively printable standard ASCII characters (0x20 to 0x7E).
#define FLASH_WRITING_FAILED 0x0611 //Failed to write the Wi-Fi settings (SSID, password, token) to the flash
#define BUFFER_FULL 0x2501      //Failed to queue the payload because the sending queue is already full
#define DUPLICATE_ID 0x2511     //Failed to queue the payload because the Payload ID provided by the asset is already in use in the terminal queue.
#define BUFFER_EMPTY 0x2601     //Failed to dequeue a payload from the buffer because the buffer is empty
#define INVALID_POS 0x3501      //Failed to update the geolocation information. Latitude and longitude fields must in the range [-90,90] degrees and [-180,180] degrees, respectively.
#define NO_ACK 0x4501       //No satellite acknowledgement available for any payload.
#define NO_CLEAR 0x4601       //No payload ack to clear, or it was already cleared.

//Events
#define EVENT_SAK 0xF1   //Satellite Acknowledgement (SAK) Available
#define EVENT_RESET 0xF2 //Terminal has reset
#define EVENT_NO_EVENT  0x00

//Escape characters
#define STX 0x02
#define ETX 0x03

//Command/Response size
#define COMMAND_MAX_SIZE 200
#define RESPONSE_MAX_SIZE 100

//Device type
#define TYPE_ASTRONODE_S 3
#define TYPE_WIFI_DEVKIT 4

//Functions return codes
#define ERROR 0
#define SUCCESS 1


class ASTRONODE
{

  private:

    //Global variables
    Stream *_serialPort;

    uint8_t answer_from_astronode[RESPONSE_MAX_SIZE];     //this will contain the response from the dev kit
    uint8_t answer_from_astronode_hex[2 * RESPONSE_MAX_SIZE]; //this will contain the response from the dev kit
    uint8_t command_to_astronode[COMMAND_MAX_SIZE];       //max size is payload size = 160 + START + ID + Length (2B) + CRC (2B).
    uint8_t command_to_astronode_hex[2 * COMMAND_MAX_SIZE];   //max size is payload size = 160 + START + ID + Length (2B) + CRC (2B).

    //Functions prototype
    uint8_t encode_send_request(uint8_t reg, uint8_t *param, uint8_t param_length);
    uint16_t receive_decode_answer(uint8_t *param, uint8_t param_length);
    void byte_array_to_hex_array(uint8_t *in, uint8_t length, uint8_t *out);
    void hex_array_to_byte_array(uint8_t *in, uint8_t length, uint8_t *out);
    uint8_t nibble_to_hex(uint8_t nibble);
    uint8_t hex_to_nibble(uint8_t hex);
    uint16_t crc_compute(uint8_t *data, uint16_t data_length, uint16_t init);

    void print_array_to_hex(uint8_t data[], size_t length);
    String print2digits(int number);
    String time_to_string(uint8_t year, uint8_t month, uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds);

  public:

    //Global variables
    typedef struct
    {
      uint8_t product_id;
      uint8_t hardware_rev;
      uint8_t firmware_maj_ver;
      uint8_t firmware_min_ver;
      uint8_t firmware_rev;
      bool with_pl_ack;
      bool with_geoloc;
      bool with_ephemeris;
      bool with_deep_sleep_en;
      bool with_msg_ack_pin_en;
      bool with_msg_reset_pin_en;
    } ASTRONODE_CONFIG;
    ASTRONODE_CONFIG config;

    //Functions prototype
    uint8_t begin(Stream &serialPort);
    void end();

    uint8_t configuration_write(bool with_pl_ack,
                                bool with_geoloc,
                                bool with_ephemeris,
                                bool with_deep_sleep,
                                bool with_ack_event_pin_mask,
                                bool with_reset_event_pin_mask);
    uint8_t wifi_configuration_write(const char *wland_ssid,
                                     const char *wland_key,
                                     const char *auth_token);
    uint8_t geolocation_write(int32_t lat, int32_t lon);
    uint8_t factory_reset(void);
    uint8_t configuration_read(void);
    uint8_t configuration_save(void);

    uint8_t guid_read(String *guid);
    uint8_t serial_number_read(String *sn);
    uint8_t rtc_read(uint32_t *time);

    uint8_t enqueue_payload(uint8_t *data, uint8_t length, uint16_t id);
    uint8_t dequeue_payload(uint16_t *id);
    uint8_t clear_free_payloads(void);

    uint8_t event_read(uint8_t *event_type);
    uint8_t read_satellite_ack(uint16_t *id);
    uint8_t clear_satellite_ack(void);
    uint8_t clear_reset_event(void);
};

#endif
