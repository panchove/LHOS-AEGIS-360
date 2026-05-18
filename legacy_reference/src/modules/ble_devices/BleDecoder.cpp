#include <modules/ble_devices/BleDecoder.h>

std::string BleDecoder::decode(const char *blePacket) {
  std::vector<std::string> cmdArray = splitString(std::string(blePacket), ";");
  const char *macAddress = cmdArray.size() > 0 ? cmdArray[0].c_str() : "";
  const char *model = cmdArray.size() > 5 ? cmdArray[5].c_str() : "";
  const char *rssi = cmdArray.size() > 7 ? cmdArray[7].c_str() : "";
  const char *txPower = cmdArray.size() > 8 ? cmdArray[8].c_str() : "";
  const char *manufData = cmdArray.size() > 9 ? cmdArray[9].c_str() : "";
  const char *serviceData = cmdArray.size() > 10 ? cmdArray[10].c_str() : "";
  if (strcmp(model, "LAYRZ_BLEFIER_1") == 0) {
    return LAYRZ_BLEFIER_1_decoder::decode(macAddress, model, rssi, txPower,
                                           manufData, serviceData);
  } else if (strcmp(model, "LAYRZ_BLEFIER_2") == 0) {
    return LAYRZ_BLEFIER_2_decoder::decode(macAddress, model, rssi, txPower,
                                           manufData, serviceData);
  } else if (strcmp(model, "ELA_PUCK_RHT") == 0) {
    return ELA_PUCK_RHT_decoder::decode(macAddress, model, rssi, txPower,
                                        manufData, serviceData);
  } else if (strcmp(model, "ELA_PUCK_MOV") == 0) {
    return ELA_PUCK_MOV_decoder::decode(macAddress, model, rssi, txPower,
                                        manufData, serviceData);
  } else if (strcmp(model, "ELA_PUCK_DI") == 0) {
    return ELA_PUCK_DI_decoder::decode(macAddress, model, rssi, txPower,
                                       manufData, serviceData);
  } else if (strcmp(model, "ELA_PUCK_MAG") == 0) {
    return ELA_PUCK_MAG_decoder::decode(macAddress, model, rssi, txPower,
                                        manufData, serviceData);
  } else if (strcmp(model, "ELA_PUCK_PIR") == 0) {
    return ELA_PUCK_PIR_decoder::decode(macAddress, model, rssi, txPower,
                                        manufData, serviceData);
  } else if (strcmp(model, "ELA_PUCK_TPROBE") == 0) {
    return ELA_PUCK_TPROBE_decoder::decode(macAddress, model, rssi, txPower,
                                           manufData, serviceData);
  } else if (strcmp(model, "ELA_PUCK_ID") == 0) {
    return ELA_PUCK_ID_decoder::decode(macAddress, model, rssi, txPower,
                                       manufData, serviceData);
  } else if (strcmp(model, "ELA_COIN_MAG") == 0) {
    return ELA_COIN_MAG_decoder::decode(macAddress, model, rssi, txPower,
                                        manufData, serviceData);
  } else if (strcmp(model, "ELA_COIN_T") == 0) {
    return ELA_COIN_T_decoder::decode(macAddress, model, rssi, txPower,
                                      manufData, serviceData);
  } else if (strcmp(model, "ELA_LITE_TOUCH") == 0) {
    return ELA_LITE_TOUCH_decoder::decode(macAddress, model, rssi, txPower,
                                          manufData, serviceData);
  } else if (strcmp(model, "LYWSD03MMC") == 0) { // Xiaomi
    return LYWSD03MMC_decoder::decode(macAddress, model, rssi, txPower,
                                      manufData, serviceData);
  } else if (strcmp(model, "APPLE_IBEACON") == 0) {
    return APPLE_IBEACON_decoder::decode(macAddress, model, rssi, txPower,
                                         manufData, serviceData);
  } else if (strcmp(model, "ESCORT_DU") == 0) {
    return ESCORT_DU_decoder::decode(macAddress, model, rssi, txPower,
                                     manufData, serviceData);
  } else if (strcmp(model, "TELTONIKA_EYE") == 0) {
    return TELTONIKA_EYE_decoder::decode(macAddress, model, rssi, txPower,
                                         manufData, serviceData);
  } else if (strcmp(model, "GENERIC") == 0) {
    return GENERIC_decoder::decode(macAddress, model, rssi, txPower, manufData,
                                   serviceData);
  }
  // complete for all devices

  else
    return "";
}
