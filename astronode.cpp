/******************************************************************************************
*
* File:        astronode.cpp
* Author:      Raphael Valceschini
******************************************************************************************/
/****************************************************************************************
*
* Created on: 			01.04.2021
* Supported Hardware: Arduino MKR 1400
*
* Firmware Version 1.0
* First version
****************************************************************************************/

#include "astronode.h"

#define DEBUG

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

	//Send dummy command to reset communication (issue with Arduino lines ? no need if hardware reset first)
	dummy_cmd();

	return 1;
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
	uint8_t param_w[3] = {0x00, 0x00, 0x00};
	if (with_pl_ack)
	{
		param_w[0] |= 1 << 0;
	}
	if (with_geoloc)
	{
		param_w[0] |= 1 << 1;
	}
	if (with_ephemeris)
	{
		param_w[0] |= 1 << 2;
	}
	if (with_deep_sleep)
	{
		param_w[0] |= 1 << 3;
	}
	if (with_ack_event_pin_mask)
	{
		param_w[2] |= 1 << 0;
	}
	if (with_reset_event_pin_mask)
	{
		param_w[2] |= 1 << 1;
	}

#ifdef DEBUG
	Serial.print("Set config: ");
#endif

	//Send request
	if (encode_send_request(CFG_WR, param_w, sizeof(param_w)))
	{
		if (receive_decode_answer(NULL, 0) == CFG_WA)
		{

#ifdef DEBUG
			Serial.println("SUCCESS");
#endif

			return 1;
		}
		else
		{

#ifdef DEBUG
			Serial.println("FAILED");
#endif
		}
	}
	return 0;
}

uint8_t ASTRONODE::wifi_configuration_write(const char *wland_ssid, const char *wland_key, const char *auth_token)
{
	//Set parameters
	uint8_t param_w[194] = {'\0'};

	uint8_t wland_ssid_length = strlen(wland_ssid);
	uint8_t wland_key_length = strlen(wland_key);
	uint8_t auth_token_length = strlen(auth_token);

	memcpy(&param_w[0], wland_ssid, wland_ssid_length);
	memcpy(&param_w[33], wland_key, wland_key_length);
	memcpy(&param_w[97], auth_token, auth_token_length);

#ifdef DEBUG
	Serial.print("Wifi set config: ");
#endif

	//Send request
	if (encode_send_request(WIF_WR, param_w, sizeof(param_w)))
	{
		switch (receive_decode_answer(NULL, 0))
		{
		case WIF_WA:
		{
#ifdef DEBUG
			Serial.println("SUCCESS");
#endif

			return 1;
		}
		break;
		case FORMAT_NOT_VALID:
		{
#ifdef DEBUG
			Serial.println("FAILED - At least one of the fields (SSID, password, token) is not composed of exclusively printable standard ASCII characters (0x20 to 0x7E).");
#endif
		}
		break;
		case FLASH_WRITING_FAILED:
		{
#ifdef DEBUG
			Serial.println("FAILED - Failed to write the Wi-Fi settings (SSID, password, token) to the flash.");
#endif
		}
		break;
		}
	}
	return 0;
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

#ifdef DEBUG
			Serial.println("terminal configuration: ");
			Serial.print("    product_id: ");
			if (config.product_id == TYPE_ASTRONODE_S)
			{
				Serial.println("Commercial Satellite Astronode");
			}
			else if (config.product_id == TYPE_WIFI_DEVKIT)
			{
				Serial.println("Commercial Wi-Fi Dev Kit");
			}
			Serial.print("    hardware revision: ");
			Serial.println(config.hardware_rev);
			Serial.print("    firmware version: ");
			Serial.print(config.firmware_maj_ver);
			Serial.print(".");
			Serial.print(config.firmware_min_ver);
			Serial.print(".");
			Serial.println(config.firmware_rev);
			Serial.print("    with payload ack (1=yes,0=no): ");
			Serial.println(config.with_pl_ack);
			Serial.print("    with geolocation (1=yes,0=no): ");
			Serial.println(config.with_geoloc);
			Serial.print("    with ephemeris (1=yes,0=no): ");
			Serial.println(config.with_ephemeris);
			Serial.print("    with deep sleep mode enable (1=yes,0=no): ");
			Serial.println(config.with_deep_sleep_en);
			Serial.print("    with message ack pin enable (1=yes,0=no): ");
			Serial.println(config.with_msg_ack_pin_en);
			Serial.print("    with message reset pin enable (1=yes,0=no): ");
			Serial.println(config.with_msg_reset_pin_en);
#endif

			return 1;
		}
	}
	return 0;
}

uint8_t ASTRONODE::enqueue_payload(uint8_t *data, uint8_t length, uint16_t id)
{
	//Set parameters
	uint8_t param_w[160 + 2];
	uint8_t param_a[2]; // Really useful ? Redundant info...

#ifdef DEBUG
	Serial.print("Queuing payload with ID #");
	Serial.print(id);
	Serial.print(": ");
#endif

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
#ifdef DEBUG
				Serial.println("SUCCESS");
#endif

				return 1;
			}
			else
			{
#ifdef DEBUG
				Serial.println("FAILED - ID mismatch terminal <-> asset. Please dequeue payload...");
#endif
			}
		}
		break;
		case DUPLICATE_ID:
		{
#ifdef DEBUG
			Serial.println("FAILED - Failed to queue the payload because the Payload ID provided by the asset is already in use in the terminal queue.");
#endif
		}
		break;
		case LENGTH_NOT_VALID:
		{
#ifdef DEBUG
			Serial.println("FAILED - Message exceeds the maximum length for a frame.");
#endif
		}
		break;
		case BUFFER_FULL:
		{
#ifdef DEBUG
			Serial.println("FAILED - Failed to queue the payload because the sending queue is already full.");
#endif
		}
		break;
		}
	}
	return 0;
}

uint8_t ASTRONODE::dequeue_payload(uint16_t *id)
{
	//Set parameters
	uint8_t param_a[2];

#ifdef DEBUG
	Serial.print("Remove oldest payload from queue: ");
#endif

	//Send request
	if (encode_send_request(PLD_DR, NULL, 0))
	{
		switch (receive_decode_answer(param_a, sizeof(param_a)))
		{
		case PLD_DA:
		{
			*id = (((uint16_t)param_a[1]) << 8) + ((uint16_t)param_a[0]);

#ifdef DEBUG
			Serial.print("SUCCESS - Removed payload with ID #");
			Serial.print(*id);
			Serial.print("from terminal's queue");
#endif

			return 1;
		}
		break;
		case BUFFER_EMPTY:
		{
#ifdef DEBUG
			Serial.println("FAILED - Failed to dequeue a payload from the buffer because the buffer is empty.");
#endif
		}
		break;
		}
	}
	return 0;
}

uint8_t ASTRONODE::clear_free_payloads(void)
{
	//Set parameters

#ifdef DEBUG
	Serial.print("Clear/free entire terminal's queue: ");
#endif

	//Send request
	if (encode_send_request(PLD_FR, NULL, 0))
	{
		if (receive_decode_answer(NULL, 0) == PLD_FA)
		{

#ifdef DEBUG
			Serial.println("SUCCESS");
#endif

			return 1;
		}
	}
	return 0;
}

uint8_t ASTRONODE::geolocation_write(int32_t lat, int32_t lon)
{
	//Set parameters
	uint8_t param_w[8];

	memcpy(&param_w[0], &lat, sizeof(lat));
	memcpy(&param_w[4], &lon, sizeof(lon));

#ifdef DEBUG
	Serial.print("Set geolocation: ");
#endif

	//Send request
	if (encode_send_request(GEO_WR, param_w, sizeof(param_w)))
	{
		switch (receive_decode_answer(NULL, 0))
		{
		case GEO_WA:
		{
#ifdef DEBUG
			Serial.println("SUCCESS");
#endif

			return 1;
		}
		break;
		case INVALID_POS:
		{
#ifdef DEBUG
			Serial.println("FAILED - Invalid position");
#endif
		}
		break;
		}
	}
	return 0;
}

uint8_t ASTRONODE::read_satellite_ack(uint16_t *id)
{
	//Set parameters
	uint8_t param_a[2];

#ifdef DEBUG
	Serial.print("Satellite acknowledge received with ID #: ");
#endif

	//Send request
	if (encode_send_request(SAK_RR, NULL, 0))
	{
		if (receive_decode_answer(param_a, sizeof(param_a)) == SAK_RA)
		{

			*id = (((uint16_t)param_a[1]) << 8) + (uint16_t)(param_a[0]);

#ifdef DEBUG
			Serial.println(*id);
#endif

			return 1;
		}
		else
		{
#ifdef DEBUG
			Serial.println("FAILED - no message acknowledged");
#endif
		}
	}
	return 0;
}

uint8_t ASTRONODE::clear_satellite_ack(void)
{
	//Set parameters

#ifdef DEBUG
	Serial.print("Clear sat ACK: ");
#endif

	//Send request
	if (encode_send_request(SAK_CR, NULL, 0))
	{
		if (receive_decode_answer(NULL, 0) == SAK_CA)
		{

#ifdef DEBUG
			Serial.println("Satellite acknowledge cleared.");
#endif

			return 1;
		}
	}
	return 0;
}

uint8_t ASTRONODE::clear_reset_event(void)
{
	//Set parameters
#ifdef DEBUG
	Serial.print("Clear RESET event: ");
#endif

	//Send request
	if (encode_send_request(RES_CR, NULL, 0))
	{
		if (receive_decode_answer(NULL, 0) == RES_CA)
		{

#ifdef DEBUG
			Serial.println("RESET event cleared.");
#endif

			return 1;
		}
	}
	return 0;
}

uint8_t ASTRONODE::event_read(uint8_t *event_type)
{
	//Set parameters
	uint8_t param_a;

#ifdef DEBUG
	Serial.print("Read EVENT: ");
#endif

	//Send request
	if (encode_send_request(EVT_RR, NULL, 0))
	{
		if (receive_decode_answer(&param_a, sizeof(param_a)) == EVT_RA)
		{
			if (param_a & (1 << 0))
			{
#ifdef DEBUG
				Serial.println("Satellite Acknowledgment (SAK) Available.");
#endif
				*event_type = EVENT_SAK;
			}
			else if (param_a & (1 << 1))
			{
#ifdef DEBUG
				Serial.println("Terminal has reset.");
#endif
				*event_type = EVENT_RESET;
			}
			else
			{
#ifdef DEBUG
        Serial.println("No event");
#endif
				*event_type = EVENT_NO_EVENT;
			}
			return 1;
		}
	}
	return 0;
}

uint8_t ASTRONODE::guid_read(String *guid)
{
	//Set parameters
	uint8_t param_a[36];

#ifdef DEBUG
	Serial.print("GUID: ");
#endif

	//Send request
	if (encode_send_request(DGI_RR, NULL, 0))
	{
		if (receive_decode_answer(param_a, sizeof(param_a)) == DGI_RA)
		{
			for (uint8_t i = 0; i < sizeof(param_a); i++)
			{
				guid->concat((char)param_a[i]);
			}

#ifdef DEBUG
			Serial.println(*guid);
#endif

			return 1;
		}
	}
	return 0;
}

uint8_t ASTRONODE::serial_number_read(String *sn)
{
	//Set parameters
	uint8_t param_a[16];

#ifdef DEBUG
	Serial.print("Serial number: ");
#endif

	//Send request
	if (encode_send_request(DSN_RR, NULL, 0))
	{
		if (receive_decode_answer(param_a, sizeof(param_a)) == DSN_RA)
		{
			for (uint8_t i = 0; i < sizeof(param_a); i++)
			{
				sn->concat((char)param_a[i]);
			}

#ifdef DEBUG
			Serial.println(*sn);
#endif

			return 1;
		}
	}
	return 0;
}

uint8_t ASTRONODE::rtc_read(uint32_t *time)
{
	//Set parameters
	uint8_t param_a[4];

#ifdef DEBUG
	Serial.print("RTC time: ");
#endif

	//Send request
	if (encode_send_request(RTC_RR, NULL, 0))
	{
		if (receive_decode_answer(param_a, sizeof(param_a)) == RTC_RA)
		{
			uint32_t time_tmp = (((uint32_t)param_a[3]) << 24) + (((uint32_t)param_a[2]) << 16) + (((uint32_t)param_a[1]) << 8) + (uint32_t)(param_a[0]);

			*time = time_tmp + 1514761200; //2018-01-01T00:00:00Z

#ifdef DEBUG
			Serial.println(*time);
#endif

			return 1;
		}
	}
	return 0;
}

uint8_t ASTRONODE::factory_reset(void)
{
	//Set parameters

#ifdef DEBUG
	Serial.print("Save config: ");
#endif

	//Send request
	if (encode_send_request(CFG_FR, NULL, 0))
	{
		if (receive_decode_answer(NULL, 0) == CFG_FA)
		{

#ifdef DEBUG
			Serial.println("SUCCES");
#endif

			return 1;
		}
		else
		{
#ifdef DEBUG
			Serial.println("FAILED");
#endif
		}
	}
	return 0;
}

uint8_t ASTRONODE::configuration_save(void)
{
	//Set parameters

#ifdef DEBUG
	Serial.print("Save config: ");
#endif

	//Send request
	if (encode_send_request(CFG_SR, NULL, 0))
	{
		if (receive_decode_answer(NULL, 0) == CFG_SA)
		{

#ifdef DEBUG
			Serial.println("SUCCES");
#endif

			return 1;
		}
		else
		{
#ifdef DEBUG
			Serial.println("FAILED");
#endif
		}
	}
	return 0;
}

void ASTRONODE::dummy_cmd(void)
{
  encode_send_request(0x00, NULL, 0);
  receive_decode_answer(NULL, 0);
}

uint8_t ASTRONODE::encode_send_request(uint8_t reg, uint8_t *param, uint8_t param_length)
{
	//Flush incoming buffer (TODO: needed ?)
	while (_serialPort->available())
	{
		_serialPort->read();
	}

	//Copy command in buffer
	command_to_astronode[0] = reg;
	memcpy(&command_to_astronode[1], param, param_length);

	//Compute CRC
	uint16_t cmd_crc = crc_compute(command_to_astronode, param_length + 1, 0xFFFF);
	memcpy(&command_to_astronode[1 + param_length], &cmd_crc, sizeof(cmd_crc));

	/*
	#ifdef DEBUG
	Serial.print("asset -> terminal (+ CRC): ");
	printData(command_to_astronode, 1 + param_length + 2);
	#endif
	*/

	//Translate to hexadecimal
	byte_array_to_hex_array(command_to_astronode, 1 + param_length + 2, &command_to_astronode_hex[1]);

	//Add escape characters
	command_to_astronode_hex[0] = STX;
	command_to_astronode_hex[2 * (1 + param_length + 2) + 1] = ETX;

	/*#ifdef DEBUG
	Serial.print("asset -> terminal (+ CRC + HEX encoding): ");
	printData(command_to_astronode_hex, 2 * (1 + param_length + 2) + 2);
	#endif*/

	//Write command
	if (_serialPort->write(command_to_astronode_hex, 2 * (1 + param_length + 2) + 2) == (size_t)(2 * (1 + param_length + 2) + 2))
	{
		return 1;
	}
	return 0;
}

uint16_t ASTRONODE::receive_decode_answer(uint8_t *param, uint8_t param_length)
{
	//Read answer
	size_t rx_length = _serialPort->readBytesUntil(ETX, answer_from_astronode_hex, 2 * RESPONSE_MAX_SIZE);

	if (rx_length)
	{ //Only with for escape character

		/*#ifdef DEBUG
		Serial.print("terminal -> asset (+ CRC + HEX encoding): ");
		printData(answer_from_astronode_hex, rx_length);
		#endif*/

		//Translate to binary
		hex_array_to_byte_array(&answer_from_astronode_hex[1], rx_length, answer_from_astronode);

		/*
		#ifdef DEBUG
		Serial.print("terminal -> asset (+ CRC): ");
		printData(answer_from_astronode, rx_length >> 1);
		#endif
		*/

		//Verify CRC
		uint16_t cmd_crc = crc_compute(answer_from_astronode, (rx_length >> 1) - 2, 0xFFFF);
		uint16_t cmd_crc_check = (((uint16_t)answer_from_astronode[(rx_length >> 1) - 2 + 1]) << 8) + (uint16_t)(answer_from_astronode[(rx_length >> 1) - 2]);

		if (cmd_crc == cmd_crc_check)
		{

			if (answer_from_astronode[0] == ERROR)
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
#ifdef DEBUG
			Serial.println("Failed to check CRC - frame is not valid");
#endif
		}
	}
	else
	{
#ifdef DEBUG
		Serial.println("Failed to receive data from astronode before timeout");
#endif
	}
	return 0;
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

void ASTRONODE::printData(uint8_t data[], size_t length)
{
	Serial.println("uint8_t message[] = {");
	for (size_t i = 0; i < length; i++)
	{
		Serial.print("0x");
		if ((data[i] >> 4) == 0)
			Serial.print("0"); // print preceeding high nibble if it's zero
		Serial.print(data[i], HEX);
		if (i < (length - 1))
			Serial.print(", ");
		if ((31 - i) % 16 == 0)
			Serial.println();
	}
	Serial.println("};\n\r");
}
