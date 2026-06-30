/*
 * ═══════════════════════════════════════════════════════════════
 * MQTT INTEGRATION per OraQuadra2
 * ═══════════════════════════════════════════════════════════════
 * 
 * Questo file aggiunge supporto MQTT completo a OraQuadra2 mantenendo
 * TUTTE le funzionalità originali (interfaccia web, Alexa, pulsanti).
 * 
 * Include TUTTI i sensori della versione ESPHome precedente + nuovi.
 * 
 * Author: Claude AI Assistant
 * Version: 1.0
 * Date: 2024
 */

#ifndef MQTT_INTEGRATION_H
#define MQTT_INTEGRATION_H

#include <PubSubClient.h>
#include <ArduinoJson.h>

// ═══════════════════════════════════════════════════════════════
// CONFIGURAZIONE MQTT
// ═══════════════════════════════════════════════════════════════

#define MQTT_BASE_TOPIC "oraquadra2"
#define MQTT_DISCOVERY_PREFIX "homeassistant"  // Per auto-discovery HA

// Intervalli di pubblicazione (millisecondi)
#define SENSOR_PUBLISH_INTERVAL 30000    // 30 secondi
#define FAST_SENSOR_INTERVAL 5000        // 5 secondi (per LED, luminosità)
#define DIAGNOSTIC_INTERVAL 60000        // 60 secondi (per WiFi, CPU, memoria)

// ═══════════════════════════════════════════════════════════════
// CLASSE MQTT INTEGRATION
// ═══════════════════════════════════════════════════════════════

class MqttIntegration {
private:
    PubSubClient* client;
    
    // Timer per pubblicazione sensori
    unsigned long lastSensorPublish = 0;
    unsigned long lastFastSensorPublish = 0;
    unsigned long lastDiagnosticPublish = 0;
    
    // Flag per Home Assistant Discovery
    bool discoveryPublished = false;
    
    // ═══════════════════════════════════════════════════════════
    // TOPIC HELPER FUNCTIONS
    // ═══════════════════════════════════════════════════════════
    
    String getStateTopic(const char* sensor) {
        return String(MQTT_BASE_TOPIC) + "/state/" + String(sensor);
    }
    
    String getCommandTopic(const char* command) {
        return String(MQTT_BASE_TOPIC) + "/cmd/" + String(command);
    }
    
    String getAvailabilityTopic() {
        return String(MQTT_BASE_TOPIC) + "/status";
    }
    
    // ═══════════════════════════════════════════════════════════
    // PUBLISH HELPERS
    // ═══════════════════════════════════════════════════════════
    
    void publishState(const char* sensor, String value, bool retained = true) {
        String topic = getStateTopic(sensor);
        client->publish(topic.c_str(), value.c_str(), retained);
    }
    
    void publishState(const char* sensor, float value, bool retained = true) {
        publishState(sensor, String(value, 2), retained);
    }
    
    void publishState(const char* sensor, int value, bool retained = true) {
        publishState(sensor, String(value), retained);
    }
    
    void publishState(const char* sensor, bool value, bool retained = true) {
        publishState(sensor, value ? "ON" : "OFF", retained);
    }

public:
    
    // ═══════════════════════════════════════════════════════════
    // INIZIALIZZAZIONE
    // ═══════════════════════════════════════════════════════════
    
    void init(PubSubClient* mqttClient) {
        client = mqttClient;
        client->setCallback([this](char* topic, byte* payload, unsigned int length) {
            this->mqttCallback(topic, payload, length);
        });
    }
    
    void setup() {
        // Sottoscrivi a tutti i comandi
        String cmdTopic = String(MQTT_BASE_TOPIC) + "/cmd/#";
        client->subscribe(cmdTopic.c_str());
        
        // Pubblica stato online
        publishAvailability(true);
        
        // Pubblica discovery config per Home Assistant
        if (!discoveryPublished) {
            publishHomeAssistantDiscovery();
            discoveryPublished = true;
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // LOOP PRINCIPALE - Chiama questo nel loop()
    // ═══════════════════════════════════════════════════════════
    
    void loop() {
        unsigned long now = millis();
        
        // Pubblica sensori veloci (LED, luminosità, stato orologio)
        if (now - lastFastSensorPublish >= FAST_SENSOR_INTERVAL) {
            publishFastSensors();
            lastFastSensorPublish = now;
        }
        
        // Pubblica sensori normali (preset, pulsanti, temperatura)
        if (now - lastSensorPublish >= SENSOR_PUBLISH_INTERVAL) {
            publishNormalSensors();
            lastSensorPublish = now;
        }
        
        // Pubblica diagnostica (WiFi, CPU, memoria, uptime)
        if (now - lastDiagnosticPublish >= DIAGNOSTIC_INTERVAL) {
            publishDiagnosticSensors();
            lastDiagnosticPublish = now;
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // PUBBLICAZIONE SENSORI VELOCI
    // ═══════════════════════════════════════════════════════════
    
    void publishFastSensors() {
        // Questi sensori cambiano frequentemente
        
        // LED Attivi - numero di LED accesi
        publishState("led/active", getLedActiveCount());
        
        // Luminosità attuale (0-100%)
        publishState("brightness", getBrightness());
        
        // Orologio attivo/spento
        publishState("clock/active", isClockActive());
        
        // Stato lampeggio secondi
        publishState("seconds/flashing", isSecondsFlashing());
    }
    
    // ═══════════════════════════════════════════════════════════
    // PUBBLICAZIONE SENSORI NORMALI
    // ═══════════════════════════════════════════════════════════
    
    void publishNormalSensors() {
        // Mode Button
        publishState("button/mode", getModeButtonState());
        
        // Seconds Button
        publishState("button/seconds", getSecondsButtonState());
        
        // Preset corrente (numero)
        publishState("preset/current", getCurrentPreset());
        
        // Preset nome
        publishState("preset/name", getPresetName());
        
        // Temperatura ESP32
        publishState("temperature", getTemperature());
        
        // Colore RGB corrente (JSON)
        publishRGBColor();
        
        // Modalità corrente (normale/arcobaleno/fade/etc)
        publishState("mode/current", getCurrentMode());
        
        // Velocità animazione
        publishState("animation/speed", getAnimationSpeed());
    }
    
    // ═══════════════════════════════════════════════════════════
    // PUBBLICAZIONE SENSORI DIAGNOSTICI
    // ═══════════════════════════════════════════════════════════
    
    void publishDiagnosticSensors() {
        // WiFi
        publishState("wifi/connected", WiFi.status() == WL_CONNECTED);
        publishState("wifi/rssi", WiFi.RSSI());
        publishState("wifi/ssid", WiFi.SSID());
        publishState("wifi/ip", WiFi.localIP().toString());
        
        // Memoria
        publishState("memory/free", ESP.getFreeHeap());
        publishState("memory/total", ESP.getHeapSize());
        publishState("memory/usage_percent", getMemoryUsagePercent());
        
        // CPU
        publishState("cpu/usage", getCpuUsage());
        publishState("cpu/frequency", ESP.getCpuFreqMHz());
        
        // Sistema
        publishState("system/uptime", millis() / 1000);
        publishState("system/firmware", getFirmwareVersion());
        publishState("system/chip_model", ESP.getChipModel());
        
        // Stato connessione (valore numerico dalla tua immagine)
        publishState("connection/state", getConnectionState());
    }
    
    // ═══════════════════════════════════════════════════════════
    // PUBBLICAZIONE COLORE RGB (JSON)
    // ═══════════════════════════════════════════════════════════
    
    void publishRGBColor() {
        StaticJsonDocument<128> doc;
        RGB color = getCurrentRGBColor();
        
        doc["r"] = color.r;
        doc["g"] = color.g;
        doc["b"] = color.b;
        
        String output;
        serializeJson(doc, output);
        publishState("color/rgb", output);
    }
    
    // ═══════════════════════════════════════════════════════════
    // GESTIONE COMANDI MQTT DA HOME ASSISTANT
    // ═══════════════════════════════════════════════════════════
    
    void mqttCallback(char* topic, byte* payload, unsigned int length) {
        // Converti payload in String
        String payloadStr = "";
        for (unsigned int i = 0; i < length; i++) {
            payloadStr += (char)payload[i];
        }
        
        String topicStr = String(topic);
        
        Serial.print("MQTT ricevuto - Topic: ");
        Serial.print(topicStr);
        Serial.print(" | Payload: ");
        Serial.println(payloadStr);
        
        // ═══════════════════════════════════════════════════════
        // COMANDI POWER
        // ═══════════════════════════════════════════════════════
        
        if (topicStr.endsWith("/cmd/power")) {
            if (payloadStr == "ON") {
                setClockPower(true);
                publishState("clock/active", true);
            } else if (payloadStr == "OFF") {
                setClockPower(false);
                publishState("clock/active", false);
            }
        }
        
        // ═══════════════════════════════════════════════════════
        // COMANDI LUMINOSITÀ
        // ═══════════════════════════════════════════════════════
        
        else if (topicStr.endsWith("/cmd/brightness")) {
            int brightness = payloadStr.toInt();
            if (brightness >= 0 && brightness <= 100) {
                setBrightness(brightness);
                publishState("brightness", brightness);
            }
        }
        
        // ═══════════════════════════════════════════════════════
        // COMANDI PRESET
        // ═══════════════════════════════════════════════════════
        
        else if (topicStr.endsWith("/cmd/preset")) {
            int preset = payloadStr.toInt();
            setPreset(preset);
            publishState("preset/current", preset);
            publishState("preset/name", getPresetName());
        }
        
        // ═══════════════════════════════════════════════════════
        // COMANDI COLORE RGB
        // ═══════════════════════════════════════════════════════
        
        else if (topicStr.endsWith("/cmd/color")) {
            // Aspetta JSON: {"r":255,"g":0,"b":0}
            StaticJsonDocument<128> doc;
            DeserializationError error = deserializeJson(doc, payloadStr);
            
            if (!error) {
                RGB color;
                color.r = doc["r"];
                color.g = doc["g"];
                color.b = doc["b"];
                setRGBColor(color);
                publishRGBColor();
            }
        }
        
        // ═══════════════════════════════════════════════════════
        // COMANDI MODALITÀ
        // ═══════════════════════════════════════════════════════
        
        else if (topicStr.endsWith("/cmd/mode")) {
            setMode(payloadStr);
            publishState("mode/current", payloadStr);
        }
        
        // ═══════════════════════════════════════════════════════
        // COMANDI ANIMAZIONE
        // ═══════════════════════════════════════════════════════
        
        else if (topicStr.endsWith("/cmd/animation/speed")) {
            int speed = payloadStr.toInt();
            if (speed >= 1 && speed <= 10) {
                setAnimationSpeed(speed);
                publishState("animation/speed", speed);
            }
        }
        
        // ═══════════════════════════════════════════════════════
        // COMANDI EFFETTI
        // ═══════════════════════════════════════════════════════
        
        else if (topicStr.endsWith("/cmd/effect")) {
            setEffect(payloadStr);
            publishState("effect/current", payloadStr);
        }
        
        // ═══════════════════════════════════════════════════════
        // RESTART
        // ═══════════════════════════════════════════════════════
        
        else if (topicStr.endsWith("/cmd/restart")) {
            if (payloadStr == "RESTART") {
                publishAvailability(false);
                delay(100);
                ESP.restart();
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // AVAILABILITY STATUS
    // ═══════════════════════════════════════════════════════════
    
    void publishAvailability(bool online) {
        String topic = getAvailabilityTopic();
        client->publish(topic.c_str(), online ? "online" : "offline", true);
    }
    
    // ═══════════════════════════════════════════════════════════
    // HOME ASSISTANT AUTO-DISCOVERY
    // ═══════════════════════════════════════════════════════════
    
    void publishHomeAssistantDiscovery() {
        // Pubblica configurazioni per auto-discovery in Home Assistant
        // Questo rende i sensori disponibili automaticamente in HA
        
        String deviceInfo = getDeviceInfoJson();
        
        // Light entity (controllo principale)
        publishDiscoveryConfig("light", "oraquadra_light", 
            "OraQuadra Light", deviceInfo, "light");
        
        // Sensori
        publishDiscoveryConfig("sensor", "led_active", 
            "LED Attivi", deviceInfo, "sensor", "LED");
        
        publishDiscoveryConfig("sensor", "brightness", 
            "Luminosità", deviceInfo, "sensor", "%");
        
        publishDiscoveryConfig("sensor", "temperature", 
            "Temperatura", deviceInfo, "sensor", "°C", "temperature");
        
        publishDiscoveryConfig("sensor", "wifi_rssi", 
            "WiFi Signal", deviceInfo, "sensor", "dBm", "signal_strength");
        
        publishDiscoveryConfig("sensor", "memory_free", 
            "Memoria Libera", deviceInfo, "sensor", "bytes");
        
        publishDiscoveryConfig("sensor", "cpu_usage", 
            "CPU Usage", deviceInfo, "sensor", "%");
        
        publishDiscoveryConfig("sensor", "uptime", 
            "Uptime", deviceInfo, "sensor", "s", "duration");
        
        // Binary sensors
        publishDiscoveryConfig("binary_sensor", "wifi_connected", 
            "WiFi Connesso", deviceInfo, "binary_sensor", "", "connectivity");
        
        publishDiscoveryConfig("binary_sensor", "clock_active", 
            "Orologio Attivo", deviceInfo, "binary_sensor");
        
        // Numero di preset
        publishDiscoveryConfig("number", "preset", 
            "Preset", deviceInfo, "number");
        
        Serial.println("Home Assistant Discovery pubblicata!");
    }
    
private:
    
    // ═══════════════════════════════════════════════════════════
    // HELPER PER DISCOVERY CONFIG
    // ═══════════════════════════════════════════════════════════
    
    void publishDiscoveryConfig(const char* component, const char* objectId,
                                const char* name, String deviceInfo,
                                const char* iconType = "",
                                const char* unit = "",
                                const char* deviceClass = "") {
        
        String discoveryTopic = String(MQTT_DISCOVERY_PREFIX) + "/" + 
                               String(component) + "/" + 
                               String(MQTT_BASE_TOPIC) + "/" + 
                               String(objectId) + "/config";
        
        StaticJsonDocument<512> doc;
        
        doc["name"] = name;
        doc["unique_id"] = String(MQTT_BASE_TOPIC) + "_" + String(objectId);
        doc["state_topic"] = getStateTopic(objectId);
        
        if (String(component) == "light") {
            doc["command_topic"] = getCommandTopic("power");
            doc["brightness_state_topic"] = getStateTopic("brightness");
            doc["brightness_command_topic"] = getCommandTopic("brightness");
            doc["brightness_scale"] = 100;
            doc["payload_on"] = "ON";
            doc["payload_off"] = "OFF";
        }
        
        if (strlen(unit) > 0) {
            doc["unit_of_measurement"] = unit;
        }
        
        if (strlen(deviceClass) > 0) {
            doc["device_class"] = deviceClass;
        }
        
        doc["availability_topic"] = getAvailabilityTopic();
        doc["payload_available"] = "online";
        doc["payload_not_available"] = "offline";
        
        // Device info
        JsonObject device = doc.createNestedObject("device");
        device["identifiers"][0] = MQTT_BASE_TOPIC;
        device["name"] = "OraQuadra2";
        device["model"] = "Word Clock ESP32";
        device["manufacturer"] = "SurvivalHacking";
        
        String output;
        serializeJson(doc, output);
        
        client->publish(discoveryTopic.c_str(), output.c_str(), true);
    }
    
    String getDeviceInfoJson() {
        return "{}";  // Placeholder
    }
    
    // ═══════════════════════════════════════════════════════════
    // FUNZIONI GETTER - DA IMPLEMENTARE NEL FILE PRINCIPALE
    // ═══════════════════════════════════════════════════════════
    // Queste funzioni devono essere implementate nel tuo 
    // OraQuadra2_arduino.ino per leggere i valori reali
    
    int getLedActiveCount();        // Numero LED accesi
    int getBrightness();            // Luminosità 0-100
    bool isClockActive();           // Orologio ON/OFF
    bool isSecondsFlashing();       // Secondi lampeggianti
    String getModeButtonState();    // Stato pulsante mode
    String getSecondsButtonState(); // Stato pulsante secondi
    int getCurrentPreset();         // Numero preset corrente
    String getPresetName();         // Nome preset
    float getTemperature();         // Temperatura CPU
    RGB getCurrentRGBColor();       // Colore RGB corrente
    String getCurrentMode();        // Modalità corrente
    int getAnimationSpeed();        // Velocità animazione
    float getCpuUsage();            // Uso CPU
    float getMemoryUsagePercent();  // % memoria usata
    String getFirmwareVersion();    // Versione firmware
    int getConnectionState();       // Stato connessione
    
    // ═══════════════════════════════════════════════════════════
    // FUNZIONI SETTER - DA IMPLEMENTARE NEL FILE PRINCIPALE
    // ═══════════════════════════════════════════════════════════
    
    void setClockPower(bool state);      // Accendi/spegni
    void setBrightness(int value);       // Imposta luminosità
    void setPreset(int preset);          // Cambia preset
    void setRGBColor(RGB color);         // Imposta colore
    void setMode(String mode);           // Cambia modalità
    void setAnimationSpeed(int speed);   // Velocità animazione
    void setEffect(String effect);       // Imposta effetto
};

// ═══════════════════════════════════════════════════════════════
// STRUCT RGB
// ═══════════════════════════════════════════════════════════════

struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

#endif // MQTT_INTEGRATION_H
