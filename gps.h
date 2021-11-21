// vim: ts=2 shiftwidth=2
#ifndef gps_h_
#define gps_h_


#include <stdint.h>	// int32_t, etc.
#include <stdbool.h>	// true, false.


typedef enum {
	SENTENCE_NONE = 0,
	SENTENCE_GGA = 1,
	SENTENCE_VTG = 2,
	SENTENCE_ZDA = 3,
} SENTENCE;

/** Handle gps input. */
extern SENTENCE
handle_gps_input(	const uint8_t		c,
			int32_t*		gps_time,
			uint16_t*		course_x100);


#endif /* gps_h_ */

