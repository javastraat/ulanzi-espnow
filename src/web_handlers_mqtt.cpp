#include <Arduino.h>
// web_handlers_mqtt.cpp — MQTT page and API handlers.
#include "web_handlers_mqtt.h"
#include "globals.h"
#include "mqtt.h"
#include "nvs_settings.h"
#include "web/mqtt.h"

void registerMqttHandlers() {

  webServer.on("/mqtt", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_MQTT);
  });

  webServer.on("/api/hass/log", HTTP_GET, []() {
    char buf[1024]; int n = 0;
    n += snprintf(buf + n, sizeof(buf) - n, "{\"count\":%lu,\"log\":[", (unsigned long)wsCountMqtt);
    for (int i = 0; i < (int)wsMqttFill; i++) {
      int idx = ((int)wsMqttHead - 1 - i + POCSAG_LOG_SIZE) % POCSAG_LOG_SIZE;
      char safe[POCSAG_MSG_MAX_LEN * 2 + 1]; int si = 0;
      for (int j = 0; wsMqttLog[idx].msg[j] && si < (int)sizeof(safe) - 2; j++) {
        char c = wsMqttLog[idx].msg[j];
        if (c == '"' || c == '\\') safe[si++] = '\\';
        safe[si++] = c;
      }
      safe[si] = '\0';
      if (i > 0) n += snprintf(buf + n, sizeof(buf) - n, ",");
      n += snprintf(buf + n, sizeof(buf) - n, "{\"msg\":\"%s\",\"ts\":\"%s\"}", safe, wsMqttLog[idx].ts);
    }
    snprintf(buf + n, sizeof(buf) - n, "]}");
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/mqtt", HTTP_GET, []() {
    char buf[512];
    snprintf(buf, sizeof(buf),
      "{\"enabled\":%s,\"connected\":%s,\"status\":\"%s\","
      "\"broker\":\"%s\",\"port\":%u,"
      "\"user\":\"%s\",\"pass\":\"%s\","
      "\"node_id\":\"%s\",\"prefix\":\"%s\","
      "\"discovery\":%s,\"ha_name\":\"%s\"}",
      mqttEnabled   ? "true" : "false",
      mqttIsConnected() ? "true" : "false",
      mqttGetStatus(),
      mqttBroker, mqttPort,
      mqttUser,
      mqttPass,
      mqttNodeId, mqttPrefix,
      mqttDiscovery ? "true" : "false",
      mqttHaName);
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/mqtt", HTTP_POST, []() {
    if (webServer.hasArg("enabled"))   mqttEnabled   = webServer.arg("enabled")  == "1";
    if (webServer.hasArg("discovery")) mqttDiscovery = webServer.arg("discovery") == "1";
    if (webServer.hasArg("port"))      mqttPort      = (uint16_t)webServer.arg("port").toInt();
    if (webServer.hasArg("broker")) {
      strncpy(mqttBroker, webServer.arg("broker").c_str(), 63); mqttBroker[63] = '\0';
    }
    if (webServer.hasArg("user")) {
      strncpy(mqttUser, webServer.arg("user").c_str(), 31); mqttUser[31] = '\0';
    }
    if (webServer.hasArg("pass")) {
      strncpy(mqttPass, webServer.arg("pass").c_str(), 63); mqttPass[63] = '\0';
    }
    if (webServer.hasArg("node")) {
      String v = webServer.arg("node"); v.trim();
      if (v.length() > 0) { strncpy(mqttNodeId, v.c_str(), 31); mqttNodeId[31] = '\0'; }
    }
    if (webServer.hasArg("prefix")) {
      strncpy(mqttPrefix, webServer.arg("prefix").c_str(), 31); mqttPrefix[31] = '\0';
    }
    if (webServer.hasArg("ha_name")) {
      String v = webServer.arg("ha_name"); v.trim();
      strncpy(mqttHaName, v.c_str(), 31); mqttHaName[31] = '\0';
    }
    saveSettings();
    mqttRequestReconnect();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/mqtt/discovery", HTTP_POST, []() {
    if (!mqttIsConnected()) {
      webServer.send(200, "application/json", "{\"ok\":false,\"msg\":\"not connected\"}");
      return;
    }
    mqttRequestReconnect();  // reconnect will re-publish discovery
    webServer.send(200, "application/json", "{\"ok\":true}");
  });
}
