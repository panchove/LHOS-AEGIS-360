#include <modules/global_objects/GlobalObjectsLayrzHub.h>

// Global variables initialization
settingsVars hubSettings;
char *msgBuffer = nullptr;
char *settingsBuffer = nullptr;
char *uartIoDataBuffer = nullptr;
char *rs485DataBuffer = nullptr;
char *rs232_1DataBuffer = nullptr;
char *rs232_2DataBuffer = nullptr;
char *extIoDataBuffer = nullptr;
char *canDataBuffer = nullptr;
char *obd2DtcBuffer = nullptr;
char *mediaMsgBuffer = nullptr;
char *modbusMsgBuffer = nullptr;
uint32_t supportedPids[150] = {0};
uint8_t supportedPidsCount = 0;
bool isObd2Initialized = false;
time_t timestamp;
volatile bool requestObd2Dtcs = false;
volatile bool bleConfigConnected = false;
volatile bool httpBusy = false;
volatile uint32_t httpBusySinceMs = 0;
volatile bool isFWChecked = false;
volatile bool isSocketAuth = false;
volatile bool isNtpSynced = false;
volatile bool isValidTime = false;
volatile bool netConnectFirstTime = true;
volatile bool socketSendSuccess =
  false; // Flag to track if the last socket send was successful
volatile bool bootAfterCommand = false;
volatile bool rebootAfterPcAck = false;
char rebootAfterPcCmdId[64] = {0};
volatile bool arducamInitialized = false;
volatile bool sdCardInitialized =
  false;                              // Flag to check if SD card is initialized
volatile bool gBleStackReady = false; // Flag to indicate if BLE stack is ready
long socketAuthTimeout = 60000;
#if defined(LAYRZ_HUB1_BUILD)
esp_adc_cal_characteristics_t *adc_chars_gpio1 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio2 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio5 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio6 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio7 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio14 = nullptr;
#elif defined(LAYRZ_HUB2_BUILD)
esp_adc_cal_characteristics_t *adc_chars_gpio3 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio4 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio5 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio6 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio7 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio19 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio20 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio41 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio42 = nullptr;
#elif defined(LAYRZ_HUB25_BUILD)
esp_adc_cal_characteristics_t *adc_chars_ai1 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_ai2 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio19 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_gpio20 = nullptr;
esp_adc_cal_characteristics_t *adc_chars_vbat = nullptr;
#endif

#if defined(LAYRZ_HUB1_BUILD)
TimerHandle_t debTimerIO1 = NULL;
TimerHandle_t debTimerIO2 = NULL;
TimerHandle_t debTimerIO5 = NULL;
TimerHandle_t debTimerIO6 = NULL;
TimerHandle_t debTimerIO7 = NULL;
TimerHandle_t debTimerIO14 = NULL;
TimerHandle_t debTimerIO45 = NULL;
TimerHandle_t debTimerIO46 = NULL;
TimerHandle_t debTimerIO47 = NULL;
#elif defined(LAYRZ_HUB2_BUILD)
TimerHandle_t debTimerIO3 = NULL;
TimerHandle_t debTimerIO4 = NULL;
TimerHandle_t debTimerIO5 = NULL;
TimerHandle_t debTimerIO6 = NULL;
TimerHandle_t debTimerIO19 = NULL;
TimerHandle_t debTimerIO20 = NULL;
TimerHandle_t debTimerIO41 = NULL;
TimerHandle_t debTimerIO42 = NULL;
#elif defined(LAYRZ_HUB25_BUILD)
TimerHandle_t debTimerIO4 = NULL;
TimerHandle_t debTimerIO5 = NULL;
TimerHandle_t debTimerIO6 = NULL;
TimerHandle_t debTimerIO7 = NULL;
TimerHandle_t debTimerIO8 = NULL;
TimerHandle_t debTimerIO19 = NULL;
TimerHandle_t debTimerIO20 = NULL;
#endif
TimerHandle_t bleConfigTimer = NULL;
#if defined(LAYRZ_HUB1_BUILD)
GPIOConfig gpioConfigs[10] = {{IO1, 0},  {IO2, 0},  {IO5, 0},
                              {IO6, 0},  {IO7, 0},  {IO14, 0},
                              {IO45, 0}, {IO46, 0}, {IO47, 0}};
uint32_t gpioEventCounters[10] = {0};
#elif defined(LAYRZ_HUB2_BUILD)
GPIOConfig gpioConfigs[10] = {{IO3, 0},  {IO4, 0},  {IO5, 0},  {IO6, 0},
                              {IO19, 0}, {IO20, 0}, {IO41, 0}, {IO42, 0}};
uint32_t gpioEventCounters[10] = {0};
#elif defined(LAYRZ_HUB25_BUILD)
GPIOConfig gpioConfigs[10] = {{DI1, 0},     {DI2, 0},     {DI3, 0}, {DI4, 0},
                              {AI1_DI5, 4}, {AI2_DI6, 4}, {DO1, 3}, {DO2, 3},
                              {IO19, 0},    {IO20, 0}};
uint32_t gpioEventCounters[10] = {0};
#endif

// LEDC channel allocation (6 channels)
ledc_channel_t pwmChannels[6] = {LEDC_CHANNEL_0, LEDC_CHANNEL_1,
                                 LEDC_CHANNEL_2, LEDC_CHANNEL_3,
                                 LEDC_CHANNEL_4, LEDC_CHANNEL_5};

blePair bleDevices[50] = {};

// Global xTaskHandle initialization
TaskHandle_t updateSensorsHandle = NULL;
TaskHandle_t sendSensorDataHandle = NULL;
TaskHandle_t pingTaskHandle = NULL;
TaskHandle_t checkWifiNetworkHandle = NULL;
TaskHandle_t checkGSMNetworkHandle = NULL;
TaskHandle_t getCommandsHandle = NULL;
TaskHandle_t bleScanHandle = NULL;
TaskHandle_t serialMonitorHandle = NULL;
TaskHandle_t checkFirmwareHandle = NULL;
TaskHandle_t confiotOverBleHandle = NULL;
TaskHandle_t synchroNTPHandle = NULL;
TaskHandle_t tcpSocketReceptionHandle = NULL;
TaskHandle_t j1939MonitorTaskHandle = NULL;
TaskHandle_t obd2MonitorTaskHandle = NULL;
TaskHandle_t arducamTaskHandle = NULL;
TaskHandle_t systemHealthMonitorHandle = NULL;

// Global xTask initialization
SemaphoreHandle_t xSemaphore = NULL;

// Serial objects initialization
HardwareSerial UART_COMM = HardwareSerial(1);
HardwareSerial ZigSerial = HardwareSerial(2);
SoftwareSerial UART_IO;
SoftwareSerial RS232_1;
SoftwareSerial RS232_2;
SoftwareSerial RS485;

// Network objects initialization
#ifdef USE_ASYNC_A7670
// Use AsyncA7670 library
AsyncA7670 modemAsync(UART_COMM);
AsyncA7670Client GSMClient(modemAsync, 0);     // TCP client using mux 0
AsyncA7670Client GSMClientSSL(modemAsync, 1);  // SSL base client using mux 1
AsyncA7670Client GSMClientSSL2(modemAsync, 2); // SSL base client using mux 2
AsyncA7670Client GSMFotaClient(modemAsync,
                               2); // FOTA non SSL client using mux 2
AsyncA7670Client GSMZigbeeClient(modemAsync, 3); // Zigbee client using mux 3
#else
// Use TinyGSM library (default)
TinyGsm modem(UART_COMM);
TinyGsmClient GSMClient(modem);
AsyncA7670 modemAsync(UART_COMM);
AsyncA7670Client GSMClientAsync(modemAsync);
#endif
WiFiClient wifiClient;

SSLClient GSMClient_SSL =
  SSLClient(&GSMClientSSL); // API SSL client using mux 1
SSLClient GSMClient_SSL2 =
  SSLClient(&GSMClientSSL2); // FOTA SSL client using mux 2
SSLClient WifiClient_SSL = SSLClient(&wifiClient);
SSLClient *tcpSslClient = nullptr;
// ESP_SSLClient *tcpSslClient = nullptr;
Client *tcpNonSslClient = nullptr;
Client *zigbeeTcpClient = nullptr;

// BLE objects initialization

// Configurator over BLE externs
NimBLEExtAdvertising *pAdvertising;      /** BLE Advertiser */
NimBLECharacteristic *pCharacteristicRX; /** Characteristic for Rx */
NimBLECharacteristic *pCharacteristicTX; /** Characteristic for Tx */
NimBLEServer *pServer;                   /** BLE Server */
NimBLEService *pService;                 /** BLE Service */
// Legacy Preferences object - DEPRECATED, use UnifiedSettingsStorage instead
// Preferences settings;                                       /** Preferences
// object for storing credentials */
NimBLEScan *pBLEScan;
// StaticJsonDocument<512> doc;
uint16_t connHandle;

// RGB Neopixel LED
Adafruit_NeoPixel rgbLed = Adafruit_NeoPixel(2, LED, NEO_GRB + NEO_KHZ800);
byte rgbBrightness = 20;

uint16_t mtuNegotiatedSize = 23; // Default MTU size for BLE connections

// bleDevicesMap removed - now handled by BleMemoryManager SPIRAM array
bool spiSharedInitialized = false;

#if defined(LAYRZ_HUB2_BUILD) || defined(LAYRZ_HUB25_BUILD)
BlackBoxLayrzHub::Config blackBoxConfig = {
  .segmentMaxBytes = 512 * 1024, // 512 KB
  .syncEveryN = 1,               // Flush every 1 appends/commits
  .logOnEnqueue = false,         // Write enqueued messages to history.txt
};
BlackBoxLayrzHub
  blackboxSDCARD(SD, blackBoxConfig); // BlackBox instance for SD card
#endif
SemaphoreHandle_t spiBusMutex = nullptr; // mutex for shared SPI bus
