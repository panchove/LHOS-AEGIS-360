#ifndef ASYNC_A7670_H
#define ASYNC_A7670_H

#include <Arduino.h>
#include <Stream.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>

// Forward declaration
class AsyncA7670Client;

// Configuration constants
#define A7670_MAX_SEND_CHUNK 1500
#define A7670_RESPONSE_TIMEOUT_MS 10000
#define A7670_CONNECT_TIMEOUT_MS 30000
#define A7670_TASK_STACK_SIZE 6144
#define A7670_TASK_PRIORITY 10
#define A7670_QUEUE_SIZE 32
#define A7670_RX_BUFFER_SIZE                                                   \
  8192 // Increased from 2048 to 8KB for large payloads

// Command types for internal queue
enum class A7670CommandType {
  AT_COMMAND,
  GPRS_CONNECT,
  GPRS_DISCONNECT,
  TCP_CONNECT,
  TCP_DISCONNECT,
  TCP_SEND,
  NTP_SYNC,
  GET_STATUS
};

// Status enums
enum class GPRSStatus { DISCONNECTED, CONNECTING, CONNECTED, ERROR };

enum class TCPStatus { CLOSED, CONNECTING, CONNECTED, ERROR };

enum class NTPStatus { NOT_SYNCED, SYNCING, SYNCED, ERROR };

// Internal command structure
struct A7670Command {
  A7670CommandType type;
  char data[256];
  uint8_t mux;   // For TCP operations
  size_t length; // For send operations
  uint32_t timeout;
  bool priority; // High priority commands go first
};

class AsyncA7670 {
public:
  explicit AsyncA7670(Stream &stream);
  ~AsyncA7670();

  // Status variables
  bool initialized;
  bool modemReady;
  GPRSStatus gprsStatus;
  TCPStatus tcpStatus[10]; // Support up to 10 sockets
  NTPStatus ntpStatus;

  // Network registration status
  bool
    netRegStatus; // Network registration status (updated by AT+CGREG response)
  bool modemRestarted; // Modem restart completion status (updated by restart
                       // process)

  // Initialization
  bool begin(uint32_t baudRate = 115200);
  bool registerNetwork();
  void end();

  // Debug output
  void setDebugStream(Stream *debugStream) { _debugStream = debugStream; }

  // GPRS functions (TinyGSM compatible naming)
  bool gprsConnect(const char *apn, const char *user = nullptr,
                   const char *password = nullptr);
  bool gprsDisconnect();
  void checkGprsStatus(); // New function to check and update GPRS status
  bool isGprsConnected();
  bool isGprsConnecting() {
    if (xSemaphoreTake(_statusMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
      return false;
    }
    bool connecting = (gprsStatus == GPRSStatus::CONNECTING);
    xSemaphoreGive(_statusMutex);
    return connecting;
  }
  void checkTcpStatus();
  String getLocalIP();
  GPRSStatus getGprsStatus() { return gprsStatus; }

  // Network registration functions
  void checkNetworkRegistration(); // Check network registration status with
                                   // AT+CGREG?
  bool isNetworkConnected() {
    return netRegStatus;
  } // Use actual registration status

  // TCP functions (TinyGSM compatible naming)
  void ensureTcpCollectionMutex(); // Ensure the mutex exists before any network
                                   // operation
  bool modemConnect(const char *host, uint16_t port, uint8_t mux = 0,
                    bool ssl = false, int timeout_s = 30);
  bool modemDisconnect(uint8_t mux = 0);
  bool modemGetConnected(uint8_t mux = 0);
  size_t modemSend(const void *buff, size_t len, uint8_t mux = 0);
  size_t modemRead(size_t size, uint8_t mux = 0);
  size_t modemReadBytes(uint8_t *buffer, size_t size, uint8_t mux = 0);
  size_t modemGetAvailable(uint8_t mux = 0);

  // Client management (TinyGSM compatible)
  void maintainClient(uint8_t mux = 0);
  AsyncA7670Client *clients[10]; // Array of client pointers

  // NTP functions
  bool
  ntpServerSync(const char *server = "pool.ntp.org",
                int timeZone = 0); // timeZone in hours (e.g., -5 for GMT-5)
  bool isNtpSynced();
  bool getNetworkTime(int *year, int *month, int *day, int *hour, int *minute,
                      int *second, float *timezone);
  NTPStatus getNtpStatus() { return ntpStatus; }

  // Status and diagnostics
  String getModemInfo();
  int getSignalQuality();
  int getSignalStrength(); // Returns signal strength in dBm
  bool isModemAlive();

  // TinyGSM Compatibility Layer
  bool restart(const char *pin = nullptr);
  bool waitForNetwork(uint32_t timeout_ms = 60000);
  String getModemName() { return getModemInfo(); }
  bool begin(const char *pin);

  // GPS functions and variables
  uint8_t gnssMode = 3;
  bool gnssEnabled = false;
  bool gnssDataReceived = false;

  struct gnssData {
    String rawData = "";
    float latitude = 0.00000;
    float longitude = 0.00000;
    float altitude = 0.0;
    float speed = 0.0;
    float course = 0.0;
    uint8_t fixQuality = 0;
    uint8_t gpsSatellitesVisible = 0;
    uint8_t beidouSatellitesVisible = 0;
    uint8_t glonassSatellitesVisible = 0;
    uint8_t galileoSatellitesVisible = 0;
    String date = "";
    String utcTime = "";
    uint32_t timestamp = 0; // UNIX timestamp in seconds
    float hdop = 0.0;
    float pdop = 0.0;
    float vdop = 0.0;
    uint8_t satellitesUsed = 0;
  } gnssData;
  bool enableGnss();
  bool setGnssMode(int mode);
  bool getGnssData();
  bool isGnssEnabled() { return gnssEnabled; }
  bool isGnssFixed() { return gnssData.fixQuality > 0; }
  bool isGnssDataReceived() { return gnssDataReceived; }

  // TCP connection info structure
  struct TCPConnection {
    String host;
    uint16_t port;
    bool connected = false;
    bool ssl = false;
    uint8_t retryCount;
    uint32_t lastRetryTime;
    bool got_data;
    size_t expectedDataLength;
    uint32_t lastActivityTime = 0;
    size_t modemArrivedBytes =
      0; // bytes that have arrived and are buffered in the modem
    bool modemAvailableRequested =
      false; // Flag to indicate if data request has been made to modem
    size_t modemAvailable =
      0; // new: bytes available in modem buffer requested with CIPRXGET=4
    bool incomingData =
      false; // Flag to indicate if data is currently being received
    size_t incomingDataLength = 0; // Length of incoming data being received

  } tcpConnections[10];

private:
  // FreeRTOS task functions
  static void modemTaskWrapper(void *parameter);
  static void responseTaskWrapper(void *parameter);
  void modemTask();
  void responseTask();

  // Internal command processing
  bool sendCommand(A7670CommandType type, const char *data = nullptr,
                   uint8_t mux = 0, size_t length = 0,
                   uint32_t timeout = A7670_RESPONSE_TIMEOUT_MS,
                   bool priority = false);
  bool sendATCommand(const char *command,
                     uint32_t timeout = A7670_RESPONSE_TIMEOUT_MS,
                     bool priority = false);
  bool waitForResponse(const char *expectedResponse, uint32_t timeout);

  // GPRS internal functions
  void processGprsConnect();
  void processGprsDisconnect();

  // TCP internal functions
  void processTcpConnect(uint8_t mux);
  void processTcpDisconnect(uint8_t mux);
  void processTcpSend(uint8_t mux);

  // NTP internal functions
  void processNtpSync();
  void checkNtpStatus();

  // Utility functions
  void processIncomingData();
  bool parseReceiveHeader(const String &line, int &mux, int &length);
  void parseResponse(const char *response);
  void collectTcpData(uint8_t mux, size_t expectedLength,
                      uint32_t timeoutMs = 5000);
  void debugPrint(const char *message);
  void debugPrintln(const char *message);

  // Member variables
  Stream &_stream;
  Stream *_debugStream;

  // FreeRTOS objects
  TaskHandle_t _modemTaskHandle;
  TaskHandle_t _responseTaskHandle;
  TaskHandle_t
    _maintainFifoTaskHandle; // New task handle for maintainClient FIFO
  QueueHandle_t _commandQueue;
  SemaphoreHandle_t _responseMutex;
  SemaphoreHandle_t _statusMutex;
  SemaphoreHandle_t _connectionMutex;
  SemaphoreHandle_t _tcpCollectionMutex; // Mutex to control TCP data collection

  // TCP data collection state
  bool _isTcpDataCollection; // Flag indicating TCP data collection is active
                             // True while we are consuming a binary block; used
                             // to suppress proactive CIPRXGET=4
  volatile bool _drainingPayload = false;
  // True while waiting for the CIPSEND prompt ('>').
  volatile bool _awaitingSendPrompt = false;
  // Set when '>' prompt is detected by the response task.
  volatile bool _sendPromptReceived = false;

  // Status variables
  bool _sendConfirmReceived; // Flag to track +CIPSEND: confirmation
  int _lastSendConfirmed;    // Number of bytes confirmed in last +CIPSEND:

  // Connection info
  String _apn;
  String _apnUser;
  String _apnPassword;
  String _localIP;
  String _resolvedHostIp;

  // Response handling
  char _responseBuffer[4096];
  size_t _responseIndex;
  bool _waitingForResponse;
  String _expectedResponse;
  uint32_t _responseTimeout;
  uint32_t _responseStartTime;

  // GNSS request pacing
  volatile bool _gnssInfoPending = false;
  volatile uint32_t _gnssInfoSentMs = 0;

  // NTP info
  String _ntpServer;
  int _timeZone;
  String _dateTime;

  // Signal quality tracking
  int _signalQuality;  // Signal quality percentage (0-100)
  int _signalStrength; // Signal strength in dBm

  // Configuration
  uint32_t _baudRate;
};

#endif // ASYNC_A7670_H
