#include <modules/gpio/GpioLayrzHub.h>

#if defined(LAYRZ_HUB1_BUILD)
volatile bool blockHandlerIO1 = false;
volatile bool blockHandlerIO2 = false;
volatile bool blockHandlerIO5 = false;
volatile bool blockHandlerIO6 = false;
volatile bool blockHandlerIO7 = false;
volatile bool blockHandlerIO14 = false;
TimerHandle_t GpioLayrzHub::debTimerIO1 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO2 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO5 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO6 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO7 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO14 = nullptr;

#elif defined(LAYRZ_HUB2_BUILD)
volatile bool blockHandlerIO3 = false;
volatile bool blockHandlerIO4 = false;
volatile bool blockHandlerIO5 = false;
volatile bool blockHandlerIO6 = false;
volatile bool blockHandlerIO19 = false;
volatile bool blockHandlerIO20 = false;
volatile bool blockHandlerIO41 = false;
volatile bool blockHandlerIO42 = false;
TimerHandle_t GpioLayrzHub::debTimerIO3 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO4 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO5 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO6 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO19 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO20 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO41 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO42 = nullptr;
#elif defined(LAYRZ_HUB25_BUILD)
volatile bool blockHandlerIO4 = false;
volatile bool blockHandlerIO5 = false;
volatile bool blockHandlerIO6 = false;
volatile bool blockHandlerIO42 = false;
volatile bool blockHandlerIO7 = false;
volatile bool blockHandlerIO8 = false;
volatile bool blockHandlerIO19 = false;
volatile bool blockHandlerIO20 = false;
TimerHandle_t GpioLayrzHub::debTimerIO4 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO5 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO6 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO42 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO7 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO8 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO19 = nullptr;
TimerHandle_t GpioLayrzHub::debTimerIO20 = nullptr;
#endif

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

TaskHandle_t GpioLayrzHub::gpioEventTaskHandle = nullptr;
QueueHandle_t GpioLayrzHub::gpioEventQueue = nullptr;

GpioLayrzHub::DigitalOutputConfig digitalOutputsConfigs[10];

/* ********************************************************************************************************************
Public Methods
***********************************************************************************************************************/
void GpioLayrzHub::initGPIO() {
  gpioEventQueue = xQueueCreate(10, sizeof(GpioEvent_t));
  xTaskCreatePinnedToCore(gpioEventTask, "gpioEventTask", 4096, nullptr, 1,
                          &gpioEventTaskHandle, 1);
#if defined(LAYRZ_HUB1_BUILD)
  gpioConfigs[0].mode = hubSettings.gpio_1_mode;
  gpioConfigs[1].mode = hubSettings.gpio_2_mode;
  gpioConfigs[2].mode = hubSettings.gpio_5_mode;
  gpioConfigs[3].mode = hubSettings.gpio_6_mode;
  gpioConfigs[4].mode = hubSettings.gpio_7_mode;
  gpioConfigs[5].mode = hubSettings.gpio_14_mode;
  gpioConfigs[6].mode = hubSettings.gpio_45_mode;
  gpioConfigs[7].mode = hubSettings.gpio_46_mode;
  gpioConfigs[8].mode = hubSettings.gpio_47_mode;
#elif defined(LAYRZ_HUB2_BUILD)
  gpioConfigs[0].mode = hubSettings.gpio_3_mode;
  gpioConfigs[1].mode = hubSettings.gpio_4_mode;
  gpioConfigs[2].mode = hubSettings.gpio_5_mode;
  gpioConfigs[3].mode = hubSettings.gpio_6_mode;
  gpioConfigs[4].mode = hubSettings.gpio_19_mode;
  gpioConfigs[5].mode = hubSettings.gpio_20_mode;
  gpioConfigs[6].mode = hubSettings.gpio_41_mode;
  gpioConfigs[7].mode = hubSettings.gpio_42_mode;
#elif defined(LAYRZ_HUB25_BUILD)
  gpioConfigs[0].mode = hubSettings.di1_mode;
  gpioConfigs[1].mode = hubSettings.di2_mode;
  gpioConfigs[2].mode = hubSettings.di3_mode;
  gpioConfigs[3].mode = hubSettings.di4_mode;
  gpioConfigs[4].mode = hubSettings.ai1_di5_mode;
  gpioConfigs[5].mode = hubSettings.ai2_di6_mode;
  gpioConfigs[6].mode = 3; // DO1 is always output
  gpioConfigs[7].mode = 3; // DO2 is always output
  gpioConfigs[8].mode = hubSettings.gpio_19_mode;
  gpioConfigs[9].mode = hubSettings.gpio_20_mode;
#endif

#if defined(LAYRZ_HUB1_BUILD)
  uint8_t possibleOutputs = 6; // IO1, IO2, IO5, IO6, IO7, IO14
#elif defined(LAYRZ_HUB2_BUILD)
  uint8_t possibleOutputs = 8; // IO3, IO4, IO5, IO6, IO19, IO20, IO41, IO42
#elif defined(LAYRZ_HUB25_BUILD)
  uint8_t possibleOutputs =
    2 + (hubSettings.gpio_19_mode == 3 ? 1 : 0) +
    (hubSettings.gpio_20_mode == 3
       ? 1
       : 0); // DO1, DO2, GPIO19 (if output), GPIO20 (if output)
#endif

  for (int i = 0; i < possibleOutputs; i++) {
    digitalOutputsConfigs[i].timer =
      xTimerCreate("digitalOutputTimer", pdMS_TO_TICKS(1000), pdFALSE,
                   (void *)i, digitalOutputTimerCallback);
  }
#if defined(LAYRZ_HUB1_BUILD)
  digitalOutputsConfigs[0].pin = IO1;
  digitalOutputsConfigs[1].pin = IO2;
  digitalOutputsConfigs[2].pin = IO5;
  digitalOutputsConfigs[3].pin = IO6;
  digitalOutputsConfigs[4].pin = IO7;
  digitalOutputsConfigs[5].pin = IO14;
#elif defined(LAYRZ_HUB2_BUILD)
  digitalOutputsConfigs[0].pin = IO3;
  digitalOutputsConfigs[1].pin = IO4;
  digitalOutputsConfigs[2].pin = IO5;
  digitalOutputsConfigs[3].pin = IO6;
  digitalOutputsConfigs[4].pin = IO19;
  digitalOutputsConfigs[5].pin = IO20;
  digitalOutputsConfigs[6].pin = IO41;
  digitalOutputsConfigs[7].pin = IO42;
#elif defined(LAYRZ_HUB25_BUILD)
  digitalOutputsConfigs[0].pin = DO1;
  digitalOutputsConfigs[1].pin = DO2;
  digitalOutputsConfigs[2].pin = IO19;
  digitalOutputsConfigs[3].pin = IO20;
#endif

  // ADC1 configuration
  adc1_config_width(ADC_WIDTH_BIT_12);
  // Configure and calibrate each GPIO's ADC channel on ADC1

#if defined(LAYRZ_HUB1_BUILD)
  if (hubSettings.gpio_1_mode == 4) {
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12); // GPIO 1
    adc_chars_gpio1 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio1);
  }
  if (hubSettings.gpio_2_mode == 4) {
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_12); // GPIO 2
    adc_chars_gpio2 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio2);
  }
  if (hubSettings.gpio_5_mode == 4) {
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_12); // GPIO 5
    adc_chars_gpio5 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio5);
  }
  if (hubSettings.gpio_6_mode == 4) {
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_12); // GPIO 6
    adc_chars_gpio6 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio6);
  }
  if (hubSettings.gpio_7_mode == 4) {
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_12); // GPIO 7
    adc_chars_gpio7 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio7);
  }
  if (hubSettings.gpio_14_mode == 4) {
    adc2_config_channel_atten(ADC2_CHANNEL_3, ADC_ATTEN_DB_12); // GPIO 14
    adc_chars_gpio14 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio14);
  }

#elif defined(LAYRZ_HUB2_BUILD)
  if (hubSettings.gpio_3_mode == 4) {
    adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_12); // GPIO 3
    adc_chars_gpio3 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio3);
  }
  if (hubSettings.gpio_4_mode == 4) {
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_12); // GPIO 4
    adc_chars_gpio4 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio4);
  }
  if (hubSettings.gpio_5_mode == 4) {
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_12); // GPIO 5
    adc_chars_gpio5 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio5);
  }
  if (hubSettings.gpio_6_mode == 4) {
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_12); // GPIO 6
    adc_chars_gpio6 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio6);
  }
  if (hubSettings.gpio_19_mode == 4) {
    adc2_config_channel_atten(ADC2_CHANNEL_8, ADC_ATTEN_DB_12); // GPIO 19
    adc_chars_gpio19 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio19);
  }
  if (hubSettings.gpio_20_mode == 4) {
    adc2_config_channel_atten(ADC2_CHANNEL_9, ADC_ATTEN_DB_12); // GPIO 20
    adc_chars_gpio20 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio20);
  }
  if (hubSettings.gpio_41_mode == 4) {
    adc2_config_channel_atten(ADC2_CHANNEL_0, ADC_ATTEN_DB_12); // GPIO 41
    adc_chars_gpio41 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio41);
  }
  if (hubSettings.gpio_42_mode == 4) {
    adc2_config_channel_atten(ADC2_CHANNEL_1, ADC_ATTEN_DB_12); // GPIO 42
    adc_chars_gpio42 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio42);
  }

#elif defined(LAYRZ_HUB25_BUILD)
  if (hubSettings.ai1_di5_mode == 4) {
    adc1_config_channel_atten(ADC1_CHANNEL_3,
                              ADC_ATTEN_DB_12); // AI1/DI5 (GPIO 7)
    adc_chars_ai1 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_ai1);
  }

  if (hubSettings.ai2_di6_mode == 4) {
    adc1_config_channel_atten(ADC1_CHANNEL_4,
                              ADC_ATTEN_DB_12); // AI2/DI6 (GPIO 8)
    adc_chars_ai2 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_ai2);
  }
  if (hubSettings.gpio_19_mode == 4) {
    adc2_config_channel_atten(ADC2_CHANNEL_8, ADC_ATTEN_DB_12); // GPIO 19
    adc_chars_gpio19 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio19);
  }
  if (hubSettings.gpio_20_mode == 4) {
    adc2_config_channel_atten(ADC2_CHANNEL_9, ADC_ATTEN_DB_12); // GPIO 20
    adc_chars_gpio20 = (esp_adc_cal_characteristics_t *)calloc(
      1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3300, adc_chars_gpio20);
  }

#endif

  // Configure the LEDC timer for 12-bit resolution and 10 kHz frequency
  ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE, // Low-speed mode for compatibility
    .duty_resolution = PWM_RESOLUTION, // 12-bit duty resolution
    .timer_num = LEDC_TIMER_0,         // Use timer 0 for this configuration
    .freq_hz = PWM_FREQUENCY,          // Set output frequency to 10 kHz
    .clk_cfg = LEDC_AUTO_CLK           // Auto-select the clock source
  };
  ledc_timer_config(&ledc_timer);

#if defined(LAYRZ_HUB1_BUILD)
  for (int i = 0; i < 9; i++) {
    if (gpioConfigs[i].mode == 2 || gpioConfigs[i].mode == 11) {
      pinMode(gpioConfigs[i].pin, INPUT_PULLUP);
      if (gpioConfigs[i].pin == IO1) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO1Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO2) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO2Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO5) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO5Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO6) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO6Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO7) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO7Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO14) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO14Handler,
                        FALLING);
      }
    } else if (gpioConfigs[i].mode == 1) {
      pinMode(gpioConfigs[i].pin, INPUT_PULLDOWN);
      if (gpioConfigs[i].pin == IO1) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO1Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO2) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO2Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO5) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO5Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO6) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO6Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO7) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO7Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO14) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO14Handler,
                        RISING);
      }
    } else if (gpioConfigs[i].mode == 3 || gpioConfigs[i].mode == 5) {
      pinMode(gpioConfigs[i].pin, OUTPUT);
      if (gpioConfigs[i].mode == 5) {
        ledc_channel_config_t ledc_channel = {
          .gpio_num = gpioConfigs[i].pin, // Set the GPIO for this channel
          .speed_mode = LEDC_LOW_SPEED_MODE,
          .channel = pwmChannels[i],
          .timer_sel = LEDC_TIMER_0, // Use the configured timer 0
          .duty = 0,                 // Set initial duty cycle to 0%
          .hpoint = 0};
        ledc_channel_config(&ledc_channel);
      }
    } else if (gpioConfigs[i].mode == 0 || gpioConfigs[i].mode == 4 ||
               gpioConfigs[i].mode == 9 || gpioConfigs[i].mode == 10) {
      pinMode(gpioConfigs[i].pin, INPUT);
    }
  }
  for (int i = 0; i < 6; i++) {
    if (gpioConfigs[i].mode == 3) {           // digital output
      digitalWrite(gpioConfigs[i].pin, HIGH); // Set initial state to HIGH
    }
  }

#elif defined(LAYRZ_HUB2_BUILD)
  pinMode(IO7, INPUT); // GPIO 7 is always input in LAYRZ HUB2 for battery
                       // voltage measurement
  for (int i = 0; i < 8; i++) {
    if (gpioConfigs[i].mode == 2 || gpioConfigs[i].mode == 11) {
      pinMode(gpioConfigs[i].pin, INPUT_PULLUP);
      if (gpioConfigs[i].pin == IO3) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO3Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO4) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO4Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO5) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO5Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO6) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO6Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO19) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO19Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO20) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO20Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO41) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO41Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO42) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO42Handler,
                        FALLING);
      }
    } else if (gpioConfigs[i].mode == 1) {
      pinMode(gpioConfigs[i].pin, INPUT_PULLDOWN);
      if (gpioConfigs[i].pin == IO3) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO3Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO4) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO4Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO5) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO5Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO6) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO6Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO19) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO19Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO20) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO20Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO41) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO41Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO42) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO42Handler,
                        RISING);
      }
    } else if (gpioConfigs[i].mode == 3 || gpioConfigs[i].mode == 5) {
      pinMode(gpioConfigs[i].pin, OUTPUT);
      if (gpioConfigs[i].mode == 5) {
        ledc_channel_config_t ledc_channel = {
          .gpio_num = gpioConfigs[i].pin, // Set the GPIO for this channel
          .speed_mode = LEDC_LOW_SPEED_MODE,
          .channel = pwmChannels[i],
          .timer_sel = LEDC_TIMER_0, // Use the configured timer 0
          .duty = 0,                 // Set initial duty cycle to 0%
          .hpoint = 0};
        ledc_channel_config(&ledc_channel);
      }
    }

    else if (gpioConfigs[i].mode == 0 || gpioConfigs[i].mode == 4 ||
             gpioConfigs[i].mode == 9 || gpioConfigs[i].mode == 10) {
      pinMode(gpioConfigs[i].pin, INPUT);
    }
  }
  for (int i = 0; i < 8; i++) {
    if (gpioConfigs[i].mode == 3) {           // digital output
      digitalWrite(gpioConfigs[i].pin, HIGH); // Set initial state to HIGH
    }
  }
#elif defined(LAYRZ_HUB25_BUILD)
  for (int i = 0; i < 10; i++) {
    if (gpioConfigs[i].mode == 2 || gpioConfigs[i].mode == 11) {
      pinMode(gpioConfigs[i].pin, INPUT_PULLUP);
      if (gpioConfigs[i].pin == DI1) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO4Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == DI2) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO5Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == DI3) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO6Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == DI4) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO42Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == AI1_DI5) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO7Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == AI2_DI6) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO8Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO19) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO19Handler,
                        FALLING);
      } else if (gpioConfigs[i].pin == IO20) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO20Handler,
                        FALLING);
      }
    } else if (gpioConfigs[i].mode == 1) {
      pinMode(gpioConfigs[i].pin, INPUT_PULLDOWN);
      if (gpioConfigs[i].pin == DI1) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO4Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == DI2) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO5Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == DI3) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO6Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == DI4) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO42Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == AI1_DI5) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO7Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == AI2_DI6) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO8Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO19) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO19Handler,
                        RISING);
      } else if (gpioConfigs[i].pin == IO20) {
        attachInterrupt(digitalPinToInterrupt(gpioConfigs[i].pin), IO20Handler,
                        RISING);
      }
    } else if (gpioConfigs[i].mode == 3) {
      pinMode(gpioConfigs[i].pin, OUTPUT);
    } else if (gpioConfigs[i].mode == 4) {
      pinMode(gpioConfigs[i].pin, INPUT);
    }
  }
  pinMode(VBAT_SENSE,
          INPUT);       // VBAT_SENSE is always input in LAYRZ HUB25 for battery
                        // voltage measurement
  pinMode(DO1, OUTPUT); // DO1 is always output in LAYRZ HUB25
  pinMode(DO2, OUTPUT); // DO2 is always output in LAYR
  digitalWrite(DO1, HIGH); // Set initial state of DO1 to HIGH
  digitalWrite(DO2, HIGH); // Set initial state of DO2 to HIGH
#endif

  if (!createDebTimers()) {
    debugPrint("Failed to create one or more debounce timers\n");
  }
}

void GpioLayrzHub::gpioEventTask(void *pvParameters) {
  // GpioLayrzHub* self = static_cast<GpioLayrzHub*>(pvParameters);
  GpioLayrzHub *self = nullptr; // not used, safe to ignore
  GpioEvent_t event;

  while (true) {
    if (xQueueReceive(self->gpioEventQueue, &event, portMAX_DELAY)) {
      BaseType_t result;
#if defined(LAYRZ_HUB1_BUILD)
      switch (event.pinIndex) {
      case 1:
        if (debTimerIO1) {
          result = xTimerReset(debTimerIO1, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO1\n");
        }
        break;
      case 2:
        if (debTimerIO2) {
          result = xTimerReset(debTimerIO2, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO2\n");
        }
        break;
      case 5:
        if (debTimerIO5) {
          result = xTimerReset(debTimerIO5, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO5\n");
        }
        break;
      case 6:
        if (debTimerIO6) {
          result = xTimerReset(debTimerIO6, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO6\n");
        }
        break;
      case 7:
        if (debTimerIO7) {
          result = xTimerReset(debTimerIO7, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO7\n");
        }
        break;
      case 14:
        if (debTimerIO14) {
          result = xTimerReset(debTimerIO14, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO14\n");
        }
        break;
      }
#elif defined(LAYRZ_HUB2_BUILD)
      switch (event.pinIndex) {
      case 3:
        if (debTimerIO3) {
          result = xTimerReset(debTimerIO3, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO3\n");
        }
        break;
      case 4:
        if (debTimerIO4) {
          result = xTimerReset(debTimerIO4, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO4\n");
        }
        break;
      case 5:
        if (debTimerIO5) {
          result = xTimerReset(debTimerIO5, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO5\n");
        }
        break;
      case 6:
        if (debTimerIO6) {
          result = xTimerReset(debTimerIO6, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO6\n");
        }
        break;
      case 19:
        if (debTimerIO19) {
          result = xTimerReset(debTimerIO19, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO19\n");
        }
        break;
      case 20:
        if (debTimerIO20) {
          result = xTimerReset(debTimerIO20, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO20\n");
        }
        break;
      case 41:
        if (debTimerIO41) {
          result = xTimerReset(debTimerIO41, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO41\n");
        }
        break;
      case 42:
        if (debTimerIO42) {
          result = xTimerReset(debTimerIO42, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO42\n");
        }
        break;
      }
#elif defined(LAYRZ_HUB25_BUILD)
      switch (event.pinIndex) {
      case 4:
        if (debTimerIO4) {
          result = xTimerReset(debTimerIO4, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO4\n");
        }
        break;
      case 5:
        if (debTimerIO5) {
          result = xTimerReset(debTimerIO5, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO5\n");
        }
        break;
      case 6:
        if (debTimerIO6) {
          result = xTimerReset(debTimerIO6, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO6\n");
        }
        break;
      case 42:
        if (debTimerIO42) {
          result = xTimerReset(debTimerIO42, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO42\n");
        }
        break;
      case 7:
        if (debTimerIO7) {
          result = xTimerReset(debTimerIO7, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO7\n");
        }
        break;
      case 8:
        if (debTimerIO8) {
          result = xTimerReset(debTimerIO8, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO8\n");
        }
        break;
      case 19:
        if (debTimerIO19) {
          result = xTimerReset(debTimerIO19, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO19\n");
        }
        break;
      case 20:
        if (debTimerIO20) {
          result = xTimerReset(debTimerIO20, 0);
          if (result != pdPASS)
            debugPrint("Failed to reset debTimerIO20\n");
        }
        break;
      }
#endif
      if (event.triggerCamera) {
        ArducamLayrzHub::takePicture(); // Call the function to take a picture
      }
    }
  }
}

void GpioLayrzHub::setPWMDuty(int channel, uint32_t duty) {
  if (duty > PWM_MAX_DUTY)
    duty = PWM_MAX_DUTY; // Clamp duty cycle to max value
  ledc_set_duty(LEDC_LOW_SPEED_MODE, pwmChannels[channel], duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, pwmChannels[channel]);
}

bool GpioLayrzHub::createDebTimers() {
#if defined(LAYRZ_HUB1_BUILD)
  if (gpioConfigs[0].mode == 1 || gpioConfigs[0].mode == 2 ||
      gpioConfigs[0].mode == 11) {
    debTimerIO1 = xTimerCreate("debTimerIO1", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)0, timerIO1CB);
    if (debTimerIO1 == NULL) {
      return false;
    }
    // xTimerStart(debTimerIO1, 0);
    debugPrint("Debounce timer for GPIO1 created\n");
  }
  if (gpioConfigs[1].mode == 1 || gpioConfigs[1].mode == 2 ||
      gpioConfigs[1].mode == 11) {
    debTimerIO2 = xTimerCreate("debTimerIO2", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)1, timerIO2CB);
    if (debTimerIO2 == NULL) {
      return false;
    }
    // xTimerStart(debTimerIO2, 0);
    debugPrint("Debounce timer for GPIO2 created\n");
  }
  if (gpioConfigs[2].mode == 1 || gpioConfigs[2].mode == 2 ||
      gpioConfigs[2].mode == 11) {
    debTimerIO5 = xTimerCreate("debTimerIO5", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)2, timerIO5CB);
    if (debTimerIO5 == NULL) {
      return false;
    }
    // xTimerStart(debTimerIO5, 0);
    debugPrint("Debounce timer for GPIO5 created\n");
  }
  if (gpioConfigs[3].mode == 1 || gpioConfigs[3].mode == 2 ||
      gpioConfigs[3].mode == 11) {
    debTimerIO6 = xTimerCreate("debTimerIO6", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)3, timerIO6CB);
    if (debTimerIO6 == NULL) {
      return false;
    }
    // xTimerStart(debTimerIO6, 0);
    debugPrint("Debounce timer for GPIO6 created\n");
  }
  if (gpioConfigs[4].mode == 1 || gpioConfigs[4].mode == 2 ||
      gpioConfigs[4].mode == 11) {
    debTimerIO7 = xTimerCreate("debTimerIO7", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)4, timerIO7CB);
    if (debTimerIO7 == NULL) {
      return false;
    }
    // xTimerStart(debTimerIO7, 0);
    debugPrint("Debounce timer for GPIO7 created\n");
  }
  if (gpioConfigs[5].mode == 1 || gpioConfigs[5].mode == 2 ||
      gpioConfigs[5].mode == 11) {
    debTimerIO14 = xTimerCreate("debTimerIO14", pdMS_TO_TICKS(100), pdFALSE,
                                (void *)5, timerIO14CB);
    if (debTimerIO14 == NULL) {
      return false;
    }
    // xTimerStart(debTimerIO14, 0);
    debugPrint("Debounce timer for GPIO14 created\n");
  }
#elif defined(LAYRZ_HUB2_BUILD)
  if (gpioConfigs[0].mode == 1 || gpioConfigs[0].mode == 2 ||
      gpioConfigs[0].mode == 11) {
    debTimerIO3 = xTimerCreate("debTimerIO3", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)0, timerIO3CB);
    if (debTimerIO3 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO3 created\n");
  }
  if (gpioConfigs[1].mode == 1 || gpioConfigs[1].mode == 2 ||
      gpioConfigs[1].mode == 11) {
    debTimerIO4 = xTimerCreate("debTimerIO4", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)1, timerIO4CB);
    if (debTimerIO4 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO4 created\n");
  }
  if (gpioConfigs[2].mode == 1 || gpioConfigs[2].mode == 2 ||
      gpioConfigs[2].mode == 11) {
    debTimerIO5 = xTimerCreate("debTimerIO5", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)2, timerIO5CB);
    if (debTimerIO5 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO5 created\n");
  }
  if (gpioConfigs[3].mode == 1 || gpioConfigs[3].mode == 2 ||
      gpioConfigs[3].mode == 11) {
    debTimerIO6 = xTimerCreate("debTimerIO6", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)3, timerIO6CB);
    if (debTimerIO6 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO6 created\n");
  }
  if (gpioConfigs[4].mode == 1 || gpioConfigs[4].mode == 2 ||
      gpioConfigs[4].mode == 11) {
    debTimerIO19 = xTimerCreate("debTimerIO19", pdMS_TO_TICKS(100), pdFALSE,
                                (void *)4, timerIO19CB);
    if (debTimerIO19 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO19 created\n");
  }
  if (gpioConfigs[5].mode == 1 || gpioConfigs[5].mode == 2 ||
      gpioConfigs[5].mode == 11) {
    debTimerIO20 = xTimerCreate("debTimerIO20", pdMS_TO_TICKS(100), pdFALSE,
                                (void *)5, timerIO20CB);
    if (debTimerIO20 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO20 created\n");
  }
  if (gpioConfigs[6].mode == 1 || gpioConfigs[6].mode == 2 ||
      gpioConfigs[6].mode == 11) {
    debTimerIO41 = xTimerCreate("debTimerIO41", pdMS_TO_TICKS(100), pdFALSE,
                                (void *)6, timerIO41CB);
    if (debTimerIO41 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO41 created\n");
  }
  if (gpioConfigs[7].mode == 1 || gpioConfigs[7].mode == 2 ||
      gpioConfigs[7].mode == 11) {
    debTimerIO42 = xTimerCreate("debTimerIO42", pdMS_TO_TICKS(100), pdFALSE,
                                (void *)7, timerIO42CB);
    if (debTimerIO42 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO42 created\n");
  }
#elif defined(LAYRZ_HUB25_BUILD)
  if (gpioConfigs[0].mode == 1 || gpioConfigs[0].mode == 2 ||
      gpioConfigs[0].mode == 11) {
    debTimerIO4 = xTimerCreate("debTimerIO4", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)0, timerIO4CB);
    if (debTimerIO4 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO4 created\n");
  }
  if (gpioConfigs[1].mode == 1 || gpioConfigs[1].mode == 2 ||
      gpioConfigs[1].mode == 11) {
    debTimerIO5 = xTimerCreate("debTimerIO5", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)1, timerIO5CB);
    if (debTimerIO5 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO5 created\n");
  }
  if (gpioConfigs[2].mode == 1 || gpioConfigs[2].mode == 2 ||
      gpioConfigs[2].mode == 11) {
    debTimerIO6 = xTimerCreate("debTimerIO6", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)2, timerIO6CB);
    if (debTimerIO6 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO6 created\n");
  }
  if (gpioConfigs[3].mode == 1 || gpioConfigs[3].mode == 2 ||
      gpioConfigs[3].mode == 11) {
    debTimerIO42 = xTimerCreate("debTimerIO42", pdMS_TO_TICKS(100), pdFALSE,
                                (void *)3, timerIO42CB);
    if (debTimerIO42 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO42 created\n");
  }
  if (gpioConfigs[4].mode == 1 || gpioConfigs[4].mode == 2 ||
      gpioConfigs[4].mode == 11) {
    debTimerIO7 = xTimerCreate("debTimerIO7", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)4, timerIO7CB);
    if (debTimerIO7 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO7 created\n");
  }
  if (gpioConfigs[5].mode == 1 || gpioConfigs[5].mode == 2 ||
      gpioConfigs[5].mode == 11) {
    debTimerIO8 = xTimerCreate("debTimerIO8", pdMS_TO_TICKS(100), pdFALSE,
                               (void *)5, timerIO8CB);
    if (debTimerIO8 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO8 created\n");
  }
  if (gpioConfigs[8].mode == 1 || gpioConfigs[8].mode == 2 ||
      gpioConfigs[8].mode == 11) {
    debTimerIO19 = xTimerCreate("debTimerIO19", pdMS_TO_TICKS(100), pdFALSE,
                                (void *)8, timerIO19CB);
    if (debTimerIO19 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO19 created\n");
  }
  if (gpioConfigs[9].mode == 1 || gpioConfigs[9].mode == 2 ||
      gpioConfigs[9].mode == 11) {
    debTimerIO20 = xTimerCreate("debTimerIO20", pdMS_TO_TICKS(100), pdFALSE,
                                (void *)9, timerIO20CB);
    if (debTimerIO20 == NULL) {
      return false;
    }
    debugPrint("Debounce timer for GPIO20 created\n");
  }
#endif
  return true;
}

void GpioLayrzHub::printGPIOConfigs() {
#if defined(LAYRZ_HUB1_BUILD)
  int gpioPins = 9;
#elif defined(LAYRZ_HUB2_BUILD)
  int gpioPins = 8;
#elif defined(LAYRZ_HUB25_BUILD)
  int gpioPins = 10;
#endif
  for (int i = 0; i < gpioPins; i++) {
    debugPrint("GPIO%d Mode: %d\n", gpioConfigs[i].pin, gpioConfigs[i].mode);
  }
}

void GpioLayrzHub::printGPIOEventCounters() {
#if defined(LAYRZ_HUB1_BUILD)
  int gpioPins = 6;
#elif defined(LAYRZ_HUB2_BUILD)
  int gpioPins = 8;
#elif defined(LAYRZ_HUB25_BUILD)
  int gpioPins = 10;
#endif
  for (int i = 0; i < gpioPins; i++) {
    debugPrint("GPIO%d Event Counter: %d\n", gpioConfigs[i].pin,
               gpioEventCounters[i]);
  }
}

void GpioLayrzHub::resetGPIOEventCounters() {
#if defined(LAYRZ_HUB1_BUILD)
  int gpioPins = 6;
#elif defined(LAYRZ_HUB2_BUILD)
  int gpioPins = 8;
#elif defined(LAYRZ_HUB25_BUILD)
  int gpioPins = 10;
#endif
  for (int i = 0; i < gpioPins; i++) {
    gpioEventCounters[i] = 0;
  }
}

uint8_t GpioLayrzHub::getBatteryLevel(uint32_t vbatSense_mV) {
  // Convert to VDD (mV)
  uint32_t vdd = (vbatSense_mV * 10) / 6; // divide by 0.6

  if (vdd >= 4200)
    return 100;

  if (vdd <= 2400)
    return 0;

  return (uint8_t)(((vdd - 2400) * 100) / 1800);
}

std::string GpioLayrzHub::getGPIOPayload() {
  std::string payload = "";
#if defined(LAYRZ_HUB1_BUILD)
  for (int i = 0; i < 6; i++) {
    if (gpioConfigs[i].mode == 1 || gpioConfigs[i].mode == 2) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".di:" + std::to_string(digitalRead(gpioConfigs[i].pin)) + ",";
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".counter:" + std::to_string(gpioEventCounters[i]) + ",";
    } else if (gpioConfigs[i].mode == 0) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".di:" + std::to_string(digitalRead(gpioConfigs[i].pin)) + ",";
    } else if (gpioConfigs[i].mode == 3) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".do:" + std::to_string(digitalRead(gpioConfigs[i].pin)) + ",";
    } else if (gpioConfigs[i].mode == 4) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) + ".ai:" +
                 std::to_string(readCalibratedVoltage(gpioConfigs[i].pin)) +
                 ",";
    } else if (gpioConfigs[i].mode == 9) { // NTC 10k
      payload += "gpio." + std::to_string(gpioConfigs[i].pin) +
                 ".temperature:" +
                 floatToString(getNTCTemperature(gpioConfigs[i].pin, 10000, 25,
                                                 3950, 10000, 1)) +
                 ",";
    } else if (gpioConfigs[i].mode == 10) { // NTC 100k
      payload += "gpio." + std::to_string(gpioConfigs[i].pin) +
                 ".temperature:" +
                 floatToString(getNTCTemperature(gpioConfigs[i].pin, 100000, 25,
                                                 3950, 100000, 1)) +
                 ",";
    }
  }
  for (int i = 6; i < 9; i++) {
    if (gpioConfigs[i].mode == 0) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".di:" + std::to_string(digitalRead(gpioConfigs[i].pin)) + ",";
    } else if (gpioConfigs[i].mode == 1) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".do:" + std::to_string(digitalRead(gpioConfigs[i].pin)) + ",";
    }
  }
#elif defined(LAYRZ_HUB2_BUILD)

  for (int i = 0; i < 10; i++) {
    if (gpioConfigs[i].mode == 1 || gpioConfigs[i].mode == 2) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".di:" + std::to_string(digitalRead(gpioConfigs[i].pin)) + ",";
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".counter:" + std::to_string(gpioEventCounters[i]) + ",";
    } else if (gpioConfigs[i].mode == 0) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".di:" + std::to_string(digitalRead(gpioConfigs[i].pin)) + ",";
    } else if (gpioConfigs[i].mode == 3) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".do:" + std::to_string(digitalRead(gpioConfigs[i].pin)) + ",";
    } else if (gpioConfigs[i].mode ==
               4) { // GPIO 19 and 20 are not analog inputs
      payload += "io" + std::to_string(gpioConfigs[i].pin) + ".ai:" +
                 std::to_string(readCalibratedVoltage(gpioConfigs[i].pin)) +
                 ",";
    } else if (gpioConfigs[i].mode == 9) { // NTC 10k
      payload += "gpio." + std::to_string(gpioConfigs[i].pin) +
                 ".temperature:" +
                 floatToString(getNTCTemperature(gpioConfigs[i].pin, 10000, 25,
                                                 3950, 10000, 1)) +
                 ",";
    } else if (gpioConfigs[i].mode == 10) { // NTC 100k
      payload += "gpio." + std::to_string(gpioConfigs[i].pin) +
                 ".temperature:" +
                 floatToString(getNTCTemperature(gpioConfigs[i].pin, 100000, 25,
                                                 3950, 100000, 1)) +
                 ",";
    }
  }
#elif defined(LAYRZ_HUB25_BUILD)
  uint32_t vbat_sense = readCalibratedVoltage(VBAT_SENSE);
  float batteryVoltage = vbat_sense * 0.00166667;
  payload += "battery.voltage:" + floatToString(batteryVoltage) + ",";
  payload +=
    "battery.level:" + std::to_string(getBatteryLevel(vbat_sense)) + ",";
  for (int i = 0; i < 10; i++) {
    if (gpioConfigs[i].mode == 1 || gpioConfigs[i].mode == 2) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".di:" + std::to_string(digitalRead(gpioConfigs[i].pin)) + ",";
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".counter:" + std::to_string(gpioEventCounters[i]) + ",";
    } else if (gpioConfigs[i].mode == 0) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".di:" + std::to_string(digitalRead(gpioConfigs[i].pin)) + ",";
    } else if (gpioConfigs[i].mode == 3) {
      payload += "io" + std::to_string(gpioConfigs[i].pin) +
                 ".do:" + std::to_string(digitalRead(gpioConfigs[i].pin)) + ",";
    } else if (gpioConfigs[i].mode ==
               4) { // GPIO 19 and 20 are not analog inputs
      payload += "io" + std::to_string(gpioConfigs[i].pin) + ".ai:" +
                 std::to_string(readCalibratedVoltage(gpioConfigs[i].pin)) +
                 ",";
    } else if (gpioConfigs[i].mode == 9) { // NTC 10k
      payload += "gpio." + std::to_string(gpioConfigs[i].pin) +
                 ".temperature:" +
                 floatToString(getNTCTemperature(gpioConfigs[i].pin, 10000, 25,
                                                 3950, 10000, 1)) +
                 ",";
    } else if (gpioConfigs[i].mode == 10) { // NTC 100k
      payload += "gpio." + std::to_string(gpioConfigs[i].pin) +
                 ".temperature:" +
                 floatToString(getNTCTemperature(gpioConfigs[i].pin, 100000, 25,
                                                 3950, 100000, 1)) +
                 ",";
    }
  }
#endif

  if (payload.length() > 0) {
    // remove last comma
    payload.pop_back();
  }
  return payload;
}

int GpioLayrzHub::setDO(int pin, int value) {
#if defined(LAYRZ_HUB1_BUILD)
  int gpioPins = 6;
#elif defined(LAYRZ_HUB2_BUILD)
  int gpioPins = 8;
#elif defined(LAYRZ_HUB25_BUILD)
  int gpioPins = 10;
#endif
  for (int i = 0; i < gpioPins; i++) {
    if (gpioConfigs[i].pin == pin) {
      if (gpioConfigs[i].mode != 3) {
        return 1; // invalid gpio mode
      }
      if (value < 0 && value > 1) {
        return 2; // invalid value
      }
      if (value == 0)
        value = 1; // Invert value for digitalWrite
      else if (value == 1)
        value = 0; // Invert value for digitalWrite
      digitalWrite(gpioConfigs[i].pin, value);
      debugPrint("Writing %d to pin %d\n", value, gpioConfigs[i].pin);
      return 0;
    }
  }
  return 3; // invalid gpio pin
}

int GpioLayrzHub::setMultipleDO(uint8_t doEnabled, uint8_t doByte,
                                uint8_t *doDuration) {
  int error = 0;
#if defined(LAYRZ_HUB1_BUILD)
  int gpioPins = 6;
#elif defined(LAYRZ_HUB2_BUILD)
  int gpioPins = 8;
#elif defined(LAYRZ_HUB25_BUILD)
  int gpioPins = 10;
#endif
  for (int i = 0; i < gpioPins; i++) {

    if (bitRead(doByte, i) && bitRead(doEnabled, i)) {
      if (gpioConfigs[i].mode != 3) {
        error = 1; // invalid gpio mode
        debugPrint("Invalid GPIO mode for pin %d\n", gpioConfigs[i].pin);
        continue;
      }

      digitalWrite(gpioConfigs[i].pin,
                   LOW); // Set pin LOW to turn on the output

      if (doDuration[i] > 0) {
        if (xTimerIsTimerActive(digitalOutputsConfigs[i].timer)) {
          xTimerStop(digitalOutputsConfigs[i].timer, 0);
        }
        xTimerChangePeriod(digitalOutputsConfigs[i].timer,
                           pdMS_TO_TICKS(doDuration[i] * 1000), 0);
        xTimerStart(digitalOutputsConfigs[i].timer, 0);
      }
    } else if (bitRead(doByte, i) == 0 && bitRead(doEnabled, i) == 1) {
      if (gpioConfigs[i].mode != 3) {
        error = 1; // invalid gpio mode
        debugPrint("Invalid GPIO mode for pin %d\n", gpioConfigs[i].pin);
        continue;
      }
      digitalWrite(gpioConfigs[i].pin,
                   HIGH); // Set pin HIGH to turn off the output
      if (xTimerIsTimerActive(digitalOutputsConfigs[i].timer)) {
        xTimerStop(digitalOutputsConfigs[i].timer, 0);
      }
    }
  }
  return error;
}

void GpioLayrzHub::digitalOutputTimerCallback(TimerHandle_t xTimer) {
  int index = (int)pvTimerGetTimerID(xTimer);
  if (index >= 0 && index < 6) {
    digitalWrite(digitalOutputsConfigs[index].pin,
                 HIGH); // Set pin HIGH to turn off the output
  }
}

int GpioLayrzHub::setPwmOutput(int pin, int value) {
  for (int i = 0; i < 9; i++) {
    if (gpioConfigs[i].pin == pin) {
      if (gpioConfigs[i].mode != 5) {
        return 1; // invalid gpio mode
      }
      debugPrint("Value: %d\n", value);
      if (value < 0 || value > 100) {
        return 2; // invalid value
      }
      uint32_t duty = (value * PWM_MAX_DUTY) / 100;
      debugPrint("Duty: %d\n", duty);
      setPWMDuty(i, duty);
      return 0;
    }
  }
  return 3; // invalid gpio pin
}

/* ********************************************************************************************************************
Private Methods
***********************************************************************************************************************/

uint32_t GpioLayrzHub::readCalibratedVoltage(int pin) {
  int adc_reading = 0; // Using int for compatibility with adc2_get_raw
  esp_adc_cal_characteristics_t *adc_chars = nullptr;
  adc_unit_t unit;

  // Define channel variables specifically for each ADC unit
  adc1_channel_t adc1_channel;
  adc2_channel_t adc2_channel;
  bool is_adc1 = true;

#if defined(LAYRZ_HUB1_BUILD)
  // Assign channel and unit based on pin number
  if (pin == 1) {
    adc1_channel = ADC1_CHANNEL_0;
    adc_chars = adc_chars_gpio1;
    unit = ADC_UNIT_1;
    is_adc1 = true;
  } else if (pin == 2) {
    adc1_channel = ADC1_CHANNEL_1;
    adc_chars = adc_chars_gpio2;
    unit = ADC_UNIT_1;
    is_adc1 = true;
  } else if (pin == 5) {
    adc1_channel = ADC1_CHANNEL_4;
    adc_chars = adc_chars_gpio5;
    unit = ADC_UNIT_1;
    is_adc1 = true;
  } else if (pin == 6) {
    adc1_channel = ADC1_CHANNEL_5;
    adc_chars = adc_chars_gpio6;
    unit = ADC_UNIT_1;
    is_adc1 = true;
  } else if (pin == 7) {
    adc1_channel = ADC1_CHANNEL_6;
    adc_chars = adc_chars_gpio7;
    unit = ADC_UNIT_1;
    is_adc1 = true;
  } else if (pin == 14) {
    adc2_channel = ADC2_CHANNEL_3; // GPIO14 is mapped to ADC2_CHANNEL_3
    adc_chars = adc_chars_gpio14;
    unit = ADC_UNIT_2;
    is_adc1 = false;
#elif defined(LAYRZ_HUB2_BUILD)
  if (pin == 3) {
    adc1_channel = ADC1_CHANNEL_2;
    adc_chars = adc_chars_gpio3;
    unit = ADC_UNIT_1;
    is_adc1 = true;
  } else if (pin == 4) {
    adc1_channel = ADC1_CHANNEL_3;
    adc_chars = adc_chars_gpio4;
    unit = ADC_UNIT_1;
    is_adc1 = true;
  } else if (pin == 5) {
    adc1_channel = ADC1_CHANNEL_4;
    adc_chars = adc_chars_gpio5;
    unit = ADC_UNIT_1;
    is_adc1 = true;
  } else if (pin == 6) {
    adc1_channel = ADC1_CHANNEL_5;
    adc_chars = adc_chars_gpio6;
    unit = ADC_UNIT_1;
    is_adc1 = true;
  } else if (pin == 19) {
    adc2_channel = ADC2_CHANNEL_8;
    adc_chars = adc_chars_gpio19;
    unit = ADC_UNIT_2;
    is_adc1 = false;
  } else if (pin == 20) {
    adc2_channel = ADC2_CHANNEL_9;
    adc_chars = adc_chars_gpio20;
    unit = ADC_UNIT_2;
    is_adc1 = false;
#elif defined(LAYRZ_HUB25_BUILD)
  if (pin == AI1_DI5) {
    adc1_channel = ADC1_CHANNEL_6;
    adc_chars = adc_chars_ai1;
    unit = ADC_UNIT_1;
    is_adc1 = true;
  } else if (pin == AI2_DI6) {
    adc1_channel = ADC1_CHANNEL_7;
    adc_chars = adc_chars_ai2;
    unit = ADC_UNIT_1;
    is_adc1 = true;
  } else if (pin == 19) {
    adc2_channel = ADC2_CHANNEL_8;
    adc_chars = adc_chars_gpio19;
    unit = ADC_UNIT_2;
    is_adc1 = false;
  } else if (pin == 20) {
    adc2_channel = ADC2_CHANNEL_9;
    adc_chars = adc_chars_gpio20;
    unit = ADC_UNIT_2;
    is_adc1 = false;
  } else if (pin == VBAT_SENSE) {
    adc1_channel = ADC1_CHANNEL_2; // GPIO3 is mapped to ADC1_CHANNEL_2
    adc_chars = adc_chars_vbat;
    unit = ADC_UNIT_1;
    is_adc1 = true;
#endif
  } else {
    debugPrint("Invalid GPIO pin for ADC reading\n");
    return 0;
  }

  // Null check for adc_chars before using it
  if (adc_chars == nullptr) {
    debugPrint("Error: ADC characteristics pointer is null\n");
    return 0;
  }

  // Read raw ADC value
  if (is_adc1) {
    adc_reading = adc1_get_raw(adc1_channel);
  } else {
    adc2_get_raw(adc2_channel, ADC_WIDTH_BIT_12, &adc_reading);
  }

  // Convert to calibrated voltage and return
  return esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
}

float GpioLayrzHub::getNTCTemperature(int pin, int r0, int t0, int beta, int r1,
                                      int mode) {
  // mode 0: NTC with pull-up resistor, mode 1: NTC with pull-down resistor
  // R1 is the value of the pull-up or pull-down resistor
  // R0 is the resistance of the NTC at temperature T0
  // Beta is the beta value of the NTC
  float pinVoltage = (float)readMedianVoltage(pin) * 0.001;
  debugPrint("Pin voltage: %f V\n", pinVoltage);
  float NTCResistance = 0;
  if (mode == 0) { // Pull-up resistor
    NTCResistance = r1 * (pinVoltage / (3.3 - pinVoltage));
  } else if (mode == 1) { // Pull-down resistor
    NTCResistance = r1 * ((3.3 - pinVoltage) / pinVoltage);
  }
  debugPrint("NTC Resistance: %f Ohm\n", NTCResistance);
  // Calculate temperature using Steinhart-Hart equation
  float temperature =
    1.0 / ((1.0 / (t0 + 273.15)) + (log(NTCResistance / r0) / beta)) - 273.15;
  // debugPrint("Temperature: %f °C\n", temperature);
  if (r0 == 10000) {
    if (temperature < -55 || temperature > 125) {
      return NAN;
    }
  } else if (r0 == 100000) {
    if (temperature < -50 || temperature > 260) {
      return NAN;
    }
  }
  return temperature;
}

uint32_t GpioLayrzHub::readAvgVoltage(int pin, int samples) {
  uint32_t sum = 0;
  for (int i = 0; i < samples; ++i) {
    sum += readCalibratedVoltage(pin);
    vTaskDelay(1);
  }
  return sum / samples;
}

uint32_t GpioLayrzHub::readMedianVoltage(int pin, int samples) {
  std::vector<uint32_t> readings;

  // Collect voltage readings
  for (int i = 0; i < samples; ++i) {
    readings.push_back(readCalibratedVoltage(pin));
  }

  // Sort the readings to find the median
  std::sort(readings.begin(), readings.end());

  // Return the median value
  if (samples % 2 == 0) {
    // For an even number of samples, return the average of the two middle
    // elements
    return (readings[samples / 2 - 1] + readings[samples / 2]) / 2;
  } else {
    // For an odd number of samples, return the middle element
    return readings[samples / 2];
  }
}

#if defined(LAYRZ_HUB1_BUILD)
void GpioLayrzHub::timerIO1CB(TimerHandle_t xTimer) {
  // blockHandlerIO1 = false;  // Re-enable interrupt handling
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO1 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO1 debounce timer expired\n");
}

void GpioLayrzHub::timerIO2CB(TimerHandle_t xTimer) {
  // blockHandlerIO2 = false;  // Re-enable interrupt handling
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO2 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO2 debounce timer expired\n");
}

void GpioLayrzHub::timerIO5CB(TimerHandle_t xTimer) {
  // blockHandlerIO5 = false;  // Re-enable interrupt handling
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO5 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO5 debounce timer expired\n");
}

void GpioLayrzHub::timerIO6CB(TimerHandle_t xTimer) {
  // blockHandlerIO6 = false;  // Re-enable interrupt handling
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO6 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO6 debounce timer expired\n");
}

void GpioLayrzHub::timerIO7CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO7 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO7 debounce timer expired\n");
}

void GpioLayrzHub::timerIO14CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO14 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO14 debounce timer expired\n");
}

#elif defined(LAYRZ_HUB2_BUILD)

void GpioLayrzHub::timerIO3CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO3 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO3 debounce timer expired\n");
}

void GpioLayrzHub::timerIO4CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO4 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO4 debounce timer expired\n");
}

void GpioLayrzHub::timerIO5CB(TimerHandle_t xTimer) {
  // blockHandlerIO5 = false;  // Re-enable interrupt handling
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO5 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO5 debounce timer expired\n");
}

void GpioLayrzHub::timerIO6CB(TimerHandle_t xTimer) {
  // blockHandlerIO6 = false;  // Re-enable interrupt handling
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO6 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO6 debounce timer expired\n");
}

void GpioLayrzHub::timerIO19CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO19 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO19 debounce timer expired\n");
}

void GpioLayrzHub::timerIO20CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO20 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO20 debounce timer expired\n");
}

void GpioLayrzHub::timerIO41CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO41 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO41 debounce timer expired\n");
}

void GpioLayrzHub::timerIO42CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO42 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO42 debounce timer expired\n");
}

#elif defined(LAYRZ_HUB25_BUILD)

void GpioLayrzHub::timerIO4CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO4 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO4 debounce timer expired\n");
}

void GpioLayrzHub::timerIO5CB(TimerHandle_t xTimer) {
  // blockHandlerIO5 = false;  // Re-enable interrupt handling
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO5 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO5 debounce timer expired\n");
}

void GpioLayrzHub::timerIO6CB(TimerHandle_t xTimer) {
  // blockHandlerIO6 = false;  // Re-enable interrupt handling
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO6 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO6 debounce timer expired\n");
}

void GpioLayrzHub::timerIO42CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO42 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO42 debounce timer expired\n");
}

void GpioLayrzHub::timerIO7CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO7 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO7 debounce timer expired\n");
}

void GpioLayrzHub::timerIO8CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO8 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO8 debounce timer expired\n");
}

void GpioLayrzHub::timerIO19CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO19 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO19 debounce timer expired\n");
}

void GpioLayrzHub::timerIO20CB(TimerHandle_t xTimer) {
  portENTER_CRITICAL_ISR(&mux);
  blockHandlerIO20 = false;
  portEXIT_CRITICAL_ISR(&mux);
  // debugPrint("IO20 debounce timer expired\n");
}
#endif

#if defined(LAYRZ_HUB1_BUILD)

void IRAM_ATTR GpioLayrzHub::IO1Handler() {
  if (!blockHandlerIO1) {
    blockHandlerIO1 = true;
    gpioEventCounters[0]++;
    GpioEvent_t evt = {.pinIndex = 1, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 1) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO2Handler() {
  if (!blockHandlerIO2) {
    blockHandlerIO2 = true;
    gpioEventCounters[1]++;
    GpioEvent_t evt = {.pinIndex = 2, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 2) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO5Handler() {
  if (!blockHandlerIO5) {
    blockHandlerIO5 = true;
    gpioEventCounters[2]++;
    GpioEvent_t evt = {.pinIndex = 5, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 5) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO6Handler() {
  if (!blockHandlerIO6) {
    blockHandlerIO6 = true;
    gpioEventCounters[3]++;
    GpioEvent_t evt = {.pinIndex = 6, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 6) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO7Handler() {
  if (!blockHandlerIO7) {
    blockHandlerIO7 = true;
    gpioEventCounters[4]++;
    GpioEvent_t evt = {.pinIndex = 7, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 7) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO14Handler() {
  if (!blockHandlerIO14) {
    blockHandlerIO14 = true;
    gpioEventCounters[5]++;
    GpioEvent_t evt = {.pinIndex = 14, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 14) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}
#elif defined(LAYRZ_HUB2_BUILD)
void IRAM_ATTR GpioLayrzHub::IO3Handler() {
  if (!blockHandlerIO3) {
    blockHandlerIO3 = true;
    gpioEventCounters[0]++;
    GpioEvent_t evt = {.pinIndex = 3, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 3) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO4Handler() {
  if (!blockHandlerIO4) {
    blockHandlerIO4 = true;
    gpioEventCounters[1]++;
    GpioEvent_t evt = {.pinIndex = 4, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 4) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO5Handler() {
  if (!blockHandlerIO5) {
    blockHandlerIO5 = true;
    gpioEventCounters[2]++;
    GpioEvent_t evt = {.pinIndex = 5, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 5) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO6Handler() {
  if (!blockHandlerIO6) {
    blockHandlerIO6 = true;
    gpioEventCounters[3]++;
    GpioEvent_t evt = {.pinIndex = 6, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 6) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO19Handler() {
  if (!blockHandlerIO19) {
    blockHandlerIO19 = true;
    gpioEventCounters[4]++;
    GpioEvent_t evt = {.pinIndex = 19, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 19) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO20Handler() {
  if (!blockHandlerIO20) {
    blockHandlerIO20 = true;
    gpioEventCounters[5]++;
    GpioEvent_t evt = {.pinIndex = 20, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 20) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO41Handler() {
  if (!blockHandlerIO41) {
    blockHandlerIO41 = true;
    gpioEventCounters[6]++;
    GpioEvent_t evt = {.pinIndex = 41, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 41) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO42Handler() {
  if (!blockHandlerIO42) {
    blockHandlerIO42 = true;
    gpioEventCounters[7]++;
    GpioEvent_t evt = {.pinIndex = 42, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 42) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

#elif defined(LAYRZ_HUB25_BUILD)
void IRAM_ATTR GpioLayrzHub::IO4Handler() {
  if (!blockHandlerIO4) {
    blockHandlerIO4 = true;
    gpioEventCounters[0]++;
    GpioEvent_t evt = {.pinIndex = 4, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 4) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO5Handler() {
  if (!blockHandlerIO5) {
    blockHandlerIO5 = true;
    gpioEventCounters[1]++;
    GpioEvent_t evt = {.pinIndex = 5, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 5) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO6Handler() {
  if (!blockHandlerIO6) {
    blockHandlerIO6 = true;
    gpioEventCounters[2]++;
    GpioEvent_t evt = {.pinIndex = 6, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 6) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO42Handler() {
  if (!blockHandlerIO42) {
    blockHandlerIO42 = true;
    gpioEventCounters[3]++;
    GpioEvent_t evt = {.pinIndex = 42, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 42) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO7Handler() {
  if (!blockHandlerIO7) {
    blockHandlerIO7 = true;
    gpioEventCounters[4]++;
    GpioEvent_t evt = {.pinIndex = 7, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 7) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO8Handler() {
  if (!blockHandlerIO8) {
    blockHandlerIO8 = true;
    gpioEventCounters[5]++;
    GpioEvent_t evt = {.pinIndex = 8, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 8) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO19Handler() {
  if (!blockHandlerIO19) {
    blockHandlerIO19 = true;
    gpioEventCounters[8]++;
    GpioEvent_t evt = {.pinIndex = 19, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 19) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

void IRAM_ATTR GpioLayrzHub::IO20Handler() {
  if (!blockHandlerIO20) {
    blockHandlerIO20 = true;
    gpioEventCounters[9]++;
    GpioEvent_t evt = {.pinIndex = 20, .triggerCamera = false};
    if (hubSettings.acam_en && hubSettings.acam_trig_src == 20) {
      evt.triggerCamera = true;
    }
    xQueueSendFromISR(gpioEventQueue, &evt, NULL);
  }
}

#endif
