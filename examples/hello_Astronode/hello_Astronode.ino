/*
  This example enqueue a single message in the queue of the Astronode S.
  It checks periodically if a new event is available.
  If a "satellite acknowledge" event is receieved, a new message is enqueued in the module.

  The circuit:
  - Nucleo-64 STM32l476 (TX -> D2(PA10), RX -> D8(PA9), GND -> GND, 3V3 -> 3V3)
  - Arduino MKR1400 (TX -> D13(RX), RX -> D14 (TX), GND -> GND, 3V3 -> VCC)
  - Arduino UNO (TX -> D2(with level shifter), RX -> D3(with level shifter), GND -> GND, 3V3 -> VCC)
  - Astronode S devkit (sat or wifi) attached
*/

#include <astronode.h>

#if defined ARDUINO_NUCLEO_L476RG
HardwareSerial Serial1(PA10, PA9);
#define ASTRONODE_SERIAL Serial1

#elif defined(__SAMD21G18A__)
#define ASTRONODE_SERIAL Serial1

#else
#include <SoftwareSerial.h>
SoftwareSerial ASTRONODE_SERIAL(2, 3);  // RX, TX
#endif

#define ASTRONODE_SERIAL_BAUDRATE 9600

#define ASTRONODE_WLAN_SSID "YOUR SSID"
#define ASTRONODE_WLAN_KEY "YOUR PWD"
#define ASTRONODE_AUTH_TOKEN "YOUR TOKEN"

#define ASTRONODE_GEO_LAT 0.0
#define ASTRONODE_GEO_LON 0.0

#define ASTRONODE_WITH_PLD_ACK true
#define ASTRONODE_WITH_GEO_LOC true
#define ASTRONODE_WITH_EPHEMERIS true
#define ASTRONODE_WITH_DEEP_SLEEP_EN false
#define ASTRONODE_WITH_MSG_ACK_PIN_EN false
#define ASTRONODE_WITH_MSG_RESET_PIN_EN false
#define ASTRONODE_WITH_CMD_EVENT_PIN_EN true
#define ASTRONODE_WITH_TX_PEND_EVENT_PIN_EN false

uint8_t data[17] = {"Hello Astrocast!"};
uint16_t counter = 0;

ASTRONODE astronode;

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  
  //Enable debugging messages on Astronode S library
  astronode.enableDebugging(Serial, false);

  ASTRONODE_SERIAL.begin(ASTRONODE_SERIAL_BAUDRATE);

  //Initialize terminal
  astronode.begin(ASTRONODE_SERIAL);

  //Write configuration
  astronode.configuration_write(ASTRONODE_WITH_PLD_ACK,
                                ASTRONODE_WITH_GEO_LOC,
                                ASTRONODE_WITH_EPHEMERIS,
                                ASTRONODE_WITH_DEEP_SLEEP_EN,
                                ASTRONODE_WITH_MSG_ACK_PIN_EN,
                                ASTRONODE_WITH_MSG_RESET_PIN_EN,
                                ASTRONODE_WITH_CMD_EVENT_PIN_EN,
                                ASTRONODE_WITH_TX_PEND_EVENT_PIN_EN);

  //Set geolocation
  astronode.geolocation_write((int32_t)(ASTRONODE_GEO_LAT * 10000000), (int32_t)(ASTRONODE_GEO_LON * 10000000));

  //Save configuration
  astronode.configuration_save();

  //Read configuration
  astronode.configuration_read();

  //If Wifi devkit, set credential info
  if (astronode.config.product_id == TYPE_WIFI_DEVKIT)
  {
    //Set WiFi configuration:
    astronode.wifi_configuration_write(ASTRONODE_WLAN_SSID, ASTRONODE_WLAN_KEY, ASTRONODE_AUTH_TOKEN);
  }

  // Get serial number
  String sn;
  astronode.serial_number_read(&sn);

  //Get GUID
  String guid;
  astronode.guid_read(&guid);

  //Clear old messages
  astronode.clear_free_payloads();
  
  //Try enqueueing a first message in the queue
  if (astronode.enqueue_payload(data, sizeof(data), counter) == ANS_STATUS_SUCCESS) {
    counter++;
  }
}

void loop()  {

  //Querry RTC time
  uint32_t rtc_time;
  astronode.rtc_read(&rtc_time);

  //Poll for new events
  uint8_t event_type;
  astronode.event_read(&event_type);

  if (event_type == EVENT_MSG_ACK)
  {
    uint16_t counter_read = 0;
    if (astronode.read_satellite_ack(&counter_read) == ANS_STATUS_SUCCESS)
    {
      astronode.clear_satellite_ack();
      astronode.enqueue_payload(data, sizeof(data), counter);
      counter++;
    }
  }
  else if (event_type == EVENT_RESET)
  {
    astronode.clear_reset_event();
    astronode.enqueue_payload(data, sizeof(data), counter);
    counter++;
  }

  delay(10000);
}