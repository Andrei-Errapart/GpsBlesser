#ifndef main_h_
#define main_h_

#include <stdint.h>	// int32_t
#include <stdbool.h>	// bool

#define	HEADINGS_PER_SECOND	25

extern int32_t
getticksoftheday();

extern void
addticksoftheday(	const int32_t		extra_ticks);

extern void setticksoftheday(	const int32_t		ticks);

extern bool
is_ticksoftheday_valid();

/** Precision timer for syncing. */
#define	PRECISION_TICKS_PER_SECOND				(1000)
#define	PRECISION_TICKS_PER_DAY					(24L*3600L*PRECISION_TICKS_PER_SECOND)
/** SmartFlasher SYNC pulse, 140 ms. */
#define	PRECISION_TICKS_PER_SYNC				504
/** SmartFlasher startup-delay, 50 ms. */
#define	PRECISION_TICKS_EXTRA_SYNC				180
/** Sync period, 15 minutes. */
#define	PRECISION_TICKS_PERIOD_SYNC				(900L * PRECISION_TICKS_PER_SECOND)
/** Number of ticks before syncro impulse to turn GPS on. */
#define	PRECISION_TICKS_GPS_LEAD					(240L * PRECISION_TICKS_PER_SECOND)

#define	PRECISION_TICKS_PER_HEADING				(PRECISION_TICKS_PER_SECOND / HEADINGS_PER_SECOND)

#endif /* main_h_ */

