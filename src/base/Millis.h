#ifndef __BASE_MILLIS_H__
#define __BASE_MILLIS_H__

#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "Common.h"
#include "Timer0.h"

#define MILLIS_PER_INTERRUPT (64*256) / (F_CPU/1000) // Assuming /64 prescaling
#define REST_PER_INTERRUPT	 ((((64*256) / (F_CPU/1000000)) % 1000) >> 3) 

/* volatile for the new value beign recognized after change by the interrupt */
static volatile uint32_t milliseconds;
static UNUSED uint8_t rest;

ISR(TIMER0_OVF_vect) {
	uint32_t m = milliseconds;

	m += MILLIS_PER_INTERRUPT;
	rest += REST_PER_INTERRUPT;

	if (rest >= (1000 >> 3)) {
		rest -= (1000 >> 3);
		m++;
	}

	milliseconds = m;
}

namespace Millis {
	/*
	 * init() before reading millis()
	 * It uses timer0 prescaled to /64
	 */
	static UNUSED void init() {
		milliseconds = 0;
		Timer0::clockSelect(Timer0::CLK_BY_64);
		Timer0::enableInterrupt(Timer0::OVERFLOW);
	}

	static UNUSED uint32_t millis() {
		uint32_t m;
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			m = milliseconds;
		}
		return m;
	}
};

#endif
