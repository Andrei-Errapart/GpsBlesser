// vim: ts=4 shiftwidth=4
#include <stdint.h>	// uint8_t, etc.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>	// strlen
#include <math.h>	// sin,cos.
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usart.h"
#include "main.h"
#include "gps.h"
#include "setup.h"	// setup channel.

#define	HEADING_FIX	0

#define TMR0_PRESC	256ul
#define TMR0_RELOAD	(0ul - (F_CPU / (PRECISION_TICKS_PER_SECOND * TMR0_PRESC)))

#define	PORTC_PULSE_MASK	0x0F


/** Set up timer 1 (16-bit), frequency = F_CPU/(timertop+1). */
#define setup_timer1(timertop) do {					\
	OCR1A  = (timertop);	/* set top */				\
	/*  Mode 4 - CTC with Prescaler 1 */				\
	TCCR1B = (1<<WGM12)|(1<<CS10);					\
	TCNT1 = 0; /* reset counter */					\
	TIMSK1 |= (1<<OCIE1A); /* enable output-compare int */		\
} while (0)

/** Ticks of the day. */
static volatile int32_t	ticksoftheday = 0;
/** Is it valid? */
static volatile bool	ticksoftheday_valid = false;
/** Is it time to send the heading? */
static volatile bool	should_send_heading = false;

static SETUP		setup = { /*pulse_length=*/100, /*pulse_offset=*/0 };

/*****************************************************/
// Cannot use ISR_NOBLOCK, because it takes too much time.
ISR (TIMER1_COMPA_vect)
{	
	static uint16_t	heading_ticks = 0;
	static int32_t	lticks;
	
	PORTC |= 0x80;

	// 1. Increment ticks of the day.
	if (++ticksoftheday >= PRECISION_TICKS_PER_DAY) {
		ticksoftheday = 0;
	}
	lticks = ticksoftheday;

	// 2. Blink leds, if possible.
	if (ticksoftheday_valid) {
		const int16_t	smalltick = (lticks + PRECISION_TICKS_PER_DAY + setup.pulse_offset) % PRECISION_TICKS_PER_SECOND;
		if (smalltick < setup.pulse_length) {
			// ON
			PORTC = (PORTC & ~PORTC_PULSE_MASK) | 0x03;
		} else {
			// OFF
			PORTC = (PORTC & ~PORTC_PULSE_MASK) | 0x0C;
		}
	}

	// 3. Signal heading, if possible.
	if (++heading_ticks > PRECISION_TICKS_PER_HEADING) {
		heading_ticks = 0;
		should_send_heading = true;
	}

	PORTC &= ~0x80;
}


/*****************************************************/
void io_Init(void)
{
	PORTA = 0;
	DDRA = 0;

	PORTB = 0;
	DDRB = 0;

	PORTC = 0;
	DDRC = 0xff;

	PORTD = 0;
	DDRD = 0;

	PORTE = 0;
	DDRE = 0;

	PORTF = 0;
	DDRF = 0;

	PORTG = 0;
	DDRG = 0;

	PORTH = 0;
	DDRH = 0;

	PORTJ = 0;
	DDRJ = 0;

	PORTK = 0;
	DDRK = 0;

	PORTL = 0;
	DDRL = 0;

	setup_timer1(F_CPU / 1000);
}

/*****************************************************************************/
int32_t
getticksoftheday()
{
	const bool	interrupts_enabled = (SREG & 0x80) == 0;
	int32_t	r;
	if (interrupts_enabled) {
		cli();
		r = ticksoftheday;
		sei();
		return r;
	}
	return ticksoftheday;
}

/*****************************************************************************/
void
addticksoftheday(	const int32_t		extra_ticks)
{
	// Are we allowed to add?
	if (ticksoftheday_valid && extra_ticks!=0) {
		const bool	interrupts_enabled = (SREG & 0x80) == 0;
		if (interrupts_enabled) {
			cli();
		}

		ticksoftheday = (ticksoftheday + extra_ticks + PRECISION_TICKS_PER_DAY) % PRECISION_TICKS_PER_DAY;
		ticksoftheday_valid = ticksoftheday >= 0;

		if (interrupts_enabled) {
			sei();
		}
	}
}

/*****************************************************************************/
void setticksoftheday(	const int32_t		ticks)
{
	const bool	interrupts_enabled = (SREG & 0x80) == 0;

	if (interrupts_enabled) {
		cli();
	}

	ticksoftheday = (ticks + PRECISION_TICKS_PER_DAY) % PRECISION_TICKS_PER_DAY;
	ticksoftheday_valid = ticksoftheday >= 0;

	if (interrupts_enabled) {
		sei();
	}
}

/*****************************************************************************/
bool
is_ticksoftheday_valid()
{
	return ticksoftheday_valid;
}

/*****************************************************************************/
/*****************************************************************************/
int
main(void)
{
	char		xbuf[64];

	char		course_buffer[64] = { 0 };
#if (HEADING_FIX)
	// course is buffered for output on UART2.
	uint8_t		course_buffer_length = 0;
	uint8_t		course_to_do = 0;
	uint8_t		course_so_far = 0;
	uint8_t		course_sparse = 0;
#endif

	uint8_t 	ch;
	uint16_t	vtg_course_x100 = 0;
	int32_t		gps_ticks;
	int32_t		gps_start_ticks = 0;
	int32_t		last_offset = 0;

	int16_t		cos_x14 = 0;	// x16384
	int16_t		sin_x14 = 0;	// x16384

	io_Init();
	uart_Init();
	
	sei();

	setup_send_P(PSTR("\r\nWelcome to GPS BLESSER v1.0!\r\n"));
	setup_load_from_nvram(&setup);
	setup_send_P(PSTR("\r\n>"));

	for (;;) {
		// UART0: Data From GPS
		if (!uart0_IsRxEmpty())
		{
			ch = uart0_GetChar();

			// and handle it!
			if (ch == '$') {
				gps_start_ticks = getticksoftheday();
				PORTC = PORTC ^ 0x10;
			}

			// echo back :)
			uart0_PutChar(ch);
			uart1_PutChar(ch);

			switch (handle_gps_input(ch, &gps_ticks, &vtg_course_x100)) {
				case SENTENCE_GGA:	/* passthrough. */
				case SENTENCE_ZDA:
					// signal!
					PORTC = PORTC ^ 0x40;

					if (is_ticksoftheday_valid()) {
						int32_t	new_offset = (gps_ticks - gps_start_ticks);
						int32_t	ofs = (last_offset + new_offset) / 2;
						if (ofs < setup.jump_limit && -ofs<setup.jump_limit) {
							if (ofs > setup.offset_limit) {
								ofs = setup.offset_limit;
							} else if (-setup.offset_limit > ofs) {
								ofs = -setup.offset_limit;
							}
						}
						last_offset = new_offset;
						if (ofs != 0) {
							addticksoftheday(ofs);
						}
						if (setup.realtime_show) {
							sprintf_P(xbuf, PSTR("ofs=%ld\r\n"), ofs);
							setup_send(xbuf);
						}
					} else {
						int32_t	ofs = getticksoftheday() - gps_start_ticks;
						setup_send_P(PSTR("\r\nFirst tick!\r\n"));
						setticksoftheday(gps_ticks + ofs);
					}
					break;
				case SENTENCE_VTG:
					{
					// Course update!
					const float	vtg_course = (0.01 * 3.14159265358979323844 / 180.0) * vtg_course_x100;
					const int16_t	icos = 16384 * cos(vtg_course);
					const int16_t	isin = 16384 * sin(vtg_course);
					const int32_t	f2 = 100 - setup.reaction_speed;
					

					if (setup.realtime_show) {
						// sprintf_P(xbuf, PSTR("course=%u\r\n"), vtg_course_x100);
						setup_send(xbuf);
					}

					// 1. Update sliding buffer.
					cos_x14 = (((int32_t)setup.reaction_speed)*icos + f2*cos_x14)/100;
					sin_x14 = (((int32_t)setup.reaction_speed)*isin + f2*sin_x14)/100;

					// 2. Calculate course2_x100.
					int16_t c0 = (100.0 * 180.0 / 3.14159265358979323844) * atan2(sin_x14, cos_x14);
					uint16_t	course2_x100 = c0>=0 ? c0 : (c0 + 36000u);

#if (0)
					sprintf_P(xbuf, PSTR("course=%u course2=%u diff=%d cos_x14=%d sin_x14=%d\r\n"),
						vtg_course_x100,
						course2_x100, course2_x100 - vtg_course_x100,
						cos_x14, sin_x14);
					setup_send(xbuf);
#endif
					// 3. Update course_buffer.
					sprintf_P(course_buffer, PSTR("$%s,%03u,T*"), setup.compass_sentence, course2_x100/100);
					uint8_t		checksum = 0;
					uint8_t*	ptr = (uint8_t*)course_buffer + 1;
					for (; *ptr!='*'; ++ptr) {
						checksum = checksum ^ *ptr;
					}
					++ptr;
					sprintf_P((char*)ptr, PSTR("%02X\r\n"), (unsigned int)checksum);
#if (HEADING_FIX)
					course_buffer_length = ((char*)ptr - course_buffer) + 4;
#endif
					}
					break;
				default:
					// pass
					break;
			}

			// Add extra fresh course buffer, if necessary.
			if (should_send_heading && course_buffer[0]!=0 && ch==0x0A) {
				const char*	ptr = course_buffer;
				should_send_heading = false;
#if (HEADING_FIX)
				if (course_so_far >= course_to_do) {
					course_to_do  = course_buffer_length;
					course_so_far = 0;
				}
#endif
				for (; *ptr!=0; ++ptr) {
					uart1_PutChar(*ptr);
#if (!HEADING_FIX)
					uart2_PutChar(*ptr);
#endif
				}
			} else {
#if (HEADING_FIX)
				++course_sparse;
				if ((course_sparse & 0x03)==0 && course_so_far<course_to_do) {
					uart2_PutChar(course_buffer[course_so_far]);
					++course_so_far;
				}
#endif
			}
		}    

		// UART1: Output 1: GPS + HDG, 38400.
		if (!uart1_IsRxEmpty())
		{
			ch = uart1_GetChar();
		}


		// UART2: Setup channel.
		// UART1: Output 1: Compass, 9600, 25Hz., Sentence: HDG.
		if (!uart2_IsRxEmpty())
		{
			ch = uart2_GetChar();
			uart2_PutChar(ch);
			setup_handle_input(ch, &setup);
		}


		// Empty channel - not working?
		if (!uart3_IsRxEmpty())
		{
			ch = uart3_GetChar();
			uart3_PutChar(ch);
		}
	}
}

