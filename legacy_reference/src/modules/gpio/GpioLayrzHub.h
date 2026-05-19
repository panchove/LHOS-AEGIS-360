#pragma once

#ifndef __GPIOLAYRZHUB_H__
#define __GPIOLAYRZHUB_H__

#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <modules/arducam/ArducamLayrzHub.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

#if defined(LAYRZ_HUB1_BUILD)
extern volatile bool blockHandlerIO1;
extern volatile bool blockHandlerIO2;
extern volatile bool blockHandlerIO5;
extern volatile bool blockHandlerIO6;
extern volatile bool blockHandlerIO7;
extern volatile bool blockHandlerIO14;

#elif defined(LAYRZ_HUB2_BUILD)
extern volatile bool blockHandlerIO3;
extern volatile bool blockHandlerIO4;
extern volatile bool blockHandlerIO5;
extern volatile bool blockHandlerIO6;
extern volatile bool blockHandlerIO19;
extern volatile bool blockHandlerIO20;
extern volatile bool blockHandlerIO41;
extern volatile bool blockHandlerIO42;

#elif defined(LAYRZ_HUB25_BUILD)
extern volatile bool blockHandlerIO4;
extern volatile bool blockHandlerIO5;
extern volatile bool blockHandlerIO6;
extern volatile bool blockHandlerIO42;
extern volatile bool blockHandlerIO7;
extern volatile bool blockHandlerIO8;
extern volatile bool blockHandlerIO19;
extern volatile bool blockHandlerIO20;
#endif

class GpioLayrzHub {
public:
  static TaskHandle_t gpioEventTaskHandle;

  typedef struct {
    int pinIndex;       // 0 to 5 for IO1 to IO14
    bool triggerCamera; // true if takePicture() should be called
  } GpioEvent_t;

  static QueueHandle_t gpioEventQueue;
  // initialize GPIO pins
  static void initGPIO();
  static void gpioEventTask(void *pvParameters);
  // Set duty cycle (0-4095) for a specific channel
  static void setPWMDuty(int channel, uint32_t duty);
  // Create debounce timers for GPIOs
  static bool createDebTimers();
  // Print GPIO configurations to console in debug mode
  static void printGPIOConfigs();
  // Print GPIO event counters to console in debug mode
  static void printGPIOEventCounters();
  // Reset GPIO event counters
  static void resetGPIOEventCounters();
  // get GPIO payload
  static std::string getGPIOPayload();
  // Method to set a single digital output without duration
  static int setDO(int pin, int value);
  // Method to set multiple digital outputs with duration
  static int setMultipleDO(uint8_t doEnabled, byte doByte, uint8_t *doDuration);
  // Method to set PWM output
  static int setPwmOutput(int pin, int value);

  struct DigitalOutputConfig {
    uint8_t pin;
    TimerHandle_t timer;
  };
  static void digitalOutputTimerCallback(TimerHandle_t xTimer);

private:
  // Function to read calibrated voltage from GPIO pin
  static uint32_t readCalibratedVoltage(int pin);
  // Function to get temperature from NTC sensor
  static float getNTCTemperature(int pin, int r0, int t0, int beta, int r1,
                                 int mode);
  // Function to read average voltage from GPIO pin
  static uint32_t readAvgVoltage(int pin, int samples = 100);
  // Function to read median voltage from GPIO pin
  static uint32_t readMedianVoltage(int pin, int samples = 10);
  static uint8_t getBatteryLevel(uint32_t vbatSense_mV);

#if defined(LAYRZ_HUB1_BUILD)
  // GPIO debounce timers
  static TimerHandle_t debTimerIO1;
  static TimerHandle_t debTimerIO2;
  static TimerHandle_t debTimerIO5;
  static TimerHandle_t debTimerIO6;
  static TimerHandle_t debTimerIO7;
  static TimerHandle_t debTimerIO14;
  // GPIO debounce timer handlers
  static void timerIO1CB(TimerHandle_t xTimer);
  static void timerIO2CB(TimerHandle_t xTimer);
  static void timerIO5CB(TimerHandle_t xTimer);
  static void timerIO6CB(TimerHandle_t xTimer);
  static void timerIO7CB(TimerHandle_t xTimer);
  static void timerIO14CB(TimerHandle_t xTimer);
  // GPIO interrupt handlers
  static void IO1Handler();
  static void IO2Handler();
  static void IO5Handler();
  static void IO6Handler();
  static void IO7Handler();
  static void IO14Handler();

#elif defined(LAYRZ_HUB2_BUILD)
  // GPIO debounce timers
  static TimerHandle_t debTimerIO3;
  static TimerHandle_t debTimerIO4;
  static TimerHandle_t debTimerIO5;
  static TimerHandle_t debTimerIO6;
  static TimerHandle_t debTimerIO19;
  static TimerHandle_t debTimerIO20;
  static TimerHandle_t debTimerIO41;
  static TimerHandle_t debTimerIO42;
  // GPIO debounce timer handlers
  static void timerIO3CB(TimerHandle_t xTimer);
  static void timerIO4CB(TimerHandle_t xTimer);
  static void timerIO5CB(TimerHandle_t xTimer);
  static void timerIO6CB(TimerHandle_t xTimer);
  static void timerIO19CB(TimerHandle_t xTimer);
  static void timerIO20CB(TimerHandle_t xTimer);
  static void timerIO41CB(TimerHandle_t xTimer);
  static void timerIO42CB(TimerHandle_t xTimer);
  // GPIO interrupt handlers
  static void IO3Handler();
  static void IO4Handler();
  static void IO5Handler();
  static void IO6Handler();
  static void IO19Handler();
  static void IO20Handler();
  static void IO41Handler();
  static void IO42Handler();

#elif defined(LAYRZ_HUB25_BUILD)
  // GPIO debounce timers
  static TimerHandle_t debTimerIO4;
  static TimerHandle_t debTimerIO5;
  static TimerHandle_t debTimerIO6;
  static TimerHandle_t debTimerIO42;
  static TimerHandle_t debTimerIO7;
  static TimerHandle_t debTimerIO8;
  static TimerHandle_t debTimerIO19;
  static TimerHandle_t debTimerIO20;
  // GPIO debounce timer handlers
  static void timerIO4CB(TimerHandle_t xTimer);
  static void timerIO5CB(TimerHandle_t xTimer);
  static void timerIO6CB(TimerHandle_t xTimer);
  static void timerIO42CB(TimerHandle_t xTimer);
  static void timerIO7CB(TimerHandle_t xTimer);
  static void timerIO8CB(TimerHandle_t xTimer);
  static void timerIO19CB(TimerHandle_t xTimer);
  static void timerIO20CB(TimerHandle_t xTimer);
  // GPIO interrupt handlers
  static void IO4Handler();  // DI1 handler
  static void IO5Handler();  // DI2 handler
  static void IO6Handler();  // DI3 handler
  static void IO42Handler(); // DI4 handler
  static void IO7Handler();  // AI1/DI5 handler
  static void IO8Handler();  // AI2/DI6 handler
  static void IO19Handler(); // IO19 handler
  static void IO20Handler(); // IO20 handler
#endif
};

#endif