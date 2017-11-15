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

// #include "MicroBit.h"

BLE ble;

// MicroBit uBit;

MicroBitButton buttonA(MICROBIT_PIN_BUTTON_A, MICROBIT_ID_BUTTON_A);
MicroBitButton buttonB(MICROBIT_PIN_BUTTON_B, MICROBIT_ID_BUTTON_B);
MicroBitDisplay display;
MicroBitMessageBus messageBus;
MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ALL);

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    // BLE &ble          = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        return;
    }

    /**
     * The Beacon payload has the following composition:
     * 128-Bit / 16byte UUID = E2 0A 39 F4 73 F5 4B C4 A1 2F 17 D1 AD 07 A9 61
     * Major/Minor  = 0x1122 / 0x3344
     * Tx Power     = 0xC8 = 200, 2's compliment is 256-200 = (-56dB)
     *
     * Note: please remember to calibrate your beacons TX Power for more accurate results.
     */
    // const uint8_t uuid[] = {0xE2, 0x0A, 0x39, 0xF4, 0x73, 0xF5, 0x4B, 0xC4,
    //                         0xA1, 0x2F, 0x17, 0xD1, 0xAD, 0x07, 0xA9, 0x61};
    // uint16_t majorNumber = 1122;
    // uint16_t minorNumber = 3344;
    // uint16_t txPower     = 0xC8;
    // iBeacon *ibeacon = new iBeacon(ble, uuid, majorNumber, minorNumber, txPower);

    // ble.gap().setAdvertisingInterval(1000); /* 1000ms. */
    // ble.gap().startAdvertising();
}

void my_wait(uint32_t millis)
{
    // uBit.sleep(millis);
    // wait_ms(millis);
    fiber_sleep(millis);
}

void onButtonA(MicroBitEvent e)
{
  // uBit.display.printAsync("A");
  // wait_ms(1000);
  // my_wait(1000);
  // my_wait(1000);
  // uBit.display.clear();
  //
  // my_wait(1000);
  // wait_ms(1000);

  if (e.value == MICROBIT_BUTTON_EVT_DOWN)
    display.clear();

  if (e.value == MICROBIT_BUTTON_EVT_UP) {
    // Try to configure button A to wake from power off.
    nrf_gpio_cfg_sense_input(MICROBIT_PIN_BUTTON_A, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

    sd_power_system_off();
  }
}

// how many samples to take and average, more takes longer
// but is more 'smooth'
// bluefruit thermoscope used 80 here
#define NUMSAMPLES 10
int samples[NUMSAMPLES];

// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25

void readTemperature(){
  uint8_t i;
  float averageDigitialCounts;

  // take N samples in a row
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = P0.getAnalogValue();
  }

  // average all the samples out
  averageDigitialCounts = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     averageDigitialCounts += samples[i];
  }
  averageDigitialCounts /= NUMSAMPLES;

  // We don't need it for the resistance equation but it will help for calibration
  // to report the microVolts
  // int32_t microVolts = (int32_t)((averageDigitialCounts / 1024) * 3000000);
  // gatt.setChar(sensorService->microVoltsCharId, microVolts);

  // Serial.print("Average analog reading ");
  // Serial.println(average);

  // convert the value to resistance
  // Solving the voltage divider equation: Vout = Vin * R2 / (R1 + R2)
  // For R2 yields: R2 = R1 / ((Vin / Vout) - 1)

  float seriesResistance = 10000;
  float thermistorNominalResistance = 10000;
  float thermistorBeta = 3950;

  float resistanceRatio = (1024 / averageDigitialCounts) - 1;
  float resistance = seriesResistance  / resistanceRatio;

  // Serial.print("Thermistor resistance ");
  // Serial.println(average);

  float steinhart;
  steinhart = resistance / thermistorNominalResistance;     // (R/Ro)
  steinhart = log(steinhart);                                          // ln(R/Ro)
  steinhart /= thermistorBeta;                           // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15);                    // + (1/To)
  steinhart = 1.0 / steinhart;                                         // Invert
  steinhart -= 273.15;                         // convert to C

  // convert to an int by multipling by 10 this gives us 1 decimal place of resolution
  display.scroll((int)(steinhart * 10));

  // gatt.setChar(sensorService->measureCharId, (int32_t)(steinhart * 100));

}

void onButtonB(MicroBitEvent e)
{
  readTemperature();
}



int main(void)
{
    ble.init(bleInitComplete);

    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (!ble.hasInitialized()) { /* spin loop */ }

    // uBit.display.print("b");

    // Bring up fiber scheduler.
    scheduler_init(messageBus);

    // uBit.display.print("m");

    if(messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_DOWN, onButtonA) != MICROBIT_OK){
      display.print("e");
      while(true){
          wait_ms(10);
      }
    }

    if(messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_UP, onButtonA) != MICROBIT_OK){
      display.print("e");
      while(true){
          wait_ms(10);
      }
    }

    if(messageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_DOWN, onButtonB) != MICROBIT_OK){
      display.print("e");
      while(true){
          wait_ms(10);
      }
    }

    display.print("r");

    // this line causes a gain of 0.8mA looking at the code this seems like it must be
    // caused by making the pins be input pins and/or setting them to PullNone
    // display.disable();

    // sd_power_system_off();

    while(true){
      // spin loop incase we are running in debug mode
      my_wait(10000);
    }
    // while (true) {
    //     ble.waitForEvent(); // allows or low power operation
    // }
}
