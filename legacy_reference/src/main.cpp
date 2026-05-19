#include <modules/startup/StartupLayrzHub.h>

/**
 * The setup function is the main function that runs once in the program.
 * It performs various tasks based on certain conditions.
 *
 * @return void
 */
void setup() {
  StartupLayrzHub::InitUsbCommunications();
  StartupLayrzHub::initDeviceSettings();
  StartupLayrzHub::initRgbLed("S");
  StartupLayrzHub::checkFactoryResetOnBoot();
  const bool shouldCheckFirmwareEarly =
    hubSettings.net_en && (hubSettings.fota_en || hubSettings.fota_force);
  StartupLayrzHub::createBinarySemaphore();
  StartupLayrzHub::initSPI();
  StartupLayrzHub::initBlackBox();
  debugPrint("FIRMWARE BUILD: %d\n", BUILD_NUMBER);
  StartupLayrzHub::initRgbLed("S");

  // Run FOTA before enabling the watchdog and background tasks so the updater
  // is not competing with the rest of the startup workload.
  if (shouldCheckFirmwareEarly) {
    StartupLayrzHub::initA7670Module();
    StartupLayrzHub::initGnss();
    StartupLayrzHub::connectToNetwork(hubSettings.net_mode);
    StartupLayrzHub::checkForFirmwareUpdate();
  }

  StartupLayrzHub::initWatchdogTimer(120); // timeout in seconds
  StartupLayrzHub::startSystemHealthMonitor();
  StartupLayrzHub::InitNimBLE();
  StartupLayrzHub::startConfiotOverBLE();
  StartupLayrzHub::initArducamMega();

  if (!hubSettings.net_en) {
    debugPrint("Wifi/4G Network communication is disabled\n");
    if (hubSettings.zigbee_en && hubSettings.zigbee_local_en) {
      StartupLayrzHub::initZigbeeCoordinator();
    }
    return;
  }
  if (!shouldCheckFirmwareEarly) {
    StartupLayrzHub::initA7670Module();
    StartupLayrzHub::initGnss();
  }
  StartupLayrzHub::startTcpSocketListener();
  StartupLayrzHub::startNetworkKeepAlive();
  if (!StartupLayrzHub::initDataBuffers())
    return;
  StartupLayrzHub::initGpio();
  StartupLayrzHub::initBleSensors();
  StartupLayrzHub::initSensorsMessaging();
  // hubSettings.zigbee_hub_url = "8.tcp.ngrok.io";
  StartupLayrzHub::initZigbeeCoordinator();
  StartupLayrzHub::initSerialCommunications();
  StartupLayrzHub::initCanBus();
  StartupLayrzHub::startModbus();
  StartupLayrzHub::startSynchroNtpTask();
  StartupLayrzHub::startLayrzProtocol();
  StartupLayrzHub::startArducamCyclicCaptures();
  if (hubSettings.net_mode == "cellular") {
    debugPrint("Cellular connect now runs after sensor startup; offline "
               "backlog remains active\n");
  }
  if (!shouldCheckFirmwareEarly) {
    StartupLayrzHub::connectToNetwork(hubSettings.net_mode);
  }
}

time_t lastSocketAuthCheck = 0;
/**
 * The loop function is the main function that runs repeatedly in the program.
 * It performs various tasks based on certain conditions.
 *
 * @return void
 */
void loop() {
  esp_task_wdt_reset();
  vTaskDelay(15000 / portTICK_PERIOD_MS);
}
