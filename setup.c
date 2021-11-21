#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/crc16.h>
#include <stdio.h>	// sprintf_P
#include <stdlib.h>	// strtol
#include <errno.h>	// errno
#include <limits.h>	// LONG_MIN
#include <ctype.h>	// isalnum
#include <string.h>	// strlen
#include "setup.h"
#include "usart.h"

#define	EEPROM_START_ADDRESS	((uint8_t*)(8))

/*****************************************************************************/
void
setup_send(	const char*	s)
{
	for (; *s != 0; ++s) {
		uart2_PutChar(*s);
	}
}

/*****************************************************************************/
void
setup_send_P(			PGM_P	s)
{
	for (;;) {
		const char c = pgm_read_byte(s++);
		if (c==0) {
			break;
		}
		setup_send_char(c);
	}
}

/*****************************************************************************/
static uint8_t
hexchar_of_int(		const uint8_t	ii)
{
	return ii<10 ? ii + '0' : ii + 'A' - 10;
}

/*****************************************************************************/
void
setup_send_hex(			const uint8_t	x)
{
	setup_send_char(hexchar_of_int(x>>4));
	setup_send_char(hexchar_of_int(x&0x0F));
}

/*****************************************************************************/
void
setup_send_newline()
{
	setup_send_P(PSTR("\r\n"));
}

/*****************************************************************************/
void
setup_send_integer(
	PGM_P		name,
	const int32_t	i,
	PGM_P		unit)
{
	char	xbuf[14];
	setup_send_P(name);
	setup_send_P(PSTR(" = "));
	sprintf_P(xbuf, PSTR("%ld"), i);
	setup_send(xbuf);
	setup_send_P(unit);
	setup_send_newline();
}

/*****************************************************************************/
void
setup_send_boolean(
	PGM_P		name,
	const bool 	b)
{
	setup_send_P(name);
	setup_send_P(PSTR(" = "));
	if (b) {
		setup_send_P(PSTR("true"));
	} else {
		setup_send_P(PSTR("false"));
	}
	setup_send_newline();
}

/*****************************************************************************/
void
setup_send_string(
	PGM_P		name,
	const char*	s)
{
	setup_send_P(name);
	setup_send_P(PSTR(" = "));
	setup_send(s);
	setup_send_newline();
}

/*****************************************************************************/
uint8_t
setup_crc(const SETUP* setup)
{
	uint8_t		r = 0;
	uint8_t		i;
	for (i=0; i<sizeof(*setup); ++i) {
		r = _crc_ibutton_update(r, ((const uint8_t*)setup)[i]);
	}
	return r;
}

/*****************************************************************************/
static void
setup_print(const SETUP* setup)
{
	setup_send_P(      PSTR("N  NAME             VALUE\r\n"));
	setup_send_boolean(PSTR("0: Realtime show   "), setup->realtime_show);
	setup_send_integer(PSTR("1: Pulse length    "), setup->pulse_length, PSTR("ms."));
	setup_send_integer(PSTR("2: Pulse offset    "), setup->pulse_offset, PSTR("ms."));
	setup_send_integer(PSTR("3: Offset limit    "), setup->offset_limit, PSTR("ms."));
	setup_send_integer(PSTR("4: Jump limit      "), setup->jump_limit, PSTR("ms."));
	setup_send_integer(PSTR("5: Reaction speed  "), setup->reaction_speed, PSTR("%."));
	setup_send_string(PSTR("6: Compass sentence"), setup->compass_sentence);
	setup_send_P(PSTR("Set new values as follows: N VALUE\r\n"));
	setup_send_P(PSTR("Realtime show is toggled, no value needed. For example, set pulse length to 100ms:\r\n"));
	setup_send_P(PSTR("1 100"));
}

/*****************************************************************************/
bool
setup_load_from_nvram(SETUP* setup)
{
	// 1. Read from eeprom.
	const uint8_t		crc2 = eeprom_read_byte((uint8_t*)(EEPROM_START_ADDRESS));
	eeprom_read_block(
		(uint8_t*)(setup),
		(void*)(EEPROM_START_ADDRESS+1), sizeof(*setup));

	// 2. Check CRC.
	if (setup_crc(setup) == crc2) {
		setup_print(setup);
		return true;
	} else {
		setup->realtime_show = true;
		setup->pulse_length = 100;
		setup->pulse_offset = 0;
		setup->offset_limit = 10;
		setup->jump_limit = 2000;
		setup->reaction_speed = 10;
		strcpy_P(setup->compass_sentence, PSTR("HDHDT"));
		setup_print(setup);
		return false;
	}
}

/*****************************************************************************/
void
setup_store_to_nvram(const SETUP* setup)
{
	// 2. Write to the eeprom.
	eeprom_write_byte(((uint8_t*)(EEPROM_START_ADDRESS)), setup_crc(setup));
	eeprom_write_block(
		(const uint8_t*)(setup),
		(void*)(EEPROM_START_ADDRESS+1), sizeof(*setup));
}

/*****************************************************************************/
static int32_t
parse_integer_in_range(
	char*	s,
	const int32_t	min_value,
	const int32_t	max_value,
	const int32_t	old_value,
	PGM_P		name,
	PGM_P		unit)
{
	char		xbuf[128];
	int32_t		new_value;
	char*	endptr = s;

	errno = 0;
	new_value = strtol(s, &endptr, 10);

	if (errno==0 && endptr!=s) {
		if (new_value >= min_value && new_value<=max_value) {
			setup_send_P(name);

			sprintf_P(xbuf, PSTR(" is now %ld"), new_value);
			setup_send(xbuf);

			setup_send_char(' ');
			setup_send_P(unit);
			setup_send_P(PSTR(".\r\n"));
			return new_value;
		} else {
			setup_send_P(name);

			sprintf_P(xbuf, PSTR(" %ld"), new_value);
			setup_send(xbuf);

			sprintf_P(xbuf, PSTR(" is out of the range %ld .. %ld \r\n"), min_value, max_value);
			setup_send(xbuf);

			return old_value;
		}
	} else {
		setup_send_P(PSTR("Invalid integer specified.\r\n"));
		return old_value;
	}
}

/*****************************************************************************/
static unsigned int	input_length = 0;
static char		input_buffer[64];

void
setup_handle_input(
	const char 	c,
	SETUP*		setup)
{
	if (c == '\r') {
		setup_send_char('\n');
		input_buffer[input_length] = 0;
		// Look what we've got.
		if (input_length>0) {
			const char cmd = input_buffer[0];
			if (cmd == '?') {
				setup_print(setup);
			} else if (cmd == '0') {
				setup->realtime_show = !setup->realtime_show;
				setup_store_to_nvram(setup);
				if (setup->realtime_show) {
					setup_send_P(PSTR("Realtime show is now ON."));
				} else {
					setup_send_P(PSTR("Realtime show is now OFF."));
				}
			} else if (input_length>2) {
				switch (cmd) {
					case '1':
						setup->pulse_length = parse_integer_in_range(
							input_buffer + 2,
							1, 999, setup->pulse_length,
							PSTR("Pulse length"), PSTR("ms"));
						setup_store_to_nvram(setup);
						break;
					case '2':
						setup->pulse_offset = parse_integer_in_range(
							input_buffer + 2,
							-1000, 1000, setup->pulse_offset,
							PSTR("Pulse offset"), PSTR("ms"));
						setup_store_to_nvram(setup);
						break;
					case '3':
						setup->offset_limit = parse_integer_in_range(
							input_buffer + 2,
							-100, 100, setup->offset_limit,
							PSTR("Offset limit"), PSTR("ms"));
						setup_store_to_nvram(setup);
						break;
					case '4':
						setup->jump_limit = parse_integer_in_range(
							input_buffer + 2,
							LONG_MIN, LONG_MAX, setup->jump_limit,
							PSTR("Jump limit"), PSTR("ms"));
						setup_store_to_nvram(setup);
						break;
					case '5':
						setup->reaction_speed = parse_integer_in_range(
							input_buffer + 2,
							1, 100, setup->reaction_speed,
							PSTR("Reaction speed"), PSTR(""));
						setup_store_to_nvram(setup);
						break;
					case '6':
						{
							const char*	s = input_buffer + 2;
							while (*s!=0 && !isalnum(*s)) {
								++s;
							}
							if (strlen(s) == 5) {
								strcpy(setup->compass_sentence, s);
								setup_store_to_nvram(setup);
								setup_send_P(PSTR("Compass sentence set to "));
								setup_send(setup->compass_sentence);
							} else {
								setup_send_P(PSTR("Invalid compass sentence specified (length must equal 5):"));
								setup_send(s);
							}
						}
						break;
				}
			}
		}
		// Print prompt.
		setup_send_P(PSTR("\r\n>"));
		input_length = 0;
	} else if (c == 0x08) {
		// it is nice to handle backspace.
		if (input_length>0) {
			--input_length;
		}
	} else if (c!='\r' && c!='\n') {
		if (input_length+1<sizeof(input_buffer)) {
			input_buffer[input_length] = c;
			++input_length;
		} else {
			input_length = 0;
		}
	}
}

