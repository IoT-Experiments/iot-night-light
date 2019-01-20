#include "device_ctrl.h"
#include <time.h>

DeviceCtrl::DeviceCtrl() {

}

DeviceCtrl::~DeviceCtrl() {

}

void DeviceCtrl::setMqttHost(const char* host) {
  strcpy(this->mqttHost, host);
}
void DeviceCtrl::setMqttPort(const long port) {
  this->mqttPort = port;
}
void DeviceCtrl::setRegistryId(const char* registryId) {
  strcpy(this->registryId, registryId);
}
void DeviceCtrl::setLocation(const char* location) {
  strcpy(this->location, location);
}
void DeviceCtrl::setDeviceId(const char* deviceId) {
  strcpy(this->deviceId, deviceId);
}
void DeviceCtrl::setProjectId(const char* projectId) {
  strcpy(this->projectId, projectId);
}
void DeviceCtrl::setPrivateKey(const char* privateKey) {
  strcpy(this->privateKey, privateKey);
  this->fillPrivKey();
}

String DeviceCtrl::getJwt() {
  if (this->iss == 0 || time(nullptr) - this->iss > TOKEN_LIFETIME) {  // TODO: exp in device
    // Disable software watchdog as these operations can take a while.
    Serial.println("Refreshing JWT");
    this->iss = time(nullptr);
    this->jwt = createJwt(this->projectId, this->iss, this->priv_key, TOKEN_LIFETIME);
  }
  return this->jwt;
}

bool DeviceCtrl::isTokenExpired() {
  return this->iss > 0 && time(nullptr) - this->iss > TOKEN_LIFETIME;
}

const char* DeviceCtrl::getMqttHost() {
  return this->mqttHost;
}
const long DeviceCtrl::getMqttPort() {
  return this->mqttPort;
}
const char* DeviceCtrl::getRegistryId() {
  return this->registryId;
}
const char* DeviceCtrl::getLocation() {
  return this->location;
}
const char* DeviceCtrl::getDeviceId() {
  return this->deviceId;
}
const char* DeviceCtrl::getProjectId() {
  return this->projectId;
}
const char* DeviceCtrl::getPrivateKey() {
  return this->privateKey;
}

// Fills the priv_key global variable with private key str which is of the form
// aa:bb:cc:dd:ee:...
void DeviceCtrl::fillPrivKey() {
  this->priv_key[8] = 0;
  char* privateKeyPtr = this->privateKey;
  for (int i = 7; i >= 0; i--) {
    this->priv_key[i] = 0;
    for (int byte_num = 0; byte_num < 4; byte_num++) {
      this->priv_key[i] = (this->priv_key[i] << 8) + strtoul(privateKeyPtr, NULL, 16);
      privateKeyPtr += 3;
    }
  }
}

String DeviceCtrl::getConfigTopic() {
  return String("/devices/") + this->deviceId + "/config";
}

String DeviceCtrl::getCommandsTopic() {
  return String("/devices/") + this->deviceId + "/commands/#";
}

String DeviceCtrl::getEventsTopic() {
  return String("/devices/") + this->deviceId + "/events";
}

String DeviceCtrl::getStateTopic() {
  return String("/devices/") + this->deviceId + "/state";
}

String DeviceCtrl::getClientId() {
  return String("projects/") + this->projectId + "/locations/" + this->location +
         "/registries/" + this->registryId + "/devices/" + this->deviceId;
}
