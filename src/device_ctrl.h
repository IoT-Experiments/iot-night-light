#ifndef _DEVICE_CTRL_H_
#define _DEVICE_CTRL_H_

#include <Arduino.h>
#include <jwt_helper.h>

#ifndef TOKEN_LIFETIME
#define TOKEN_LIFETIME  3600
#endif

class DeviceCtrl {
  public:
    DeviceCtrl();
    ~DeviceCtrl();
    void setMqttHost(const char* host);
    void setMqttPort(const long port);
    void setRegistryId(const char* registryId);
    void setLocation(const char* location);
    void setDeviceId(const char* deviceId);
    void setProjectId(const char* projectId);
    void setPrivateKey(const char* privateKey);
    bool isTokenExpired();

    const char* getMqttHost();
    const long getMqttPort();
    const char* getRegistryId();
    const char* getLocation();
    const char* getDeviceId();
    const char* getProjectId();
    const char* getPrivateKey();

    String getConfigTopic();
    String getCommandsTopic();
    String getEventsTopic();
    String getStateTopic();
    String getClientId();

    String getJwt();
  protected:

  private:
    void fillPrivKey();
    NN_DIGIT priv_key[9];
    String jwt;

    char mqttHost[40];
    long mqttPort;
    char registryId[40];
    char location[40];
    char deviceId[40];
    char projectId[40];
    char privateKey[100];

    unsigned long iss = 0;
};

#endif
