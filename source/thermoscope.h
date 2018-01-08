#ifndef THERMOSCOPE_H
#define THERMOSCOPE_H

#include "mbed.h"
#include "ble/services/iBeacon.h"
/*
 * The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ
 * If we're compiling under GCC, then we suppress any warnings generated from this code (but not the rest of the DAL)
 * The ARM cc compiler is more tolerant. We don't test __GNUC__ here to detect GCC as ARMCC also typically sets this
 * as a compatability option, but does not support the options used...
 */
#if !defined(__arm)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "nrf_soc.h"
#include "nrf_gpio.h"

/*
 * Return to our predefined compiler settings.
 */
#if !defined(__arm)
#pragma GCC diagnostic pop
#endif

#include "MicroBitDisplay.h"
#include "MicroBitFiber.h"
#include "MicroBitMessageBus.h"
#include "MicroBitButton.h"
#include "MicroBitPin.h"
#include "MicroBitStorage.h"
#include "MicroBitSystemTimer.h"

extern BLE ble;

extern MicroBitDisplay display;

#endif
