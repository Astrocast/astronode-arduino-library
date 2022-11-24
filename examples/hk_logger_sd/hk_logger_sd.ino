/*
  This example periodically log all housekeeping from the module on an SD card (peformance counters, env. variables, etc.)
  It enqueues a single message in the queue of the Astronode S.
  It checks periodically if a new event is available.
  If a "satellite acknowledge" event is receieved, the ACK is cleared and a new message is enqueued in the module.
  If a "command received" event is receieved, the data contained in the command are written to the SD card and the command is cleared.
  When all tasks are done, the arduino go in deep sleep.
  
  The circuit:
  - Adafruit Feather M0 Adalogger (TX -> D16(RX), RX -> D15 (TX), GND -> GND, 3V3 -> 3V3)
  - Astronode S devkit (sat or wifi) attached
*/

#include <astronode.h>
#include <Adafruit_SleepyDog.h>
#include <SPI.h>
#include <SD.h>

//System configuration
#define MAIN_LOOP_PERIOD 3000      //[ms]
#define MAIN_LOOP_WDT_PERIOD 16000 //[ms]

// Pin mapping
#define PIN_SD_CS 4
#define PIN_SD_CD 7
#define PIN_LED_STATUS LED_BUILTIN

// Astronode configuration
#define ASTRONODE_SERIAL Serial1
#define ASTRONODE_SERIAL_BAUDRATE 9600
#define ASTRONODE_WITH_PLD_ACK true
#define ASTRONODE_WITH_GEO_LOC false
#define ASTRONODE_WITH_EPHEMERIS false
#define ASTRONODE_WITH_DEEP_SLEEP_EN false
#define ASTRONODE_WITH_MSG_ACK_PIN_EN false
#define ASTRONODE_WITH_MSG_RESET_PIN_EN false
#define ASTRONODE_WITH_CMD_EVENT_PIN_EN true
#define ASTRONODE_WITH_TX_PEND_EVENT_PIN_EN false
#define ASTRONODE_PAYLOAD_SIZE 80 //[Bytes]

ASTRONODE astronode;
File logFile;

bool save_hk_to_sd(uint32_t timestamp);
bool save_command_to_sd(uint32_t timestamp, uint8_t data[40], uint32_t createdDate);

void setup()
{
    //Initialize watchdog:
    Watchdog.enable(MAIN_LOOP_WDT_PERIOD);

    //Initialize GPIOs
    pinMode(PIN_LED_STATUS, OUTPUT);
    pinMode(PIN_SD_CD, INPUT_PULLUP);
    digitalWrite(PIN_LED_STATUS, LOW);

    //Initialize terminal
    ASTRONODE_SERIAL.begin(ASTRONODE_SERIAL_BAUDRATE);
    if (astronode.begin(ASTRONODE_SERIAL) != ANS_STATUS_SUCCESS)
    {
        while (1)
            ;
    }
    astronode.configuration_write(ASTRONODE_WITH_PLD_ACK,
                                  ASTRONODE_WITH_GEO_LOC,
                                  ASTRONODE_WITH_EPHEMERIS,
                                  ASTRONODE_WITH_DEEP_SLEEP_EN,
                                  ASTRONODE_WITH_MSG_ACK_PIN_EN,
                                  ASTRONODE_WITH_MSG_RESET_PIN_EN,
                                  ASTRONODE_WITH_CMD_EVENT_PIN_EN,
                                  ASTRONODE_WITH_TX_PEND_EVENT_PIN_EN);
    astronode.satellite_search_config_write(SAT_SEARCH_2755_MS, true);
    astronode.configuration_save();
    astronode.configuration_read();

    //Initialize SD card
    if (!SD.begin(PIN_SD_CS))
    {
        while (1)
            ;
    }
}

void loop()
{
    // Show operation status
    digitalWrite(PIN_LED_STATUS, HIGH);

    //Querry HK and RTC
    uint32_t rtc_time;
    if (astronode.read_performance_counter() == ANS_STATUS_SUCCESS &&
        astronode.read_module_state() == ANS_STATUS_SUCCESS &&
        astronode.read_environment_details() == ANS_STATUS_SUCCESS &&
        astronode.read_last_contact_details() == ANS_STATUS_SUCCESS &&
        astronode.rtc_read(&rtc_time) == ANS_STATUS_SUCCESS)
    {
        //Save to SD card
        if (digitalRead(PIN_SD_CD) == LOW)
        {
            while (1)
                ;
        }
        else
        {
            save_hk_to_sd(rtc_time);
        }
    }

    //Querry and process events
    uint8_t event_type;
    astronode.event_read(&event_type);
    switch (event_type)
    {
    case EVENT_MSG_ACK:
    {
        //If satellite ACK event, read and clear event to be able to queue a new message
        uint16_t counter_read = 0;
        if (astronode.read_satellite_ack(&counter_read) == ANS_STATUS_SUCCESS)
        {
            astronode.clear_satellite_ack();
        }
        break;
    }
    case EVENT_RESET:
    {
        //If reset event, clear event
        astronode.clear_reset_event();
        break;
    }
    case EVENT_CMD_RECEIVED:
    {
        //If command event, read command, write command to SD card, clear command
        uint8_t data[40] = {0};
        uint32_t createdDate = 0;
        if (astronode.read_command_40B(data, &createdDate) == ANS_STATUS_SUCCESS &&
            astronode.rtc_read(&rtc_time) == ANS_STATUS_SUCCESS)
        {
            if (save_command_to_sd(rtc_time, data, createdDate))
            {
                astronode.clear_command();
            }
        }
        break;
    }
    }

    // Try to enqueue a new message
    uint8_t data[ASTRONODE_PAYLOAD_SIZE];
    astronode.enqueue_payload(data, sizeof(data), 1);

    // Show operation status
    digitalWrite(PIN_LED_STATUS, LOW);

    //Go in deep sleep
    Watchdog.sleep(MAIN_LOOP_PERIOD);

    //Re-activate watchdog
    Watchdog.enable(MAIN_LOOP_WDT_PERIOD);
}

bool save_hk_to_sd(uint32_t timestamp)
{
    // Write log of operation to SD card
    char filename[12] = {}; //8.3 filename convention
    sprintf(filename, "%lu", timestamp / 86400);
    strcat(filename, ".csv");

    logFile = SD.open(filename, FILE_WRITE);

    if (logFile)
    {
        // Timestamp
        logFile.print(F("HK;"));
        logFile.print(timestamp);
        logFile.print(F(";"));

        // Performance counters
        logFile.print(astronode.per_struct.sat_search_phase_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.sat_detect_operation_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.signal_demod_phase_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.signal_demod_attempt_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.signal_demod_success_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.ack_demod_attempt_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.ack_demod_success_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.queued_msg_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.dequeued_unack_msg_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.ack_msg_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.sent_fragment_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.ack_fragment_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.cmd_demod_attempt_cnt);
        logFile.print(F(";"));
        logFile.print(astronode.per_struct.cmd_demod_success_cnt);
        logFile.print(F(";"));

        // Module state
        logFile.print(astronode.mst_struct.msg_in_queue);
        logFile.print(F(";"));
        logFile.print(astronode.mst_struct.ack_msg_in_queue);
        logFile.print(F(";"));
        logFile.print(astronode.mst_struct.last_rst);
        logFile.print(F(";"));

        // Environment details
        logFile.print(astronode.end_struct.last_mac_result);
        logFile.print(F(";"));
        logFile.print(astronode.end_struct.last_sat_search_peak_rssi);
        logFile.print(F(";"));
        logFile.print(astronode.end_struct.time_since_last_sat_search);
        logFile.print(F(";"));

        // Last contact details
        logFile.print(astronode.lcd_struct.time_start_last_contact);
        logFile.print(F(";"));
        logFile.print(astronode.lcd_struct.time_end_last_contact);
        logFile.print(F(";"));
        logFile.print(astronode.lcd_struct.peak_rssi_last_contact);
        logFile.print(F(";"));
        logFile.println(astronode.lcd_struct.time_peak_rssi_last_contact);
    }
    else
    {
        return false;
    }

    logFile.close();

    return true;
}

bool save_command_to_sd(uint32_t timestamp, uint8_t data[40], uint32_t createdDate)
{
    // Write log of operation to SD card
    char filename[12] = {}; //8.3 filename convention
    sprintf(filename, "%lu", timestamp / 86400);
    strcat(filename, ".csv");

    logFile = SD.open(filename, FILE_WRITE);

    if (logFile)
    {
        // Timestamp
        logFile.print(F("CMD;"));
        logFile.print(timestamp);
        logFile.print(F(";"));

        // Write command
        logFile.print(createdDate);
        logFile.print(F(";"));

        for (size_t i = 0; i < 40; i++)
        {
            if ((data[i] >> 4) == 0)
            {
                logFile.print("0"); // print preceeding high nibble if it's zero
            }
            logFile.print(data[i], HEX);
            if (i < (40 - 1))
            {
                logFile.print("-");
            }
        }

        logFile.println();
    }
    else
    {
        return false;
    }

    logFile.close();

    return true;
}