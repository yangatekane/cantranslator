#ifdef CAN_EMULATOR

#include "usbutil.h"
#include "canread.h"
#include "serialutil.h"
#include "log.h"
#include <stdlib.h>

#define NUMERICAL_SIGNAL_COUNT 11
#define BOOLEAN_SIGNAL_COUNT 5
#define STATE_SIGNAL_COUNT 2
#define EVENT_SIGNAL_COUNT 1

extern SerialDevice SERIAL_DEVICE;
extern UsbDevice USB_DEVICE;
extern Listener listener;

const char* NUMERICAL_SIGNALS[NUMERICAL_SIGNAL_COUNT] = {
    "steering_wheel_angle",
    "torque_at_transmission",
    "engine_speed",
    "vehicle_speed",
    "accelerator_pedal_position",
    "odometer",
    "fine_odometer_since_restart",
    "latitude",
    "longitude",
    "fuel_level",
    "fuel_consumed_since_restart",
};

const char* BOOLEAN_SIGNALS[BOOLEAN_SIGNAL_COUNT] = {
    "parking_brake_status",
    "brake_pedal_status",
    "headlamp_status",
    "high_beam_status",
    "windshield_wiper_status",
};

const char* STATE_SIGNALS[STATE_SIGNAL_COUNT] = {
    "transmission_gear_position",
    "ignition_status",
};

const char* SIGNAL_STATES[STATE_SIGNAL_COUNT][3] = {
    { "neutral", "first", "second" },
    { "off", "run", "accessory" },
};

const char* EVENT_SIGNALS[EVENT_SIGNAL_COUNT] = {
    "door_status",
};

struct Event {
    const char* value;
    bool event;
};

Event EVENT_SIGNAL_STATES[EVENT_SIGNAL_COUNT][3] = {
    { {"driver", false}, {"passenger", true}, {"rear_right", true}},
};

void carStop() {
  for (int i=0; i < NUMERICAL_SIGNAL_COUNT; i++) {
    writeNumericalMeasurement(NUMERICAL_SIGNALS[i], 0);
  }

  for (int j=0; j < BOOLEAN_SIGNAL_COUNT; j++) {
    if (j > 1) { // there should be a better way to do this
      writeBooleanMeasurement(BOOLEAN_SIGNALS[j], false);
    }
    else {
      writeBooleanMeasurement(BOOLEAN_SIGNALS[j], true);
    }
  }

  writeStateMeasurement(STATE_SIGNALS[0],
             SIGNAL_STATES[0][0]);

  writeStateMeasurement(STATE_SIGNALS[1],
             SIGNAL_STATES[1][0]);

  /* events? */
}

void carStart() {
  writeStateMeasurement(STATE_SIGNALS[1],
             SIGNAL_STATES[1][1]);
}

void setup() {
    srand(42);

    initializeLogging();
    initializeSerial(&SERIAL_DEVICE);
    initializeUsb(&USB_DEVICE);
}

bool usbWriteStub(uint8_t* buffer) {
    debug("Ignoring write request -- running an emulator\r\n");
    return true;
}

void loop() {
    float lastDist = 0;
    float lastGas = 0;
    float lastSpeed = 0;
    float temps = 0;
    float delayFreq = 100;

    carStart();
    float startingTime = millis();
    while(1) {
        if (millis() > startingTime + 30000) {
            carStop();
            carStart();
            break;
        }

        bool positive;
        if (lastSpeed > 120) {
            random(3) == 0 ? positive = true : positive = false;
        }
        else if (lastSpeed < 20) {
            random(3) == 0 ? positive = false : positive = true;
        }
        else if (lastSpeed == 0) {
            positive = true;
        }
        else {
            random(2) == 0 ? positive = false : positive = true;
        }

        if(positive) {
            lastSpeed = lastSpeed + random(2);
        }
        else {
            lastSpeed = lastSpeed - random(2);
        }

        float temp = lastSpeed * ((delayFreq/1000)/3600);
        lastDist = lastDist + temp;
        sendNumericalMessage(NUMERICAL_SIGNALS[3], lastSpeed, &listener); // FIXME, these should not be hardcoded
        sendNumericalMesasge(NUMERICAL_SIGNALS[6], lastDist, &listener);

        temp = random(3) * (0.001 * (delayFreq/1000)); // This is probably wrong
        lastGas = lastGas + temp;
        sendNumericalMessage(NUMERICAL_SIGNALS[10], lastGas, &listener);

        sendNumericalMessage(
                NUMERICAL_SIGNALS[rand() % NUMERICAL_SIGNAL_COUNT],
                rand() % 50 + rand() % 100 * .1, &listener);
        sendBooleanMessage(BOOLEAN_SIGNALS[rand() % BOOLEAN_SIGNAL_COUNT],
                rand() % 2 == 1 ? true : false, &listener);

        int stateSignalIndex = rand() % STATE_SIGNAL_COUNT;
        sendStringMessage(STATE_SIGNALS[stateSignalIndex],
                SIGNAL_STATES[stateSignalIndex][rand() % 3], &listener);

        int eventSignalIndex = rand() % EVENT_SIGNAL_COUNT;
        Event randomEvent = EVENT_SIGNAL_STATES[eventSignalIndex][rand() % 3];
        sendEventedBooleanMessage(EVENT_SIGNALS[eventSignalIndex],
                randomEvent.value, randomEvent.event, &listener);

        processListenerQueues(&listener);
        readFromHost(&USB_DEVICE, usbWriteStub);
        readFromSerial(&SERIAL_DEVICE, usbWriteStub);
    }
}

void reset() { }

const char* getMessageSet() {
    return "emulator";
}

#endif // CAN_EMULATOR
