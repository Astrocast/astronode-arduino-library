/*
  This example enqueue a single message in the Astronode S terminal. 
  It checks periodically if a new event is available. 
  If a SAK event is receieved, a new message is enqueued.

  The circuit:
  - Arduino board
  - Astronode S devkit (sat or wifi) attached
*/

#include <astronode.h>


//#include <SoftwareSerial.h>
//SoftwareSerial ASTRONODE_SERIAL(2, 3);  // RX, TX
HardwareSerial Serial1(PA10, PA9);
#define ASTRONODE_SERIAL Serial1

#define ASTRONODE_SERIAL_BAUDRATE 9600

#define ASTRONODE_WLAN_SSID "YOUR SSID"
#define ASTRONODE_WLAN_KEY "YOUR PWD"
#define ASTRONODE_AUTH_TOKEN "YOUT TOKEN"

#define ASTRONODE_GEO_LAT 0
#define ASTRONODE_GEO_LON 0

#define ASTRONODE_WITH_PLD_ACK true
#define ASTRONODE_WITH_GEO_LOC true
#define ASTRONODE_WITH_EPHEMERIS true
#define ASTRONODE_WITH_DEEP_SLEEP_EN false
#define ASTRONODE_WITH_MSG_ACK_PIN_EN true
#define ASTRONODE_WITH_MSG_RESET_PIN_EN true

uint8_t data[17] = {"Hello Astrocast!"};
uint16_t counter = 0;

ASTRONODE astronode;

void setup()
{
  Serial.begin(115200);
  while (! Serial);

  ASTRONODE_SERIAL.begin(ASTRONODE_SERIAL_BAUDRATE);

  if (astronode.begin(ASTRONODE_SERIAL))
  {
    //Write configuration
    astronode.configuration_write(ASTRONODE_WITH_PLD_ACK,
                                  ASTRONODE_WITH_GEO_LOC,
                                  ASTRONODE_WITH_EPHEMERIS,
                                  ASTRONODE_WITH_DEEP_SLEEP_EN,
                                  ASTRONODE_WITH_MSG_ACK_PIN_EN,
                                  ASTRONODE_WITH_MSG_RESET_PIN_EN);

    //Set geolocation
    astronode.geolocation_write(ASTRONODE_GEO_LAT, ASTRONODE_GEO_LON);

    //Read configuration
    astronode.configuration_read();

    //If Wifi devkit, set credential info
    if (astronode.config.product_id == TYPE_WIFI_DEVKIT)
    {
      //Set WiFi configuration:
      astronode.wifi_configuration_write(ASTRONODE_WLAN_SSID, ASTRONODE_WLAN_KEY, ASTRONODE_AUTH_TOKEN);
    }

    //Clean terminal queue in case older messages were still present
    astronode.clear_free_payloads();

    //Try enqueueing a first message in the queue
    if (astronode.enqueue_payload(data, sizeof(data), counter)) {
      counter++;
    }
  }
  else {
    while (1);
  }
}

void loop()  {

  //Poll for new events
  uint8_t event_type;
  astronode.event_read(&event_type);

  if (event_type == EVENT_SAK)
  {
    uint16_t counter_read = 0;
    if (astronode.read_satellite_ack(&counter_read))
    {
      astronode.clear_satellite_ack();
	  
	  //Enqueue new message
      astronode.enqueue_payload(data, sizeof(data), counter);
      counter++;
    }
  }
  else if (event_type == EVENT_RESET)
  {
    astronode.clear_reset_event();
  }

  delay(10000);
}