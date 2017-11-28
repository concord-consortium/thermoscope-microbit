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

// Temporary static name
const static char     DEVICE_NAME[]        = "Thermoscope M";

// Static configuration of services
// TODO figure out how to split this out into a header like the other services
const uint8_t  TempAServiceUUID[] = {
  0xf0, 0x00, 0xaa, 0x00, 0x04, 0x51, 0x40, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

int32_t initialTemp = 1000;

ReadOnlyGattCharacteristic<int32_t>
  tempAChar((uint16_t)0x0001, &initialTemp, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

GattCharacteristic *tempAChars[] = {&tempAChar, };
GattService         tempAService(TempAServiceUUID, tempAChars, sizeof(tempAChars) / sizeof(GattCharacteristic *));

const uint8_t  TempBServiceUUID[] = {
  0xf0, 0x00, 0xbb, 0x00, 0x04, 0x51, 0x40, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

ReadOnlyGattCharacteristic<int32_t>
  tempBChar((uint16_t)0x0001, &initialTemp, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

GattCharacteristic *tempBChars[] = {&tempBChar, };
GattService         tempBService(TempBServiceUUID, tempBChars, sizeof(tempBChars) / sizeof(GattCharacteristic *));

// MicroBit uBit;

MicroBitButton buttonA(MICROBIT_PIN_BUTTON_A, MICROBIT_ID_BUTTON_A);
MicroBitButton buttonB(MICROBIT_PIN_BUTTON_B, MICROBIT_ID_BUTTON_B);
MicroBitDisplay display;
MicroBitMessageBus messageBus;
MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ALL);

/* Restart Advertising on disconnection*/
void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *)
{
    BLE::Instance().gap().startAdvertising();
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    // I wonder why this is commented out
    BLE &ble          = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        return;
    }

    // I'm not sure what this check is for. Is is used in the ble examples from
    // nordic
    if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    // register a callback so we start advertising again on disconnect
    ble.gap().onDisconnection(disconnectionCallback);

    /**
     * In our advertising payload we just want to include the name since that is all
     * the softwrew knows about. If it fits it would make sense to include a UUID
     * too for future usage. Using a UUID will speed up the identification by the
     * browser
     */

     /* setup advertising this comes from the health thermometer example */
     // add the flags
     ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
     // TODO add a UUID this is the standard way to filter devices. This would be safer
     // than filtering by just the Thermoscope name prefix
     // but we don't have a lot of space, this would take up
     // up 16 bytes just for the id, and our name is 'Thermoscope [4byte char]' or 16 chars
     // so that is 32 bytes just for the data, and each of those need 2 more bytes of type and size
     // so that is too many bytes to include both. Either we need to decrease the name, or
     // keep the name in the scan response.
     // ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
     // I don't know what kind of additional packet this is, perhaps a known id for this type of device
     //  ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::THERMOMETER_EAR);
     ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
     // If I understand things correctly, this means that any other device can connect to
     // to us. This is also the place where it would be changed if we wanted to support
     // a scan response.
     ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
     // TODO: check the apple guidelines, I believe they recommend a different advertising
     // interval configuration
     ble.gap().setAdvertisingInterval(500); /* 500ms */

     // Add the 2 temperature sensor services
     ble.addService(tempAService);
     ble.addService(tempBService);

     ble.gap().startAdvertising();
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

    // need to clear the pull up resistor on the pins so we don't loose current there
    P0.getDigitalValue();
    P0.setPull(PullNone);

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


  // if we are using the resitor from adafruit
  // float seriesResistance = 10000;

  // If we are using the pull up resistor
  float seriesResistance = 13000;
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

  int32_t temperature_100 = (int32_t)(steinhart * 100);

  if (ble.getGapState().connected) {
      ble.gattServer().write(tempAChar.getValueHandle(), (uint8_t *) &temperature_100, sizeof(temperature_100));
  }

  // convert to an int by multipling by 10 this gives us 1 decimal place of resolution
  // display.scroll((int)(steinhart * 10));

  // reset the display after scrolling, so we can tell it is still on
  // display.print("r");
  // gatt.setChar(sensorService->measureCharId, (int32_t)(steinhart * 100));

}

void onButtonB(MicroBitEvent e)
{
  readTemperature();
}

void my_panic(){
  display.print("e");
  while(true){
      wait_ms(10);
  }
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


    // Note: The pull up resitor can vary in resistance a lot from the spec
    //  Symbol  |   Parameter              |   Min.  |    Typ.  |    Max.  |   Units
    //  RPU     |   Pull-up resistance     |   11    |    13    |    16    |   kOhm
    //  RPD     |   Pull-down resistance   |   11    |    13    |    16    |   kOhm

    // enable the pull up on on pin0
    // Need to switch into digital mode before setting the pull up
    // geting a digital value seems like the only way to do this
    P0.getDigitalValue();
    if(P0.setPull(PullUp) != MICROBIT_OK){
      my_panic();
    };

    // uBit.display.print("m");

    if(messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_DOWN, onButtonA) != MICROBIT_OK){
      my_panic();
    }

    if(messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_UP, onButtonA) != MICROBIT_OK){
      my_panic();
    }

    if(messageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_DOWN, onButtonB) != MICROBIT_OK){
      my_panic();
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
