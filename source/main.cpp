#include "thermoscope.h"
#include "spiral-animation.h"

// #include "MicroBit.h"

BLE ble;

// Temporary static name
const static char     DEVICE_NAME_PREFIX[]        = "Thermoscope ";
const static char     DEVICE_NAME[]        = "Thermoscope M";

int32_t initialTemp = 1000;
int32_t initialCounts = 0;

bool startingToSleep = false;
bool sleeping = false;
uint64_t advertisingStartTime = 0;

#define ADVERTISING_TIMEOUT_MS 90000

// Static configuration of services
// TODO figure out how to split this out into a header like the other services
int16_t adcCalibrationHalfCounts = 512;
int16_t adcCalibrationThreeQuartersCounts = 767;
char iconChar[5]  = "m";
// 1.0.0 was using the Bluefruit
// 2.0.0 used 100kOhm thermistor and external pullup
char version[] = "3.0.0";


const uint8_t advertising_img_arr[] {
                          1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

MicroBitImage advertising_img(10,5,advertising_img_arr);

// TODO figure out if we need to send the NUL at the end of the string or not.
// With the Bluefruit I just sent the string in. And it updated the char
// so I don't know what length it used. Other examples of strings in the nordic ble
// do not include the NUL. So lets try that here.
GattCharacteristic
  iconCharChar((uint16_t)0x2345, (uint8_t *)&iconChar, strlen(iconChar), 5,
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ |
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE |
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE
  );
GattCharacteristic
  versionChar((uint16_t)0x6789, (uint8_t *)&version, strlen(version), 10,
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ
  );
ReadWriteGattCharacteristic<int16_t>
  adcCalibrationHalfCountsChar((uint16_t)0x0101, &adcCalibrationHalfCounts,
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE);
ReadWriteGattCharacteristic<int16_t>
  adcCalibrationThreeQuartersCountsChar((uint16_t)0x0102, &adcCalibrationThreeQuartersCounts,
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE);
GattCharacteristic *deviceChars[] = {
  &iconCharChar,
  &versionChar,
  &adcCalibrationHalfCountsChar,
  &adcCalibrationThreeQuartersCountsChar, };
GattService         deviceInfoService((uint16_t)0x1234, deviceChars, sizeof(deviceChars) / sizeof(GattCharacteristic *));

const uint8_t  TempAServiceUUID[] = {
  0xf0, 0x00, 0xaa, 0x00, 0x04, 0x51, 0x40, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

ReadOnlyGattCharacteristic<int32_t>
  tempAChar((uint16_t)0x0001, &initialTemp, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);
ReadOnlyGattCharacteristic<int32_t>
  countsAChar((uint16_t)0x0002, &initialCounts, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);


GattCharacteristic *tempAChars[] = {&tempAChar, &countsAChar, };
GattService         tempAService(TempAServiceUUID, tempAChars, sizeof(tempAChars) / sizeof(GattCharacteristic *));

const uint8_t  TempBServiceUUID[] = {
  0xf0, 0x00, 0xbb, 0x00, 0x04, 0x51, 0x40, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

ReadOnlyGattCharacteristic<int32_t>
  tempBChar((uint16_t)0x0001, &initialTemp, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);
ReadOnlyGattCharacteristic<int32_t>
  countsBChar((uint16_t)0x0002, &initialCounts, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

GattCharacteristic *tempBChars[] = {&tempBChar, &countsBChar, };
GattService         tempBService(TempBServiceUUID, tempBChars, sizeof(tempBChars) / sizeof(GattCharacteristic *));

// MicroBit uBit;

MicroBitButton buttonA(MICROBIT_PIN_BUTTON_A, MICROBIT_ID_BUTTON_A);
MicroBitButton buttonB(MICROBIT_PIN_BUTTON_B, MICROBIT_ID_BUTTON_B);
MicroBitDisplay display;
MicroBitStorage storage;
MicroBitMessageBus messageBus;
MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ALL);
MicroBitPin P1(MICROBIT_ID_IO_P1, MICROBIT_PIN_P1, PIN_CAPABILITY_ALL);
MicroBitPin P2(MICROBIT_ID_IO_P2, MICROBIT_PIN_P2, PIN_CAPABILITY_ALL);

void my_panic(){
  display.print("e");
  while(true){
      fiber_sleep(10);
  }
}

void startAdvertising()
{
  advertisingStartTime = system_timer_current_time();
  display.print(advertising_img);
  BLE::Instance().gap().startAdvertising();
}

/* Restart Advertising on disconnection*/
void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *)
{
  startAdvertising();
}

void connectionCallback(const Gap::ConnectionCallbackParams_t *)
{
  create_fiber(displayAnimation);
}

void updateStorage()
{
  // return control so we are run later (I hope)
  fiber_sleep(100);
  if(storage.put("adcCal50", (uint8_t *)&adcCalibrationHalfCounts,
     sizeof(adcCalibrationHalfCounts)) != MICROBIT_OK){
    my_panic();
  }
  if(storage.put("adcCal75", (uint8_t *)&adcCalibrationThreeQuartersCounts,
     sizeof(adcCalibrationThreeQuartersCounts)) != MICROBIT_OK){
    my_panic();
  }
  if(storage.put("iconChar", (uint8_t *)iconChar,
     sizeof(iconChar)) != MICROBIT_OK){
    my_panic();
  }
  display.print("U");
}

void onDataWritten(const GattWriteCallbackParams* writeParams)
{
  uint16_t handle = writeParams->handle;

  if (handle == adcCalibrationHalfCountsChar.getValueHandle()) {
    memcpy(&adcCalibrationHalfCounts, writeParams->data, sizeof(adcCalibrationHalfCounts));
    display.print("H");
    create_fiber(updateStorage);
  } else if(handle == adcCalibrationThreeQuartersCountsChar.getValueHandle()) {
    memcpy(&adcCalibrationThreeQuartersCounts, writeParams->data, sizeof(adcCalibrationThreeQuartersCounts));
    display.print("T");
    create_fiber(updateStorage);
  } else if(handle == iconCharChar.getValueHandle()) {
    uint32_t len = writeParams->len;
    if(len > (sizeof(iconChar)-1)) {
      len = sizeof(iconChar) - 1;
    }
    // fill the iconChar with zeros so that regardless of the length
    // there will be at least one zero at the end
    memset(iconChar, 0, sizeof(iconChar));
    memcpy(iconChar, writeParams->data, len);
    create_fiber(updateStorage);
  }
}

// read the values fron storage and also set them on the gatt characteristics
void loadStoredValue(const char *name, int16_t& value, GattCharacteristic& gattChar)
{
  KeyValuePair* storedPair = storage.get(name);

  if (storedPair == NULL)
  {
    // nothing stored yet the default values will be used
  } else {
    memcpy(&value, storedPair->value, sizeof(value));
    ble.gattServer().write(gattChar.getValueHandle(),
      storedPair->value, sizeof(value));
    // display.scroll((int)value);
  }
}

// read the values fron storage and also set them on the gatt characteristics
void loadStoredValue(const char *name, char *value, uint16_t len, GattCharacteristic& gattChar)
{
  KeyValuePair* storedPair = storage.get(name);

  if (storedPair == NULL)
  {
    // nothing stored yet the default values will be used
  } else {
    memcpy(value, storedPair->value, len);

    // Because this is a variable length characteristic, the length needs to be updated
    // when the value is updated. Otherwise the length remains the same as when the
    // the char was initialized
    uint16_t *len = gattChar.getValueAttribute().getLengthPtr();
    *len = strlen((char *)(storedPair->value));

    // Then write the value
    ble.gattServer().write(gattChar.getValueHandle(),
      storedPair->value, strlen((char *)(storedPair->value)));
    // display.scroll(value);
  }
}

// must be called after ble is mostly initialized
void initStoredConfig()
{
  loadStoredValue("adcCal50", adcCalibrationHalfCounts,
    adcCalibrationHalfCountsChar);

  loadStoredValue("adcCal75", adcCalibrationThreeQuartersCounts,
    adcCalibrationThreeQuartersCountsChar);

  loadStoredValue("iconChar", iconChar, sizeof(iconChar), iconCharChar);

}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    // I wonder why this is commented out
    BLE &ble          = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        return;
    }

    initStoredConfig();

    // I'm not sure what this check is for. Is is used in the ble examples from
    // nordic
    if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    // register a callback so we start advertising again on disconnect
    ble.gap().onDisconnection(disconnectionCallback);

    // register connection callback so we can update the display
    ble.gap().onConnection(connectionCallback);

    /**
     * In our advertising payload we just want to include the name since that is all
     * the softwrew knows about. If it fits it would make sense to include a UUID
     * too for future usage. Using a UUID will speed up the identification by the
     * browser
     */

     /* setup advertising this comes from the health thermometer example */
     // add the flags
     ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);

     // figure out the device name
     char deviceName[sizeof(DEVICE_NAME_PREFIX) + 4];

     // Need to figure out how to initialize this.
     strcpy(deviceName, DEVICE_NAME_PREFIX);

     //  = DEVICE_NAME_PREFIX;
     strncat(deviceName, iconChar, 4);

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

     // The size should include the NUL at the end or not
     ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)deviceName, strlen(deviceName) + 1);
     // If I understand things correctly, this means that any other device can connect to
     // to us. This is also the place where it would be changed if we wanted to support
     // a scan response.
     ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
     // TODO: check the apple guidelines, I believe they recommend a different advertising
     // interval configuration: https://developer.apple.com/hardwaredrivers/BluetoothDesignGuidelines.pdf
     // the lowest recommended adv interval from apple, is 152ms. However they say it is
     // fine to advertise faster for the first 30 seconds when it is turned on
     ble.gap().setAdvertisingInterval(20); /* 100ms */

     // It seems the default connection interval that is used is very slow
     // if I understand it right. The Peripheral supplies its preferences and the central
     // device decides what to do. So I think the default preferences from the MicroBit
     // are very slow.  These are set by this struct: ConnectionParams_t
     // and these methods can be used to set them: getPreferredConnectionParams
     // setPreferredConnectionParams
     // From the micro bit dal source
     // Configure for high speed mode where possible.
     Gap::ConnectionParams_t fast;
     ble.gap().getPreferredConnectionParams(&fast);
     fast.minConnectionInterval = 8;  // 10 ms
     fast.maxConnectionInterval = 16; // 20 ms
     fast.slaveLatency = 0;
     ble.gap().setPreferredConnectionParams(&fast);

     ble.gattServer().onDataWritten(&onDataWritten);


     // Add the 2 temperature sensor services
     ble.addService(deviceInfoService);
     ble.addService(tempAService);
     ble.addService(tempBService);

     startAdvertising();
}


void my_wait(uint32_t millis)
{
    // uBit.sleep(millis);
    // wait_ms(millis);
    fiber_sleep(millis);
}

void startSleep(){
  startingToSleep = true;

  // P2 is used to power the voltage divider
  // power it down as we prepare for sleep
  P2.setDigitalValue(0);

  display.clear();
}

// sleeping is divided into two parts this is the second part
// TODO test out if this finishSleep needs to be done in the main loop
// it would be cleaner if this could be done using a delayed fiber method
void finishSleep(){
  sleeping = true;

  // configure button A to wake from power off.
  nrf_gpio_cfg_sense_input(MICROBIT_PIN_BUTTON_A, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

  #if READ_TEMP == readTemperaturePullUp
    // need to clear the pull up resistor on the pins so we don't loose current there
    P0.getDigitalValue();
    P0.setPull(PullNone);
    P1.getDigitalValue();
    P1.setPull(PullNone);
  #endif

  // is it possible that the A2D converter gets left on here?

  sd_power_system_off();
}

void onButtonA(MicroBitEvent e)
{

  if (e.value == MICROBIT_BUTTON_EVT_DOWN) {
    // startingToSleep = true;
    //
    // // P2 is used to power the voltage divider
    // // power it down as we prepare for sleep
    // P2.setDigitalValue(0);
    //
    // display.clear();
  }

  if (e.value == MICROBIT_BUTTON_EVT_UP) {
    startSleep();

    // // Try to configure button A to wake from power off.
    // nrf_gpio_cfg_sense_input(MICROBIT_PIN_BUTTON_A, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
    //
    // // need to clear the pull up resistor on the pins so we don't loose current there
    // // P0.getDigitalValue();
    // // P0.setPull(PullNone);
    // // P1.getDigitalValue();
    // // P1.setPull(PullNone);
    //
    // // is it possible that the A2D converter gets left on here?
    //
    // sd_power_system_off();
  }
}

// how many samples to take and average, more takes longer
// but is more 'smooth'
// bluefruit thermoscope used 80 here the micro bit with the pullup approach was
// very smooth with just 10, so I might be able to go lower
#define NUMSAMPLES 1
int samples[NUMSAMPLES];

// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25

void _readTemperature(MicroBitPin& pin, GattAttribute::Handle_t gattTempHandle,
    GattAttribute::Handle_t gattCountsHandle,
    float seriesResistance, float thermistorNominalResistance, float thermistorBeta){
  uint8_t i;
  float averageDigitialCounts;

  // take N samples in a row
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = pin.getAnalogValue();
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

  // ratio = Vin/Vout
  // Need to calibrate this ratio using the stored calibration points
  float ratioSlope = 0.25 / (adcCalibrationThreeQuartersCounts - adcCalibrationHalfCounts);
  float ratioOffset = 0.5 - ratioSlope * adcCalibrationHalfCounts;
  float ratio = averageDigitialCounts * ratioSlope + ratioOffset;

  // convert the ratio to resistance
  // Solving the voltage divider equation:
  //   Vout = Vin * R2 / (R1 + R2)
  // For R2 yields:
  //   R2 = Vout * R1/ (Vin - Vout)
  // Then writing that in terms of a the ratio
  //   ratio = Vin/Vout
  //   R2 = ratio*R1/(1 - ratio)
  float resistance = seriesResistance * ratio / (1 - ratio);

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
  int32_t averageDigitalCounts_100 = (int32_t)(averageDigitialCounts * 100);

  if (ble.getGapState().connected) {
      ble.gattServer().write(gattTempHandle, (uint8_t *) &temperature_100, sizeof(temperature_100));
      ble.gattServer().write(gattCountsHandle, (uint8_t *) &averageDigitalCounts_100, sizeof(averageDigitalCounts_100));
  }

  // convert to an int by multipling by 10 this gives us 1 decimal place of resolution
  // display.scroll((int)(steinhart * 10));

  // reset the display after scrolling, so we can tell it is still on
  // display.print("A");
  // gatt.setChar(sensorService->measureCharId, (int32_t)(steinhart * 100));

}

void readTemperaturePullUp(MicroBitPin& pin, GattAttribute::Handle_t gattTempHandle,
    GattAttribute::Handle_t gattCountsHandle){
  // If we are using the pull up resistor
  float seriesResistance = 13000;
  float thermistorNominalResistance = 10000;
  float thermistorBeta = 3950;
  _readTemperature(pin, gattTempHandle, gattCountsHandle, seriesResistance, thermistorNominalResistance, thermistorBeta);
}

void readTemperature100k(MicroBitPin& pin, GattAttribute::Handle_t gattTempHandle,
    GattAttribute::Handle_t gattCountsHandle){
  // There is a 10MOhm pull up resistor on P0, P1, and P2
  float seriesResistance = 99010;
  float thermistorNominalResistance = 100000;
  // The reported Beta from the datasheet was 4261, but adjusting that for the
  // range of temperatures (0 - 25) that these will be used for 4090 is
  // a better value.  That was computed via
  // http://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm
  float thermistorBeta = 4090;

  _readTemperature(pin, gattTempHandle, gattCountsHandle, seriesResistance, thermistorNominalResistance, thermistorBeta);
}

#define READ_TEMP readTemperaturePullUp

int main(void)
{
    // reset this incase globals are preserved during deepSleep
    startingToSleep = false;

    // Indicate that we are initializing
    display.print("I");

    ble.init(bleInitComplete);

    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (!ble.hasInitialized()) { /* spin loop */ }

    // uBit.display.print("b");

    // Bring up fiber scheduler.
    scheduler_init(messageBus);


    #if READ_TEMP == readTemperaturePullUp
      // Note: The pull up resitor can vary in resistance a lot from the spec
      //  Symbol  |   Parameter              |   Min.  |    Typ.  |    Max.  |   Units
      //  RPU     |   Pull-up resistance     |   11    |    13    |    16    |   kOhm
      //  RPD     |   Pull-down resistance   |   11    |    13    |    16    |   kOhm

      // enable the pull up on on pin0
      // Need to switch into digital mode before setting the pull up
      // geting a digital value seems like the only way to do this

      // Also this will result in more power usage if the pull up resistor is left enable
      // the whole time even when not collecting. A better approach would be to disable
      // the pull up until we are connector. Or perhaps only enabling it during the actual
      // collection.
      P0.getDigitalValue();
      if(P0.setPull(PullUp) != MICROBIT_OK){
        my_panic();
      };
      P1.getDigitalValue();
      if(P1.setPull(PullUp) != MICROBIT_OK){
        my_panic();
      };
    #else

      // just to be sure
      P0.getDigitalValue();
      P0.setPull(PullNone);
      P1.getDigitalValue();
      P1.setPull(PullNone);

      // P2 is used to power the voltage divider
      P2.setDigitalValue(1);

    #endif

    // uBit.display.print("m");

    if(messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_DOWN, onButtonA) != MICROBIT_OK){
      my_panic();
    }

    if(messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_UP, onButtonA) != MICROBIT_OK){
      my_panic();
    }

    // this line causes a gain of 0.8mA looking at the code this seems like it must be
    // caused by making the pins be input pins and/or setting them to PullNone
    // display.disable();

    // sd_power_system_off();

    while(true){
      if (startingToSleep) {
        startingToSleep = false;
        my_wait(500);
        finishSleep();
      } else if (sleeping) {
        my_wait(500);
      } else if (ble.getGapState().connected){
          READ_TEMP(P0, tempAChar.getValueHandle(), countsAChar.getValueHandle());
          my_wait(500);
          // the connected state might have changed after this wait
          if (ble.getGapState().connected){
            READ_TEMP(P1, tempBChar.getValueHandle(), countsBChar.getValueHandle());
            my_wait(500);
          }
      } else if (ble.getGapState().advertising){
        uint64_t advertisingTime = system_timer_current_time() - advertisingStartTime;
        if(advertisingTime > ADVERTISING_TIMEOUT_MS){
          startSleep();
        } else {
          uint64_t remainingAdvertisingTime = ADVERTISING_TIMEOUT_MS - advertisingTime;
          int pixels_to_show = ((remainingAdvertisingTime * 5)/ ADVERTISING_TIMEOUT_MS) + 1;
          // display.print(pixels_to_show);
          display.print(advertising_img, pixels_to_show - 5, 0);
          // display.print((int)((ADVERTISING_TIMEOUT_MS - advertisingTime) / 10000));
          my_wait(100);
        }
      } else {
        my_wait(100);
      }
    }
    // while (true) {
    //     ble.waitForEvent(); // allows or low power operation
    // }
}
