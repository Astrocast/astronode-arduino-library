/******************************************************************************************
* File:        astronode.h
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

#ifndef _ASTRONODE_h
#define _ASTRONODE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

//Timeout
#define TIMEOUT_SERIAL 1500 //ms
#define BOOT_TIME 700       //ms

//REQUEST (Asset => Terminal)
#define CFG_WR 0x05 //Write configuration, and store in non-volatile memory
#define WIF_WR 0x06 //Write Wi-Fi settings, and store non-volatile memory (Wi-Fi only)
#define SSC_WR 0x07 //Satellite Search Configuration Write Request. Stored in RAM (never saved in NVM).
#define CFG_SR 0x10 //Save configuration in NVM request
#define CFG_FR 0x11 //Factory reset configuration request
#define CFG_RR 0x15 //Read configuration from non-volatile memory
#define RTC_RR 0x17 //Real Time Clock read request
#define NCO_RR 0x18 //Next Contact Opportunity read request
#define MGI_RR 0x19 //Module GUID read request
#define MSN_RR 0x1A //Module Serial Number read request
#define MPN_RR 0x1B //Module Product Number read request
#define PLD_ER 0x25 //Enqueue uplink payload in non-volatile memory
#define PLD_DR 0x26 //Dequeue uplink payload from non-volatile memory
#define PLD_FR 0x27 //Clear (Free) all queued payloads from non-volatile memory
#define GEO_WR 0x35 //Write geolocation longitude and latitude, and store in non-volatile memory
#define SAK_RR 0x45 //Read Acknowledgment
#define SAK_CR 0x46 //Confirm to the terminal that Acknowledgment was properly decoded and can be deleted by the terminal
#define CMD_RR 0x47 //Read a command message
#define CMD_CR 0x48 //Confirm to the module that the command was properly decoded and can be deleted by the module
#define RES_CR 0x55 //Clear reset event
#define TTX_SR 0x61 //Test Transmit Start Request
#define EVT_RR 0x65 //Reads the event register
#define PER_SR 0x66 //Context Save Request - recommended before cutting power
#define PER_RR 0x67 //Performance Counter Read Request
#define PER_CR 0x68 //Performance Counter Clear Request
#define MST_RR 0x69 //Module State Read Request
#define LCD_RR 0x6A //Last Contact Details Read Request
#define END_RR 0x6B //Environment Details Read Request to evaluate RF environment

//ANSWER (Terminal => Asset)
#define CFG_WA 0x85 //Answer last configuration write operation with status
#define WIF_WA 0x86 //Answer last Wi-Fi settings write operation with status (Wi-Fi only)
#define SSC_WA 0x87 //Answer last Satellite Search Configuration write operation
#define CFG_SA 0x90 //Answer last configuration save requests with status
#define CFG_FA 0x91 //Answer last factory reset request with status
#define CFG_RA 0x95 //Answer last configuration read operation with value
#define RTC_RA 0x97 //Answer last RTC read request with module time
#define NCO_RA 0x98 //Answer with the time to the next contact opportunity
#define MGI_RA 0x99 //Answer last GUID read with module GUID
#define MSN_RA 0x9A //Answer last Serial Number read with module Serial Number
#define MPN_RA 0x9B //Answer last Module Product Number read with the Product Number
#define PLD_EA 0xA5 //Answer last uplink payload enqueue operation with status
#define PLD_DA 0xA6 //Answer last uplink payload dequeue operation with status
#define PLD_FA 0xA7 //Answer last free queued payloads operation with status
#define GEO_WA 0xB5 //Answer last geolocation write operation with status
#define SAK_RA 0xC5 //Answer with Acknowledgment information
#define SAK_CA 0xC6 //Answer last SAK_CR confirmation
#define CMD_RA 0xC7 //Answer last CMD_RR with command data
#define CMD_CA 0xC8 //Answer last CMD_CR
#define RES_CA 0xD5 //Answer the reset clear request
#define EVT_RA 0xE5 //Answer indicates which events are currently pending
#define PER_SA 0xE6 //Answer confirming Context Save Request
#define PER_RA 0xE7 //Answer with Performance Counters in Type, Length, Value format
#define PER_CA 0xE8 //Answer confirming Performance Counter Clear Request
#define MST_RA 0xE9 //Answer with details of the current Module State
#define LCD_RA 0xEA //Answer with details of the Last Contact
#define END_RA 0xEB //Answer with details of the RF environment
#define ERR_RA 0xFF //Answer a request reporting an error

//Error code list
#define CRC_NOT_VALID 0x0001        //Discrepancy between provided CRC and expected CRC.
#define LENGTH_NOT_VALID 0x0011     //Message exceeds the maximum length for a frame.
#define OPCODE_NOT_VALID 0x0121     //Invalid Operation Code used.
#define ARG_NOT_VALID 0x0122        //Invalid argument used.
#define FLASH_WRITING_FAILED 0x0123 //Failed to write to the flash.
#define DEVICE_BUSY 0x0124          //Device is busy.
#define FORMAT_NOT_VALID 0x0601     //At least one of the fields (SSID, password, token) is not composed of exclusively printable standard ASCII characters (0x20 to 0x7E).
#define PERIOD_INVALID 0x0701       //The Satellite Search Config period enumeration value is not valid
#define BUFFER_FULL 0x2501          //Failed to queue the payload because the sending queue is already full
#define DUPLICATE_ID 0x2511         //Failed to queue the payload because the Payload ID provided by the asset is already in use in the terminal queue.
#define BUFFER_EMPTY 0x2601         //Failed to dequeue a payload from the buffer because the buffer is empty
#define INVALID_POS 0x3501          //Failed to update the geolocation information. Latitude and longitude fields must in the range [-90,90] degrees and [-180,180] degrees, respectively.
#define NO_ACK 0x4501               //No satellite acknowledgement available for any payload.
#define NO_ACK_CLEAR 0x4601         //No payload ack to clear, or it was already cleared.
#define NO_COMMAND 0x4701           //No command is available.
#define NO_COMMAND_CLEAR 0x4801     //No command to clear, or it was already cleared.
#define MAX_TX_REACHED 0x6101       //Failed to test Tx due to the maximum number of transmissions being reached.

//Escape characters
#define STX 0x02
#define ETX 0x03

//Command/Response size
#define COMMAND_MAX_SIZE 200

//Message queue description
#define ASN_MAX_MSG_SIZE 160
#define ASN_MSG_QUEUE_SIZE 8

//Functions return codes
#define ASN_NO_ERROR 0x00
#define ASN_ERROR_FAILED 0x01

//Satellite search period
#define SAT_SEARCH_DEFAULT 0
#define SAT_SEARCH_1377_MS 1
#define SAT_SEARCH_2755_MS 2
#define SAT_SEARCH_4132_MS 3
#define SAT_SEARCH_15150_MS 4
#define SAT_SEARCH_17905_MS 5
#define SAT_SEARCH_23414_MS 6

//Performance counter types
#define PER_CMD_LENGTH 90
#define PER_TYPE_SAT_SEARCH_PHASE_CNT 0x01
#define PER_TYPE_SAT_DETECT_OPERATION_CNT 0x02
#define PER_TYPE_SIGNAL_DEMOD_PHASE_CNT 0x03
#define PER_TYPE_SIGNAL_DEMOD_ATTEMPS_CNT 0x04
#define PER_TYPE_SIGNAL_DEMOD_SUCCESS_CNT 0x05
#define PER_TYPE_ACK_DEMOD_ATTEMPT_CNT 0x06
#define PER_TYPE_ACK_DEMOD_SUCCESS_CNT 0x07
#define PER_TYPE_QUEUED_MSG_CNT 0x08
#define PER_TYPE_DEQUEUED_UNACK_MSG_CNT 0x09
#define PER_TYPE_ACK_MSG_CNT 0x0A
#define PER_TYPE_SENT_FRAGMENT_CNT 0x0B
#define PER_TYPE_ACK_FRAGMENT_CNT 0x0C
#define PER_TYPE_CMD_DEMOD_ATTEMPT_CNT 0x0D
#define PER_TYPE_CMD_DEMOD_SUCCESS_CNT 0x0E

//Module state types
#define MST_CMD_LENGTH 9
#define MST_TYPE_MSG_IN_QUEUE 0x41
#define MST_TYPE_ACK_MSG_QUEUE 0x42
#define MST_TYPE_LAST_RST 0x43

//Environment details
#define END_CMD_LENGTH 12
#define END_TYPE_LAST_MAC_RESULT 0x61
#define END_TYPE_LAST_SAT_SEARCH_PEAK_RSSI 0x62
#define END_TYPE_TIME_SINCE_LAST_SAT_SEARCH 0x63

//Last contact details
#define LCD_CMD_LENGTH 21
#define LCD_TYPE_TIME_START_LAST_CONTACT 0x51
#define LCD_TYPE_TIME_END_LAST_CONTACT 0x52
#define LCD_TYPE_PEAK_RSSI_LAST_CONTACT 0x53
#define LCD_TYPE_TIME_PEAK_RSSI_LAST_CONTACT 0x54

//Events
#define EVENT_MSG_ACK 1      // A satellite payload acknowledgement is available to be read and confirmed
#define EVENT_RESET 2        // Module has reset
#define EVENT_CMD_RECEIVED 3 // A command is available to be read and confirmed
#define EVENT_MSG_PENDING 4  // An uplink message is present in the message queue, waiting to be sent, and module power should not be cut.
#define EVENT_NO_EVENT 0

//Device type
#define TYPE_ASTRONODE_S 3
#define TYPE_WIFI_DEVKIT 4

//Astrocast time
#define ASTROCAST_REF_UNIX_TIME 1514764800 //2018-01-01T00:00:00Z (= Astrocast time)

class ASTRONODE
{

private:
  //Global variables
  Stream *_serialPort;
  Stream *_debugSerial; //The stream to send debug messages to if enabled

  bool _printDebug = false;     //Flag to print the serial commands we are sending to the Serial port for debug
  bool _printFullDebug = false; //Flag to print full debug messages. Useful for UART debugging

  uint8_t com_buf_astronode[COMMAND_MAX_SIZE + 2];               //max cmd size + 2 bytes CRC
  uint8_t com_buf_astronode_hex[2 * (COMMAND_MAX_SIZE + 2) + 2]; //max cmd size + 2 bytes CRC -> double for trasport layer + add 2 escape characters

  //Functions prototype
  uint8_t encode_send_request(uint8_t reg, uint8_t *param, uint8_t param_length);
  uint16_t receive_decode_answer(uint8_t *param, uint8_t param_length);
  void byte_array_to_hex_array(uint8_t *in, uint8_t length, uint8_t *out);
  void hex_array_to_byte_array(uint8_t *in, uint8_t length, uint8_t *out);
  uint8_t nibble_to_hex(uint8_t nibble);
  uint8_t hex_to_nibble(uint8_t hex);
  uint16_t crc_compute(uint8_t *data, uint16_t data_length, uint16_t init);
  void print_error_code(uint16_t code);
  void print_array_to_hex(uint8_t data[], size_t length);

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

  typedef struct
  {
    uint32_t sat_search_phase_cnt;
    uint32_t sat_detect_operation_cnt;
    uint32_t signal_demod_phase_cnt;
    uint32_t signal_demod_attempt_cnt;
    uint32_t signal_demod_success_cnt;
    uint32_t ack_demod_attempt_cnt;
    uint32_t ack_demod_success_cnt;
    uint32_t queued_msg_cnt;
    uint32_t dequeued_unack_msg_cnt;
    uint32_t ack_msg_cnt;
    uint32_t sent_fragment_cnt;
    uint32_t ack_fragment_cnt;
    uint32_t cmd_demod_attempt_cnt;
    uint32_t cmd_demod_success_cnt;
  } ASTRONODE_PER_STRUCT;
  ASTRONODE_PER_STRUCT per_struct;

  typedef struct
  {
    uint8_t msg_in_queue;
    uint8_t ack_msg_in_queue;
    uint8_t last_rst;
  } ASTRONODE_MST_STRUCT;
  ASTRONODE_MST_STRUCT mst_struct;

  typedef struct
  {
    uint8_t last_mac_result;
    uint8_t last_sat_search_peak_rssi;
    uint32_t time_since_last_sat_search;
  } ASTRONODE_END_STRUCT;
  ASTRONODE_END_STRUCT end_struct;

  typedef struct
  {
    uint32_t time_start_last_contact;
    uint32_t time_end_last_contact;
    uint8_t peak_rssi_last_contact;
    uint32_t time_peak_rssi_last_contact;
  } ASTRONODE_LCD_STRUCT;
  ASTRONODE_LCD_STRUCT lcd_struct;

  //Functions prototype
  uint8_t begin(Stream &serialPort);
  void end();

  void enableDebugging(Stream &debugPort, bool printFullDebug);
  void disableDebugging(void);

  uint8_t configuration_write(bool with_pl_ack,
                              bool with_geoloc,
                              bool with_ephemeris,
                              bool with_deep_sleep,
                              bool with_ack_event_pin_mask,
                              bool with_reset_event_pin_mask);
  uint8_t configuration_read(void);
  uint8_t configuration_save(void);
  uint8_t wifi_configuration_write(const char *wland_ssid,
                                   const char *wland_key,
                                   const char *auth_token);
  uint8_t satellite_search_config_write(uint8_t search_period, bool force_search);
  uint8_t geolocation_write(int32_t lat, int32_t lon);
  uint8_t factory_reset(void);

  uint8_t guid_read(String *guid);
  uint8_t serial_number_read(String *sn);
  uint8_t product_number_read(String *pn);

  uint8_t rtc_read(uint32_t *time);
  uint8_t read_next_contact_opportunity(uint32_t *time_to_next_pass);
  uint8_t read_performance_counter(void);
  uint8_t save_performance_counter(void);
  uint8_t clear_performance_counter(void);
  uint8_t read_module_state(void);
  uint8_t read_environment_details(void);
  uint8_t read_last_contact_details(void);

  uint8_t enqueue_payload(uint8_t *data, uint8_t length, uint16_t id);
  uint8_t dequeue_payload(uint16_t *id);
  uint8_t clear_free_payloads(void);

  uint8_t read_command_8B(uint8_t data[8], uint32_t *createdDate);
  uint8_t read_command_40B(uint8_t data[40], uint32_t *createdDate);
  uint8_t clear_command(void);

  uint8_t event_read(uint8_t *event_type);
  uint8_t read_satellite_ack(uint16_t *id);

  uint8_t clear_satellite_ack(void);
  uint8_t clear_reset_event(void);

  void dummy_cmd(void);
};

#endif
