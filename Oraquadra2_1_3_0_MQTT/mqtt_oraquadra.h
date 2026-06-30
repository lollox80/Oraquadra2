// ============================================================================
//  mqtt_oraquadra.h  -  Integrazione MQTT / Home Assistant per OraQuadra2 V1.3.0
//  Portata da Lollo (base V1.2.6) sulla V1.3.0 ufficiale di SurvivalHacking.
//
//  Contiene: configurazione MQTT runtime (salvata in EEPROM, impostabile da web),
//  callback comandi JSON (Home Assistant), auto-discovery, pubblicazione stato,
//  riconnessione automatica, e le feature "modalita random" e "modalita test".
//
//  NOTA: questo file va incluso DOPO le dichiarazioni globali del .ino
//  (e' incluso appena prima di setup() ). Le variabili random/test sono
//  dichiarate nel .ino (servono anche al display).
// ============================================================================
#pragma once

#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient

#ifndef ENABLE_MQTT
#define ENABLE_MQTT true
#endif

// ---- Valori di default (usati al primo avvio / reset) ----
#define MQTT_SERVER_DEFAULT     "192.168.1.100"
#define MQTT_PORT_DEFAULT       1883
#define MQTT_USER_DEFAULT       "mqtt"
#define MQTT_PASSWORD_DEFAULT   "CHANGE_ME"
#define MQTT_BASE_TOPIC_DEFAULT "oraquadra2"
#define MQTT_DEVICE_NAME_DEFAULT "OraQuadra2"

// ---- Indirizzi EEPROM RIALLOCATI a 512+ (la V1.3.0 usa 0-459; 49-248 e' il
//      testo scorrevole, quindi gli indirizzi storici 23-217 NON sono piu' liberi).
//      Richiede EEPROM_SIZE >= 1024 (impostato nel .ino).
#define EEPROM_MQTT_ENABLED_ADDR     512   // 1 byte
#define EEPROM_MQTT_SERVER_ADDR      513   // 64 byte (513-576)
#define EEPROM_MQTT_PORT_ADDR        577   // 2 byte  (577-578)
#define EEPROM_MQTT_USER_ADDR        579   // 32 byte (579-610)
#define EEPROM_MQTT_PASSWORD_ADDR    611   // 32 byte (611-642)
#define EEPROM_MQTT_BASE_TOPIC_ADDR  643   // 32 byte (643-674)
#define EEPROM_MQTT_DEVICE_NAME_ADDR 675   // 32 byte (675-706)
#define EEPROM_RANDOM_MODE_ADDR      707   // 1 byte
#define EEPROM_RANDOM_INTERVAL_ADDR  708   // 2 byte  (708-709)
#define EEPROM_TEST_MODE_ADDR        710   // 1 byte

// ---- Forward declarations di funzioni del .ino definite piu' avanti ----
void updateDisplay();
void applyPreset(uint8_t preset);
bool isNightTime();
bool isSleepTime();
void publishMqttState();
void publishMqttDiscovery();
bool mqttReconnect();
void setupMQTT();
void loadMqttConfigFromEEPROM();
void saveMqttConfigToEEPROM();

#if ENABLE_MQTT
// ---- Stato e configurazione MQTT (globali) ----
WiFiClient   espClient;
PubSubClient mqttClient(espClient);
bool          mqttConnected = false;
bool          mqttEnabled   = true;
unsigned long lastMqttCheck   = 0;
unsigned long lastMqttPublish = 0;
String   mqttServer     = MQTT_SERVER_DEFAULT;
uint16_t mqttPort       = MQTT_PORT_DEFAULT;
String   mqttUser       = MQTT_USER_DEFAULT;
String   mqttPassword   = MQTT_PASSWORD_DEFAULT;
String   mqttBaseTopic  = MQTT_BASE_TOPIC_DEFAULT;
String   mqttDeviceName = MQTT_DEVICE_NAME_DEFAULT;
#endif


#if ENABLE_MQTT
void handleSetRandomMode() {
    if (server.hasArg("enabled")) {
        randomModeEnabled = (server.arg("enabled").toInt() == 1);
        EEPROM.write(EEPROM_RANDOM_MODE_ADDR, randomModeEnabled ? 1 : 0);
        EEPROM.commit();
        
        // Se disattivato, mantieni il preset corrente
        // Se attivato, inizia subito con un preset random
        if (randomModeEnabled) {
            currentPreset = random(NUM_MODES);  // 0-19
            applyPreset(currentPreset);
            EEPROM.write(EEPROM_PRESET_ADDR, currentPreset);
            EEPROM.commit();
            lastRandomChange = millis();
        }
        
        #if ENABLE_MQTT
        publishMqttState();
        #endif
        server.send(200, "text/plain", "OK");
        return;
    }
    server.send(400, "text/plain", "Invalid parameter");
}

// Gestisce richiesta web per impostare intervallo cambio modalità random
void handleSetRandomInterval() {
    if (server.hasArg("interval")) {
        uint16_t intervalSeconds = server.arg("interval").toInt();
        if (intervalSeconds < 5) intervalSeconds = 5;
        if (intervalSeconds > 300) intervalSeconds = 300;
        
        randomChangeInterval = intervalSeconds * 1000; // Converti in millisecondi
        
        // Salva in EEPROM (2 byte)
        EEPROM.write(EEPROM_RANDOM_INTERVAL_ADDR, randomChangeInterval & 0xFF);
        EEPROM.write(EEPROM_RANDOM_INTERVAL_ADDR + 1, (randomChangeInterval >> 8) & 0xFF);
        EEPROM.commit();
        
        Serial.print("Intervallo random impostato a: ");
        Serial.print(intervalSeconds);
        Serial.println(" secondi");
        
        #if ENABLE_MQTT
        publishMqttState();
        #endif
        server.send(200, "text/plain", "OK");
        return;
    }
    server.send(400, "text/plain", "Invalid parameter");
}


void handleSetMqttSettings() {
    if (server.hasArg("enabled") && server.hasArg("server") && 
        server.hasArg("port") && server.hasArg("user") && 
        server.hasArg("password") && server.hasArg("baseTopic") &&
        server.hasArg("deviceName")) {
        
        // Leggi tutti i parametri dalla richiesta web
        mqttEnabled = (server.arg("enabled") == "true" || server.arg("enabled") == "1");
        mqttServer = server.arg("server");
        mqttPort = server.arg("port").toInt();
        mqttUser = server.arg("user");
        mqttPassword = server.arg("password");
        mqttBaseTopic = server.arg("baseTopic");
        mqttDeviceName = server.arg("deviceName");
        
        // Controlla che la porta sia valida
        if (mqttPort == 65535 || mqttPort == 0) {
            mqttPort = 1883; // Usa porta default MQTT
        }
        if (mqttPort < 1 || mqttPort > 65535) {
            server.send(400, "text/plain", "Porta non valida (deve essere tra 1 e 65535)");
            return;
        }
        
        if (mqttServer.length() == 0 || mqttServer.length() > 64) {
            server.send(400, "text/plain", "Server non valido");
            return;
        }
        
        // Salva configurazione in EEPROM per persistenza
        saveMqttConfigToEEPROM();
        
        // Disconnetti e riconnetti con nuova configurazione
        if (mqttConnected) {
            mqttClient.disconnect();
            mqttConnected = false;
        }
        
        // Se MQTT è abilitato, riconfigura e riconnetti
        if (mqttEnabled) {
            setupMQTT();
            mqttReconnect();
        }
        
        server.send(200, "text/plain", "OK");
        return;
    }
    server.send(400, "text/plain", "Parametri mancanti");
}

// Restituisce lo stato corrente della configurazione MQTT all'interfaccia web
void handleGetMqttStatus() {
    DynamicJsonDocument doc(512);
    
    // Compila JSON con stato MQTT (password nascosta per sicurezza)
    doc["enabled"] = mqttEnabled;
    doc["connected"] = mqttConnected;
    doc["server"] = mqttServer;
    doc["port"] = mqttPort;
    doc["user"] = mqttUser;
    doc["password"] = "***";  // Non inviare password reale
    doc["baseTopic"] = mqttBaseTopic;
    doc["deviceName"] = mqttDeviceName;
    doc["state"] = mqttClient.state();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    server.send(200, "application/json", jsonString);
}

// =============================================================================

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Converte il payload in stringa
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.print("Messaggio MQTT ricevuto su topic: ");
    Serial.print(topic);
    Serial.print(" - Messaggio: ");
    Serial.println(message);
    
    // Controlla se il messaggio è sul topic comandi
    String topicStr = String(topic);
    String commandTopic = mqttBaseTopic + "/command";
    
    if (topicStr == commandTopic) {
        // Prova a interpretare come JSON (formato Home Assistant)
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, message);
        
        if (!error) {
            // Comando JSON valido - processa i vari parametri
            bool stateChanged = false;
            
            // Gestione accensione/spegnimento display
            if (doc.containsKey("state")) {
                String state = doc["state"].as<String>();
                if (state == "ON" || state == "on") {
                    displayOff = false;
                    updateDisplay();
                    stateChanged = true;
                } else if (state == "OFF" || state == "off") {
                    displayOff = true;
                    FastLED.clear();
                    FastLED.show();
                    stateChanged = true;
                }
            }
            
            // Gestione luminosità (accetta 0-100 o 0-255)
            if (doc.containsKey("brightness")) {
                int brightness = doc["brightness"].as<int>();
                // Accetta sia 0-100 (percentuale) che 0-255 (valore diretto)
                if (brightness >= 0 && brightness <= 100) {
                    intBrightness = map(brightness, 0, 100, 0, 255);
                    FastLED.setBrightness(intBrightness);
                    FastLED.show();
                    stateChanged = true;
                } else if (brightness > 100 && brightness <= 255) {
                    // Se è > 100, assume che sia già in formato 0-255
                    intBrightness = brightness;
                    FastLED.setBrightness(intBrightness);
                    FastLED.show();
                    stateChanged = true;
                }
            }
            
            // Gestione cambio preset/modalità visualizzazione
            if (doc.containsKey("preset")) {
                int preset = -1;
                if (doc["preset"].is<int>()) {
                    preset = doc["preset"].as<int>();
                } else if (doc["preset"].is<String>()) {
                    preset = doc["preset"].as<String>().toInt();
                }
                
                if (preset >= 0 && preset < NUM_MODES) {
                    currentPreset = preset;
                    applyPreset(currentPreset);
                    EEPROM.write(EEPROM_PRESET_ADDR, currentPreset);
                    EEPROM.commit();
                    stateChanged = true;
                }
            } else if (doc.containsKey("mode")) {
                // Gestione mode (compatibilità con versioni precedenti)
                int mode = -1;
                if (doc["mode"].is<int>()) {
                    mode = doc["mode"].as<int>();
                } else if (doc["mode"].is<String>()) {
                    String modeStr = doc["mode"].as<String>();
                    modeStr.toLowerCase();
                    // Converti nome modalità in numero
                    if (modeStr == "fade") mode = 0;
                    else if (modeStr == "slow") mode = 1;
                    else if (modeStr == "fast") mode = 2;
                    else if (modeStr == "matrix") mode = 3;
                    else if (modeStr == "matrix2") mode = 4;
                    else if (modeStr == "tron") mode = 5;
                    else if (modeStr == "moto") mode = 6;
                    else if (modeStr == "galaga") mode = 7;
                    else if (modeStr == "pacman") mode = 8;
                    else if (modeStr == "digitale") mode = 9;
                    else mode = modeStr.toInt(); // Fallback a conversione numerica
                }
                
                if (mode >= 0 && mode < NUM_MODES) {
                    currentMode = mode;
                    updateDisplay();
                    stateChanged = true;
                }
            }
            
            // Gestione lampeggio secondi
            if (doc.containsKey("blink")) {
                int blink = doc["blink"].as<int>();
                currentBlink = (blink == 1 || blink == true) ? 1 : 0;
                EEPROM.write(EEPROM_BLINK_ADDR, currentBlink);
                EEPROM.commit();
                updateDisplay();
                stateChanged = true;
            }
            
            // Gestione spegnimento notturno
            if (doc.containsKey("sleepModeEnabled")) {
                bool sleepEnabled = doc["sleepModeEnabled"].as<bool>();
                sleepModeEnabled = sleepEnabled;
                EEPROM.write(EEPROM_SLEEP_ENABLED_ADDR, sleepEnabled ? 1 : 0);
                EEPROM.commit();
                stateChanged = true;
            }
            
            // Gestione modalità random (cambio automatico tra visualizzazioni)
            if (doc.containsKey("randomModeEnabled")) {
                bool randomEnabled = doc["randomModeEnabled"].as<bool>();
                randomModeEnabled = randomEnabled;
                EEPROM.write(EEPROM_RANDOM_MODE_ADDR, randomEnabled ? 1 : 0);
                EEPROM.commit();
                
                // Se attivato, cambia subito a un preset casuale
                if (randomEnabled) {
                    currentPreset = random(NUM_MODES);  // 0-19
                    applyPreset(currentPreset);
                    EEPROM.write(EEPROM_PRESET_ADDR, currentPreset);
                    EEPROM.commit();
                    lastRandomChange = millis();
                }
                stateChanged = true;
            }
            
            // Gestione intervallo cambio modalità random
            if (doc.containsKey("randomInterval")) {
                uint16_t intervalSeconds = doc["randomInterval"].as<uint16_t>();
                if (intervalSeconds >= 5 && intervalSeconds <= 300) {
                    randomChangeInterval = intervalSeconds * 1000; // Converti in millisecondi
                    EEPROM.write(EEPROM_RANDOM_INTERVAL_ADDR, randomChangeInterval & 0xFF);
                    EEPROM.write(EEPROM_RANDOM_INTERVAL_ADDR + 1, (randomChangeInterval >> 8) & 0xFF);
                    EEPROM.commit();
                    stateChanged = true;
                }
            }
            
            // Gestione ora e minuto di test (impostazione valori separata)
            if (doc.containsKey("testHour")) {
                int hour = doc["testHour"].as<int>();
                if (hour >= 0 && hour <= 23) {
                    testHour = hour;
                    stateChanged = true;
                    Serial.print("Ora di test impostata a: ");
                    Serial.println(testHour);
                }
            }
            if (doc.containsKey("testMinute")) {
                int minute = doc["testMinute"].as<int>();
                if (minute >= 0 && minute <= 59) {
                    testMinute = minute;
                    stateChanged = true;
                    Serial.print("Minuto di test impostato a: ");
                    Serial.println(testMinute);
                }
            }
            
            // Gestione modalità test (mostra orario personalizzato per 5 secondi)
            if (doc.containsKey("testMode")) {
                bool testMode = doc["testMode"].as<bool>();
                if (testMode) {
                    testModeEnabled = true;
                    testModeStartTime = millis();
                    EEPROM.write(EEPROM_TEST_MODE_ADDR, 1);
                    EEPROM.commit();
                    updateDisplay(); // Aggiorna subito con orario test
                    stateChanged = true;
                    Serial.print("Modalità test attivata - mostra orario ");
                    Serial.print(testHour);
                    Serial.print(":");
                    if (testMinute < 10) Serial.print("0");
                    Serial.println(testMinute);
                }
            }
            
            // Gestione riavvio dispositivo
            if (doc.containsKey("reboot")) {
                bool reboot = doc["reboot"].as<bool>();
                if (reboot) {
                    publishMqttState();  // Pubblica stato prima del riavvio
                    delay(500);
                    ESP.restart();
                    return;  // Non necessario ma per sicurezza
                }
            }
            
            // Gestione luminosità giorno
            if (doc.containsKey("dayBrightness")) {
                int dayBright = doc["dayBrightness"].as<int>();
                if (dayBright >= 1 && dayBright <= 100) {
                    dayBrightness = map(dayBright, 0, 100, 0, 255);
                    EEPROM.write(EEPROM_DAY_BRIGHTNESS_ADDR, dayBrightness);
                    EEPROM.commit();
                    // Applica se siamo in fascia giorno
                    if (!isNightTime() && !isSleepTime()) {
                        intBrightness = dayBrightness;
                        FastLED.setBrightness(intBrightness);
                        FastLED.show();
                    }
                    stateChanged = true;
                }
            }
            
            // Gestione luminosità notte
            if (doc.containsKey("nightBrightness")) {
                int nightBright = doc["nightBrightness"].as<int>();
                if (nightBright >= 1 && nightBright <= 100) {
                    nightBrightness = map(nightBright, 0, 100, 0, 255);
                    EEPROM.write(EEPROM_NIGHT_BRIGHTNESS_ADDR, nightBrightness);
                    EEPROM.commit();
                    // Applica se siamo in fascia notte
                    if (isNightTime() && !isSleepTime()) {
                        intBrightness = nightBrightness;
                        FastLED.setBrightness(intBrightness);
                        FastLED.show();
                    }
                    stateChanged = true;
                }
            }
            
            // Gestione orario inizio giorno
            if (doc.containsKey("dayStartTime")) {
                String dayStart = doc["dayStartTime"].as<String>();
                if (dayStart.length() == 5 && dayStart.indexOf(':') == 2) {
                    dayStartHour = dayStart.substring(0, 2).toInt();
                    dayStartMinute = dayStart.substring(3, 5).toInt();
                    EEPROM.write(EEPROM_DAY_START_HOUR_ADDR, dayStartHour);
                    EEPROM.write(EEPROM_DAY_START_MINUTE_ADDR, dayStartMinute);
                    EEPROM.commit();
                    stateChanged = true;
                }
            }
            
            // Gestione orario inizio notte
            if (doc.containsKey("nightStartTime")) {
                String nightStart = doc["nightStartTime"].as<String>();
                if (nightStart.length() == 5 && nightStart.indexOf(':') == 2) {
                    nightStartHour = nightStart.substring(0, 2).toInt();
                    nightStartMinute = nightStart.substring(3, 5).toInt();
                    EEPROM.write(EEPROM_NIGHT_START_HOUR_ADDR, nightStartHour);
                    EEPROM.write(EEPROM_NIGHT_START_MINUTE_ADDR, nightStartMinute);
                    EEPROM.commit();
                    stateChanged = true;
                }
            }
            
            // Gestione orario inizio spegnimento
            if (doc.containsKey("sleepStartTime")) {
                String sleepStart = doc["sleepStartTime"].as<String>();
                if (sleepStart.length() == 5 && sleepStart.indexOf(':') == 2) {
                    sleepStartHour = sleepStart.substring(0, 2).toInt();
                    sleepStartMinute = sleepStart.substring(3, 5).toInt();
                    EEPROM.write(EEPROM_SLEEP_START_HOUR_ADDR, sleepStartHour);
                    EEPROM.write(EEPROM_SLEEP_START_MINUTE_ADDR, sleepStartMinute);
                    EEPROM.commit();
                    stateChanged = true;
                }
            }
            
            // Gestione cambio colore per nome (Bianco, Blu, Rosso, ecc.)
            if (doc.containsKey("colorName")) {
                String colorName = doc["colorName"].as<String>();
                colorName.toLowerCase();
                CRGB newColor;
                if (colorName == "bianco") {
                    newColor = CRGB::White;
                } else if (colorName == "blu") {
                    newColor = CRGB::Blue;
                } else if (colorName == "rosso") {
                    newColor = CRGB::Red;
                } else if (colorName == "verde") {
                    newColor = CRGB::Green;
                } else if (colorName == "giallo") {
                    newColor = CRGB::Yellow;
                } else if (colorName == "arancione") {
                    newColor = CRGB(255, 165, 0);
                } else {
                    newColor = clockColors.current; // Mantieni colore corrente se nome non valido
                }
                clockColors.current = newColor;
                updateDisplay();
                stateChanged = true;
            }
            
            // Gestione cambio colore RGB (formato oggetto {r, g, b})
            if (doc.containsKey("color")) {
                JsonObject color = doc["color"];
                if (color.containsKey("r") && color.containsKey("g") && color.containsKey("b")) {
                    CRGB newColor;
                    newColor.r = color["r"].as<uint8_t>();
                    newColor.g = color["g"].as<uint8_t>();
                    newColor.b = color["b"].as<uint8_t>();
                    clockColors.current = newColor;
                    updateDisplay();
                    stateChanged = true;
                }
            } else if (doc.containsKey("rgb")) {
                // Gestione cambio colore RGB (formato array [r, g, b])
                JsonArray rgb = doc["rgb"];
                if (rgb.size() == 3) {
                    CRGB newColor;
                    newColor.r = rgb[0].as<uint8_t>();
                    newColor.g = rgb[1].as<uint8_t>();
                    newColor.b = rgb[2].as<uint8_t>();
                    clockColors.current = newColor;
                    updateDisplay();
                    stateChanged = true;
                }
            }
            
            if (stateChanged) {
                publishMqttState();
            }
        } else {
            // Comando testuale (compatibilità con comandi semplici)
            if (message == "ON" || message == "on") {
                displayOff = false;
                updateDisplay();
                publishMqttState();
            } else if (message == "OFF" || message == "off") {
                displayOff = true;
                FastLED.clear();
                FastLED.show();
                publishMqttState();
            } else if (message.startsWith("MODE:")) {
                int mode = message.substring(5).toInt();
                if (mode >= 0 && mode < NUM_MODES) {
                    currentMode = mode;
                    updateDisplay();
                    publishMqttState();
                }
            } else if (message.startsWith("COLOR:")) {
                String colorHex = message.substring(6);
                if (colorHex.length() == 6) {
                    long colorValue = strtol(colorHex.c_str(), NULL, 16);
                    CRGB newColor;
                    newColor.r = (colorValue >> 16) & 0xFF;
                    newColor.g = (colorValue >> 8) & 0xFF;
                    newColor.b = colorValue & 0xFF;
                    clockColors.current = newColor;
                    updateDisplay();
                    publishMqttState();
                }
            } else if (message.startsWith("BRIGHTNESS:")) {
                int brightness = message.substring(11).toInt();
                if (brightness >= 0 && brightness <= 100) {
                    intBrightness = map(brightness, 0, 100, 0, 255);
                    FastLED.setBrightness(intBrightness);
                    FastLED.show();
                    publishMqttState();
                }
            }
        }
    }
}

// Pubblica configurazione MQTT Discovery per Home Assistant
// Crea automaticamente le entità (Light, Switch, Sensor, ecc.) in Home Assistant
void publishMqttDiscovery() {
    if (!mqttConnected || !mqttEnabled) {
        return;
    }
    
    // Prepara prefisso e ID dispositivo per Discovery
    String discoveryPrefix = "homeassistant";
    String deviceId = mqttDeviceName;
    deviceId.toLowerCase();
    deviceId.replace(" ", "_");
    
    // Configurazione entità Light (principale per controllo display)
    DynamicJsonDocument lightConfig(1536);  // Aumentato buffer per configurazione completa
    lightConfig["name"] = mqttDeviceName;
    lightConfig["unique_id"] = deviceId + "_light";
    lightConfig["state_topic"] = mqttBaseTopic + "/state";
    lightConfig["command_topic"] = mqttBaseTopic + "/command";
    lightConfig["availability_topic"] = mqttBaseTopic + "/availability";
    lightConfig["availability_template"] = "{{ value }}";
    lightConfig["schema"] = "json";
    lightConfig["brightness"] = true;
    lightConfig["rgb"] = true;
    lightConfig["state_value_template"] = "{{ value_json.state }}";
    lightConfig["brightness_value_template"] = "{{ value_json.brightness }}";
    lightConfig["rgb_value_template"] = "{{ value_json.color.r }},{{ value_json.color.g }},{{ value_json.color.b }}";
    lightConfig["color_mode"] = true;
    lightConfig["supported_color_modes"][0] = "rgb";
    lightConfig["payload_on"] = "ON";
    lightConfig["payload_off"] = "OFF";
    lightConfig["optimistic"] = false;
    lightConfig["retain"] = true;
    lightConfig["qos"] = 1;
    
    // Informazioni dispositivo (completo per Home Assistant)
    JsonObject device = lightConfig.createNestedObject("device");
    JsonArray identifiers = device.createNestedArray("identifiers");
    identifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    device["sw_version"] = "1.3.0+MQTT";
    
    String lightConfigTopic = discoveryPrefix + "/light/" + deviceId + "/config";
    String lightConfigJson;
    serializeJson(lightConfig, lightConfigJson);
    
    Serial.print("Pubblicando configurazione Light su: ");
    Serial.println(lightConfigTopic);
    Serial.print("Config JSON: ");
    Serial.println(lightConfigJson);
    
    bool published = mqttClient.publish(lightConfigTopic.c_str(), lightConfigJson.c_str(), true);
    if (published) {
        Serial.println("✓ Configurazione Light pubblicata con successo");
    } else {
        Serial.println("✗ Errore nella pubblicazione configurazione Light");
    }
    
    // Sensore: Modalità visualizzazione corrente (mostra nome preset)
    DynamicJsonDocument modeSensorConfig(512);
    modeSensorConfig["name"] = mqttDeviceName + " Mode";
    modeSensorConfig["unique_id"] = deviceId + "_mode";
    modeSensorConfig["state_topic"] = mqttBaseTopic + "/state";
    // Usa il campo mode_name che contiene il nome del preset
    modeSensorConfig["value_template"] = "{{ value_json.mode_name }}";
    modeSensorConfig["icon"] = "mdi:palette";
    device = modeSensorConfig.createNestedObject("device");
    JsonArray modeIdentifiers = device.createNestedArray("identifiers");
    modeIdentifiers.add(deviceId);
    
    String modeSensorTopic = discoveryPrefix + "/sensor/" + deviceId + "_mode/config";
    String modeSensorJson;
    serializeJson(modeSensorConfig, modeSensorJson);
    mqttClient.publish(modeSensorTopic.c_str(), modeSensorJson.c_str(), true);
    
    // Sensore: Ora corrente (formato HH:MM:SS)
    DynamicJsonDocument timeSensorConfig(512);
    timeSensorConfig["name"] = mqttDeviceName + " Time";
    timeSensorConfig["unique_id"] = deviceId + "_time";
    timeSensorConfig["state_topic"] = mqttBaseTopic + "/state";
    timeSensorConfig["value_template"] = "{{ value_json.time }}";
    timeSensorConfig["icon"] = "mdi:clock";
    // Rimosso device_class timestamp che causava problemi
    device = timeSensorConfig.createNestedObject("device");
    JsonArray timeIdentifiers = device.createNestedArray("identifiers");
    timeIdentifiers.add(deviceId);
    
    String timeSensorTopic = discoveryPrefix + "/sensor/" + deviceId + "_time/config";
    String timeSensorJson;
    serializeJson(timeSensorConfig, timeSensorJson);
    mqttClient.publish(timeSensorTopic.c_str(), timeSensorJson.c_str(), true);
    
    // Sensore: Stato lampeggio secondi
    DynamicJsonDocument blinkSensorConfig(512);
    blinkSensorConfig["name"] = mqttDeviceName + " Blink";
    blinkSensorConfig["unique_id"] = deviceId + "_blink";
    blinkSensorConfig["state_topic"] = mqttBaseTopic + "/state";
    blinkSensorConfig["value_template"] = "{{ value_json.blink }}";
    blinkSensorConfig["icon"] = "mdi:flash";
    blinkSensorConfig["device_class"] = "enum";
    device = blinkSensorConfig.createNestedObject("device");
    JsonArray blinkIdentifiers = device.createNestedArray("identifiers");
    blinkIdentifiers.add(deviceId);
    
    String blinkSensorTopic = discoveryPrefix + "/sensor/" + deviceId + "_blink/config";
    String blinkSensorJson;
    serializeJson(blinkSensorConfig, blinkSensorJson);
    mqttClient.publish(blinkSensorTopic.c_str(), blinkSensorJson.c_str(), true);
    
    // Switch: Accensione/Spegnimento display
    DynamicJsonDocument powerSwitchConfig(512);
    powerSwitchConfig["name"] = mqttDeviceName + " Power";
    powerSwitchConfig["unique_id"] = deviceId + "_power";
    powerSwitchConfig["state_topic"] = mqttBaseTopic + "/state";
    powerSwitchConfig["command_topic"] = mqttBaseTopic + "/command";
    powerSwitchConfig["value_template"] = "{{ 'ON' if value_json.state == 'ON' else 'OFF' }}";
    powerSwitchConfig["payload_on"] = "{\"state\":\"ON\"}";
    powerSwitchConfig["payload_off"] = "{\"state\":\"OFF\"}";
    powerSwitchConfig["state_on"] = "ON";
    powerSwitchConfig["state_off"] = "OFF";
    powerSwitchConfig["icon"] = "mdi:power";
    device = powerSwitchConfig.createNestedObject("device");
    JsonArray powerIdentifiers = device.createNestedArray("identifiers");
    powerIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String powerSwitchTopic = discoveryPrefix + "/switch/" + deviceId + "_power/config";
    String powerSwitchJson;
    serializeJson(powerSwitchConfig, powerSwitchJson);
    mqttClient.publish(powerSwitchTopic.c_str(), powerSwitchJson.c_str(), true);
    
    // Select: Scelta modalità visualizzazione (19 preset disponibili)
    DynamicJsonDocument modeSelectConfig(2048);
    modeSelectConfig["name"] = mqttDeviceName + " Mode";
    modeSelectConfig["unique_id"] = deviceId + "_mode_select";
    modeSelectConfig["state_topic"] = mqttBaseTopic + "/state";
    modeSelectConfig["command_topic"] = mqttBaseTopic + "/command";
    // Usa il campo mode_name per lo stato
    modeSelectConfig["value_template"] = "{{ value_json.mode_name }}";
    // Template per convertire nome preset in numero quando si invia comando
    modeSelectConfig["command_template"] = "{% if value == 'Lento Casuale' %}{\"preset\":0}{% elif value == 'Sfumato Blu' %}{\"preset\":1}{% elif value == 'Matrix Verde' %}{\"preset\":2}{% elif value == 'Lento Arancione' %}{\"preset\":3}{% elif value == 'Matrix Blu' %}{\"preset\":4}{% elif value == 'Matrix Giallo' %}{\"preset\":5}{% elif value == 'Sfumato Verde' %}{\"preset\":6}{% elif value == 'Lento Blu' %}{\"preset\":7}{% elif value == 'Matrix Ciano' %}{\"preset\":8}{% elif value == 'Sfumato Rosa' %}{\"preset\":9}{% elif value == 'Veloce Giallo' %}{\"preset\":10}{% elif value == 'Veloce Bianco' %}{\"preset\":11}{% elif value == 'Veloce Ciano' %}{\"preset\":12}{% elif value == 'Effetto TRON' %}{\"preset\":13}{% elif value == 'Effetto TRON 2' %}{\"preset\":14}{% elif value == 'Effetto Moto' %}{\"preset\":15}{% elif value == 'Effetto Galaga' %}{\"preset\":16}{% elif value == 'Effetto Pacman' %}{\"preset\":17}{% elif value == 'Digitale' %}{\"preset\":18}{% elif value == 'Arkanoid' %}{\"preset\":19}{% elif value == 'Natale' %}{\"preset\":20}{% elif value == 'Neve' %}{\"preset\":21}{% elif value == 'Laser' %}{\"preset\":22}{% elif value == 'Fuoco' %}{\"preset\":23}{% elif value == 'Black Hole' %}{\"preset\":24}{% elif value == 'Fuoco Realistico' %}{\"preset\":25}{% elif value == 'Fuochi Artificio' %}{\"preset\":26}{% elif value == 'Drift' %}{\"preset\":27}{% elif value == 'Plasma' %}{\"preset\":28}{% elif value == 'Aurora' %}{\"preset\":29}{% elif value == 'Radar' %}{\"preset\":30}{% elif value == 'Cielo Stellato' %}{\"preset\":31}{% elif value == 'Battito' %}{\"preset\":32}{% elif value == 'Tetris' %}{\"preset\":33}{% elif value == 'Snake' %}{\"preset\":34}{% elif value == 'Sabbia' %}{\"preset\":35}{% elif value == 'Temporale' %}{\"preset\":36}{% elif value == 'Bolle' %}{\"preset\":37}{% elif value == 'Pong' %}{\"preset\":38}{% elif value == 'Lava Lamp' %}{\"preset\":39}{% else %}{\"preset\":0}{% endif %}";
    JsonArray options = modeSelectConfig.createNestedArray("options");
    options.add("Lento Casuale");
    options.add("Sfumato Blu");
    options.add("Matrix Verde");
    options.add("Lento Arancione");
    options.add("Matrix Blu");
    options.add("Matrix Giallo");
    options.add("Sfumato Verde");
    options.add("Lento Blu");
    options.add("Matrix Ciano");
    options.add("Sfumato Rosa");
    options.add("Veloce Giallo");
    options.add("Veloce Bianco");
    options.add("Veloce Ciano");
    options.add("Effetto TRON");
    options.add("Effetto TRON 2");
    options.add("Effetto Moto");
    options.add("Effetto Galaga");
    options.add("Effetto Pacman");
    options.add("Digitale");
    options.add("Arkanoid");
    options.add("Natale");
    options.add("Neve");
    options.add("Laser");
    options.add("Fuoco");
    options.add("Black Hole");
    options.add("Fuoco Realistico");
    options.add("Fuochi Artificio");
    options.add("Drift");
    options.add("Plasma");
    options.add("Aurora");
    options.add("Radar");
    options.add("Cielo Stellato");
    options.add("Battito");
    options.add("Tetris");
    options.add("Snake");
    options.add("Sabbia");
    options.add("Temporale");
    options.add("Bolle");
    options.add("Pong");
    options.add("Lava Lamp");
    modeSelectConfig["icon"] = "mdi:palette";
    device = modeSelectConfig.createNestedObject("device");
    JsonArray modeSelectIdentifiers = device.createNestedArray("identifiers");
    modeSelectIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String modeSelectTopic = discoveryPrefix + "/select/" + deviceId + "_mode/config";
    String modeSelectJson;
    serializeJson(modeSelectConfig, modeSelectJson);
    mqttClient.publish(modeSelectTopic.c_str(), modeSelectJson.c_str(), true);
    
    // Number: Regolazione luminosità corrente (0-100%)
    DynamicJsonDocument brightnessNumberConfig(512);
    brightnessNumberConfig["name"] = mqttDeviceName + " Brightness";
    brightnessNumberConfig["unique_id"] = deviceId + "_brightness";
    brightnessNumberConfig["state_topic"] = mqttBaseTopic + "/state";
    brightnessNumberConfig["command_topic"] = mqttBaseTopic + "/command";
    brightnessNumberConfig["value_template"] = "{{ value_json.brightness }}";
    brightnessNumberConfig["command_template"] = "{\"brightness\":{{ value }}}";  // Accetta 0-100
    brightnessNumberConfig["min"] = 0;
    brightnessNumberConfig["max"] = 100;
    brightnessNumberConfig["step"] = 1;
    brightnessNumberConfig["unit_of_measurement"] = "%";
    brightnessNumberConfig["icon"] = "mdi:brightness-6";
    device = brightnessNumberConfig.createNestedObject("device");
    JsonArray brightnessIdentifiers = device.createNestedArray("identifiers");
    brightnessIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String brightnessNumberTopic = discoveryPrefix + "/number/" + deviceId + "_brightness/config";
    String brightnessNumberJson;
    serializeJson(brightnessNumberConfig, brightnessNumberJson);
    mqttClient.publish(brightnessNumberTopic.c_str(), brightnessNumberJson.c_str(), true);
    
    // Switch: Attiva/Disattiva lampeggio secondi
    DynamicJsonDocument blinkSwitchConfig(512);
    blinkSwitchConfig["name"] = mqttDeviceName + " Blink";
    blinkSwitchConfig["unique_id"] = deviceId + "_blink_switch";
    blinkSwitchConfig["state_topic"] = mqttBaseTopic + "/state";
    blinkSwitchConfig["command_topic"] = mqttBaseTopic + "/command";
    blinkSwitchConfig["value_template"] = "{{ 'ON' if value_json.blink == 1 else 'OFF' }}";
    blinkSwitchConfig["payload_on"] = "{\"blink\":1}";
    blinkSwitchConfig["payload_off"] = "{\"blink\":0}";
    blinkSwitchConfig["state_on"] = "ON";
    blinkSwitchConfig["state_off"] = "OFF";
    blinkSwitchConfig["icon"] = "mdi:flash";
    device = blinkSwitchConfig.createNestedObject("device");
    JsonArray blinkSwitchIdentifiers = device.createNestedArray("identifiers");
    blinkSwitchIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String blinkSwitchTopic = discoveryPrefix + "/switch/" + deviceId + "_blink/config";
    String blinkSwitchJson;
    serializeJson(blinkSwitchConfig, blinkSwitchJson);
    mqttClient.publish(blinkSwitchTopic.c_str(), blinkSwitchJson.c_str(), true);
    
    // Switch: Attiva/Disattiva spegnimento notturno automatico
    DynamicJsonDocument sleepSwitchConfig(512);
    sleepSwitchConfig["name"] = mqttDeviceName + " Sleep Mode";
    sleepSwitchConfig["unique_id"] = deviceId + "_sleep_mode";
    sleepSwitchConfig["state_topic"] = mqttBaseTopic + "/state";
    sleepSwitchConfig["command_topic"] = mqttBaseTopic + "/command";
    sleepSwitchConfig["value_template"] = "{{ 'ON' if value_json.sleepModeEnabled == true else 'OFF' }}";
    sleepSwitchConfig["payload_on"] = "{\"sleepModeEnabled\":true}";
    sleepSwitchConfig["payload_off"] = "{\"sleepModeEnabled\":false}";
    sleepSwitchConfig["state_on"] = "ON";
    sleepSwitchConfig["state_off"] = "OFF";
    sleepSwitchConfig["icon"] = "mdi:sleep";
    device = sleepSwitchConfig.createNestedObject("device");
    JsonArray sleepSwitchIdentifiers = device.createNestedArray("identifiers");
    sleepSwitchIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String sleepSwitchTopic = discoveryPrefix + "/switch/" + deviceId + "_sleep_mode/config";
    String sleepSwitchJson;
    serializeJson(sleepSwitchConfig, sleepSwitchJson);
    mqttClient.publish(sleepSwitchTopic.c_str(), sleepSwitchJson.c_str(), true);
    
    // Button: Riavvia dispositivo ESP32
    DynamicJsonDocument rebootButtonConfig(512);
    rebootButtonConfig["name"] = mqttDeviceName + " Reboot";
    rebootButtonConfig["unique_id"] = deviceId + "_reboot";
    rebootButtonConfig["command_topic"] = mqttBaseTopic + "/command";
    rebootButtonConfig["payload_press"] = "{\"reboot\":true}";
    rebootButtonConfig["icon"] = "mdi:restart";
    device = rebootButtonConfig.createNestedObject("device");
    JsonArray rebootIdentifiers = device.createNestedArray("identifiers");
    rebootIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String rebootButtonTopic = discoveryPrefix + "/button/" + deviceId + "_reboot/config";
    String rebootButtonJson;
    serializeJson(rebootButtonConfig, rebootButtonJson);
    mqttClient.publish(rebootButtonTopic.c_str(), rebootButtonJson.c_str(), true);
    
    // Select: Scelta colore predefinito (Bianco, Blu, Rosso, ecc.)
    DynamicJsonDocument colorSelectConfig(1024);
    colorSelectConfig["name"] = mqttDeviceName + " Color";
    colorSelectConfig["unique_id"] = deviceId + "_color";
    colorSelectConfig["state_topic"] = mqttBaseTopic + "/state";
    colorSelectConfig["command_topic"] = mqttBaseTopic + "/command";
    colorSelectConfig["value_template"] = "{{ value_json.colorName }}";
    colorSelectConfig["command_template"] = "{% if value == 'Bianco' %}{\"colorName\":\"Bianco\"}{% elif value == 'Blu' %}{\"colorName\":\"Blu\"}{% elif value == 'Rosso' %}{\"colorName\":\"Rosso\"}{% elif value == 'Verde' %}{\"colorName\":\"Verde\"}{% elif value == 'Giallo' %}{\"colorName\":\"Giallo\"}{% elif value == 'Arancione' %}{\"colorName\":\"Arancione\"}{% endif %}";
    JsonArray colorOptions = colorSelectConfig.createNestedArray("options");
    colorOptions.add("Bianco");
    colorOptions.add("Blu");
    colorOptions.add("Rosso");
    colorOptions.add("Verde");
    colorOptions.add("Giallo");
    colorOptions.add("Arancione");
    colorSelectConfig["icon"] = "mdi:palette";
    device = colorSelectConfig.createNestedObject("device");
    JsonArray colorIdentifiers = device.createNestedArray("identifiers");
    colorIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String colorSelectTopic = discoveryPrefix + "/select/" + deviceId + "_color/config";
    String colorSelectJson;
    serializeJson(colorSelectConfig, colorSelectJson);
    mqttClient.publish(colorSelectTopic.c_str(), colorSelectJson.c_str(), true);
    
    // Number: Regolazione luminosità fascia giorno (0-100%)
    DynamicJsonDocument dayBrightnessConfig(512);
    dayBrightnessConfig["name"] = mqttDeviceName + " Day Brightness";
    dayBrightnessConfig["unique_id"] = deviceId + "_day_brightness";
    dayBrightnessConfig["state_topic"] = mqttBaseTopic + "/state";
    dayBrightnessConfig["command_topic"] = mqttBaseTopic + "/command";
    dayBrightnessConfig["value_template"] = "{{ value_json.dayBrightness }}";
    dayBrightnessConfig["command_template"] = "{\"dayBrightness\":{{ value }}}";
    dayBrightnessConfig["min"] = 1;
    dayBrightnessConfig["max"] = 100;
    dayBrightnessConfig["step"] = 1;
    dayBrightnessConfig["unit_of_measurement"] = "%";
    dayBrightnessConfig["icon"] = "mdi:weather-sunny";
    device = dayBrightnessConfig.createNestedObject("device");
    JsonArray dayBrightIdentifiers = device.createNestedArray("identifiers");
    dayBrightIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String dayBrightTopic = discoveryPrefix + "/number/" + deviceId + "_day_brightness/config";
    String dayBrightJson;
    serializeJson(dayBrightnessConfig, dayBrightJson);
    mqttClient.publish(dayBrightTopic.c_str(), dayBrightJson.c_str(), true);
    
    // Number: Regolazione luminosità fascia notte (0-100%)
    DynamicJsonDocument nightBrightnessConfig(512);
    nightBrightnessConfig["name"] = mqttDeviceName + " Night Brightness";
    nightBrightnessConfig["unique_id"] = deviceId + "_night_brightness";
    nightBrightnessConfig["state_topic"] = mqttBaseTopic + "/state";
    nightBrightnessConfig["command_topic"] = mqttBaseTopic + "/command";
    nightBrightnessConfig["value_template"] = "{{ value_json.nightBrightness }}";
    nightBrightnessConfig["command_template"] = "{\"nightBrightness\":{{ value }}}";
    nightBrightnessConfig["min"] = 1;
    nightBrightnessConfig["max"] = 100;
    nightBrightnessConfig["step"] = 1;
    nightBrightnessConfig["unit_of_measurement"] = "%";
    nightBrightnessConfig["icon"] = "mdi:weather-night";
    device = nightBrightnessConfig.createNestedObject("device");
    JsonArray nightBrightIdentifiers = device.createNestedArray("identifiers");
    nightBrightIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String nightBrightTopic = discoveryPrefix + "/number/" + deviceId + "_night_brightness/config";
    String nightBrightJson;
    serializeJson(nightBrightnessConfig, nightBrightJson);
    mqttClient.publish(nightBrightTopic.c_str(), nightBrightJson.c_str(), true);
    
    // Text: Orario Inizio Giorno
    DynamicJsonDocument dayStartConfig(512);
    dayStartConfig["name"] = mqttDeviceName + " Day Start";
    dayStartConfig["unique_id"] = deviceId + "_day_start";
    dayStartConfig["state_topic"] = mqttBaseTopic + "/state";
    dayStartConfig["command_topic"] = mqttBaseTopic + "/command";
    dayStartConfig["value_template"] = "{{ value_json.dayStartTime }}";
    dayStartConfig["command_template"] = "{\"dayStartTime\":\"{{ value }}\"}";
    dayStartConfig["icon"] = "mdi:weather-sunny";
    device = dayStartConfig.createNestedObject("device");
    JsonArray dayStartIdentifiers = device.createNestedArray("identifiers");
    dayStartIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String dayStartTopic = discoveryPrefix + "/text/" + deviceId + "_day_start/config";
    String dayStartJson;
    serializeJson(dayStartConfig, dayStartJson);
    mqttClient.publish(dayStartTopic.c_str(), dayStartJson.c_str(), true);
    
    // Text: Orario Inizio Notte
    DynamicJsonDocument nightStartConfig(512);
    nightStartConfig["name"] = mqttDeviceName + " Night Start";
    nightStartConfig["unique_id"] = deviceId + "_night_start";
    nightStartConfig["state_topic"] = mqttBaseTopic + "/state";
    nightStartConfig["command_topic"] = mqttBaseTopic + "/command";
    nightStartConfig["value_template"] = "{{ value_json.nightStartTime }}";
    nightStartConfig["command_template"] = "{\"nightStartTime\":\"{{ value }}\"}";
    nightStartConfig["icon"] = "mdi:weather-night";
    device = nightStartConfig.createNestedObject("device");
    JsonArray nightStartIdentifiers = device.createNestedArray("identifiers");
    nightStartIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String nightStartTopic = discoveryPrefix + "/text/" + deviceId + "_night_start/config";
    String nightStartJson;
    serializeJson(nightStartConfig, nightStartJson);
    mqttClient.publish(nightStartTopic.c_str(), nightStartJson.c_str(), true);
    
    // Text: Orario Spegnimento Notturno
    DynamicJsonDocument sleepStartConfig(512);
    sleepStartConfig["name"] = mqttDeviceName + " Sleep Start";
    sleepStartConfig["unique_id"] = deviceId + "_sleep_start";
    sleepStartConfig["state_topic"] = mqttBaseTopic + "/state";
    sleepStartConfig["command_topic"] = mqttBaseTopic + "/command";
    sleepStartConfig["value_template"] = "{{ value_json.sleepStartTime }}";
    sleepStartConfig["command_template"] = "{\"sleepStartTime\":\"{{ value }}\"}";
    sleepStartConfig["icon"] = "mdi:sleep";
    device = sleepStartConfig.createNestedObject("device");
    JsonArray sleepStartIdentifiers = device.createNestedArray("identifiers");
    sleepStartIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String sleepStartTopic = discoveryPrefix + "/text/" + deviceId + "_sleep_start/config";
    String sleepStartJson;
    serializeJson(sleepStartConfig, sleepStartJson);
    mqttClient.publish(sleepStartTopic.c_str(), sleepStartJson.c_str(), true);
    
    // Sensor: IP Address
    DynamicJsonDocument ipSensorConfig(512);
    ipSensorConfig["name"] = mqttDeviceName + " IP Address";
    ipSensorConfig["unique_id"] = deviceId + "_ip_address";
    ipSensorConfig["state_topic"] = mqttBaseTopic + "/state";
    ipSensorConfig["value_template"] = "{{ value_json.ipAddress }}";
    ipSensorConfig["icon"] = "mdi:ip-network";
    device = ipSensorConfig.createNestedObject("device");
    JsonArray ipIdentifiers = device.createNestedArray("identifiers");
    ipIdentifiers.add(deviceId);
    
    String ipSensorTopic = discoveryPrefix + "/sensor/" + deviceId + "_ip_address/config";
    String ipSensorJson;
    serializeJson(ipSensorConfig, ipSensorJson);
    mqttClient.publish(ipSensorTopic.c_str(), ipSensorJson.c_str(), true);
    
    // Sensor: WiFi RSSI
    DynamicJsonDocument rssiSensorConfig(512);
    rssiSensorConfig["name"] = mqttDeviceName + " WiFi Signal";
    rssiSensorConfig["unique_id"] = deviceId + "_wifi_rssi";
    rssiSensorConfig["state_topic"] = mqttBaseTopic + "/state";
    rssiSensorConfig["value_template"] = "{{ value_json.rssi }}";
    rssiSensorConfig["device_class"] = "signal_strength";
    rssiSensorConfig["unit_of_measurement"] = "dBm";
    rssiSensorConfig["icon"] = "mdi:wifi";
    device = rssiSensorConfig.createNestedObject("device");
    JsonArray rssiIdentifiers = device.createNestedArray("identifiers");
    rssiIdentifiers.add(deviceId);
    
    String rssiSensorTopic = discoveryPrefix + "/sensor/" + deviceId + "_wifi_rssi/config";
    String rssiSensorJson;
    serializeJson(rssiSensorConfig, rssiSensorJson);
    mqttClient.publish(rssiSensorTopic.c_str(), rssiSensorJson.c_str(), true);
    
    // Binary Sensor: Stato connessione WiFi
    DynamicJsonDocument wifiBinaryConfig(512);
    wifiBinaryConfig["name"] = mqttDeviceName + " WiFi";
    wifiBinaryConfig["unique_id"] = deviceId + "_wifi_connected";
    wifiBinaryConfig["state_topic"] = mqttBaseTopic + "/state";
    // Template che converte booleano in stringa per Home Assistant
    wifiBinaryConfig["value_template"] = "{{ 'ON' if value_json.wifiConnected == true else 'OFF' }}";
    wifiBinaryConfig["device_class"] = "connectivity";
    wifiBinaryConfig["payload_on"] = "ON";
    wifiBinaryConfig["payload_off"] = "OFF";
    wifiBinaryConfig["icon"] = "mdi:wifi";
    device = wifiBinaryConfig.createNestedObject("device");
    JsonArray wifiIdentifiers = device.createNestedArray("identifiers");
    wifiIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String wifiBinaryTopic = discoveryPrefix + "/binary_sensor/" + deviceId + "_wifi/config";
    String wifiBinaryJson;
    serializeJson(wifiBinaryConfig, wifiBinaryJson);
    mqttClient.publish(wifiBinaryTopic.c_str(), wifiBinaryJson.c_str(), true);
    
    // Switch: Attiva/Disattiva modalità random (cambio automatico tra visualizzazioni)
    DynamicJsonDocument randomModeSwitchConfig(512);
    randomModeSwitchConfig["name"] = mqttDeviceName + " Random Mode";
    randomModeSwitchConfig["unique_id"] = deviceId + "_random_mode";
    randomModeSwitchConfig["state_topic"] = mqttBaseTopic + "/state";
    randomModeSwitchConfig["command_topic"] = mqttBaseTopic + "/command";
    randomModeSwitchConfig["value_template"] = "{{ 'ON' if value_json.randomModeEnabled == true else 'OFF' }}";
    randomModeSwitchConfig["payload_on"] = "{\"randomModeEnabled\":true}";
    randomModeSwitchConfig["payload_off"] = "{\"randomModeEnabled\":false}";
    randomModeSwitchConfig["state_on"] = "ON";
    randomModeSwitchConfig["state_off"] = "OFF";
    randomModeSwitchConfig["icon"] = "mdi:dice-multiple";
    device = randomModeSwitchConfig.createNestedObject("device");
    JsonArray randomIdentifiers = device.createNestedArray("identifiers");
    randomIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String randomSwitchTopic = discoveryPrefix + "/switch/" + deviceId + "_random_mode/config";
    String randomSwitchJson;
    serializeJson(randomModeSwitchConfig, randomSwitchJson);
    mqttClient.publish(randomSwitchTopic.c_str(), randomSwitchJson.c_str(), true);
    
    // Number: Intervallo cambio modalità random (5-300 secondi)
    DynamicJsonDocument randomIntervalConfig(512);
    randomIntervalConfig["name"] = mqttDeviceName + " Random Interval";
    randomIntervalConfig["unique_id"] = deviceId + "_random_interval";
    randomIntervalConfig["state_topic"] = mqttBaseTopic + "/state";
    randomIntervalConfig["command_topic"] = mqttBaseTopic + "/command";
    randomIntervalConfig["value_template"] = "{{ value_json.randomInterval }}";
    randomIntervalConfig["command_template"] = "{\"randomInterval\":{{ value }}}";
    randomIntervalConfig["min"] = 5;
    randomIntervalConfig["max"] = 300;
    randomIntervalConfig["step"] = 1;
    randomIntervalConfig["unit_of_measurement"] = "s";
    randomIntervalConfig["icon"] = "mdi:timer";
    device = randomIntervalConfig.createNestedObject("device");
    JsonArray randomIntervalIdentifiers = device.createNestedArray("identifiers");
    randomIntervalIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String randomIntervalTopic = discoveryPrefix + "/number/" + deviceId + "_random_interval/config";
    String randomIntervalJson;
    serializeJson(randomIntervalConfig, randomIntervalJson);
    mqttClient.publish(randomIntervalTopic.c_str(), randomIntervalJson.c_str(), true);
    
    // Number: Ora di test (0-23)
    DynamicJsonDocument testHourConfig(512);
    testHourConfig["name"] = mqttDeviceName + " Test Hour";
    testHourConfig["unique_id"] = deviceId + "_test_hour";
    testHourConfig["state_topic"] = mqttBaseTopic + "/state";
    testHourConfig["command_topic"] = mqttBaseTopic + "/command";
    testHourConfig["value_template"] = "{{ value_json.testHour }}";
    testHourConfig["command_template"] = "{\"testHour\":{{ value }}}";
    testHourConfig["min"] = 0;
    testHourConfig["max"] = 23;
    testHourConfig["step"] = 1;
    testHourConfig["unit_of_measurement"] = "h";
    testHourConfig["icon"] = "mdi:clock-time-one";
    device = testHourConfig.createNestedObject("device");
    JsonArray testHourIdentifiers = device.createNestedArray("identifiers");
    testHourIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String testHourTopic = discoveryPrefix + "/number/" + deviceId + "_test_hour/config";
    String testHourJson;
    serializeJson(testHourConfig, testHourJson);
    mqttClient.publish(testHourTopic.c_str(), testHourJson.c_str(), true);
    
    // Number: Minuto di test (0-59)
    DynamicJsonDocument testMinuteConfig(512);
    testMinuteConfig["name"] = mqttDeviceName + " Test Minute";
    testMinuteConfig["unique_id"] = deviceId + "_test_minute";
    testMinuteConfig["state_topic"] = mqttBaseTopic + "/state";
    testMinuteConfig["command_topic"] = mqttBaseTopic + "/command";
    testMinuteConfig["value_template"] = "{{ value_json.testMinute }}";
    testMinuteConfig["command_template"] = "{\"testMinute\":{{ value }}}";
    testMinuteConfig["min"] = 0;
    testMinuteConfig["max"] = 59;
    testMinuteConfig["step"] = 1;
    testMinuteConfig["unit_of_measurement"] = "min";
    testMinuteConfig["icon"] = "mdi:clock-time-two";
    device = testMinuteConfig.createNestedObject("device");
    JsonArray testMinuteIdentifiers = device.createNestedArray("identifiers");
    testMinuteIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String testMinuteTopic = discoveryPrefix + "/number/" + deviceId + "_test_minute/config";
    String testMinuteJson;
    serializeJson(testMinuteConfig, testMinuteJson);
    mqttClient.publish(testMinuteTopic.c_str(), testMinuteJson.c_str(), true);
    
    // Button: Modalità Test (mostra orario personalizzato per 5 secondi)
    DynamicJsonDocument testModeButtonConfig(512);
    testModeButtonConfig["name"] = mqttDeviceName + " Test Mode";
    testModeButtonConfig["unique_id"] = deviceId + "_test_mode";
    testModeButtonConfig["command_topic"] = mqttBaseTopic + "/command";
    // Il payload include testMode:true, e userà i valori di testHour e testMinute già impostati
    testModeButtonConfig["payload_press"] = "{\"testMode\":true}";
    testModeButtonConfig["icon"] = "mdi:clock-check";
    device = testModeButtonConfig.createNestedObject("device");
    JsonArray testIdentifiers = device.createNestedArray("identifiers");
    testIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String testButtonTopic = discoveryPrefix + "/button/" + deviceId + "_test_mode/config";
    String testButtonJson;
    serializeJson(testModeButtonConfig, testButtonJson);
    mqttClient.publish(testButtonTopic.c_str(), testButtonJson.c_str(), true);
    
    // Sensor: Versione Firmware
    DynamicJsonDocument firmwareSensorConfig(512);
    firmwareSensorConfig["name"] = mqttDeviceName + " Firmware Version";
    firmwareSensorConfig["unique_id"] = deviceId + "_firmware_version";
    firmwareSensorConfig["state_topic"] = mqttBaseTopic + "/state";
    firmwareSensorConfig["value_template"] = "{{ value_json.firmwareVersion }}";
    firmwareSensorConfig["icon"] = "mdi:information";
    device = firmwareSensorConfig.createNestedObject("device");
    JsonArray firmwareIdentifiers = device.createNestedArray("identifiers");
    firmwareIdentifiers.add(deviceId);
    device["name"] = mqttDeviceName;
    device["model"] = "OraQuadra2";
    device["manufacturer"] = "Survival Hacking";
    
    String firmwareSensorTopic = discoveryPrefix + "/sensor/" + deviceId + "_firmware_version/config";
    String firmwareSensorJson;
    serializeJson(firmwareSensorConfig, firmwareSensorJson);
    mqttClient.publish(firmwareSensorTopic.c_str(), firmwareSensorJson.c_str(), true);
    
    Serial.println("Configurazione MQTT Discovery pubblicata");
}

// Tenta di riconnettersi al broker MQTT
// Se la connessione riesce, pubblica configurazione Discovery e sottoscrive ai comandi
bool mqttReconnect() {
    if (!mqttEnabled) {
        return false;
    }
    
    Serial.print("Tentativo connessione MQTT a ");
    Serial.print(mqttServer);
    Serial.print(":");
    Serial.println(mqttPort);
    
    // Genera ID client univoco per evitare conflitti
    String clientId = mqttDeviceName + "_" + String(random(0xffff), HEX);
    
    // Tenta connessione con autenticazione
    if (mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPassword.c_str())) {
        Serial.println("MQTT connesso!");
        mqttConnected = true;
        
        // Pubblica stato disponibilità (online)
        String availabilityTopic = mqttBaseTopic + "/availability";
        mqttClient.publish(availabilityTopic.c_str(), "online", true);
        
        // Pubblica configurazione Discovery per Home Assistant
        publishMqttDiscovery();
        
        // Sottoscrivi al topic comandi per ricevere comandi
        String commandTopic = mqttBaseTopic + "/command";
        mqttClient.subscribe(commandTopic.c_str());
        Serial.print("Sottoscritto a: ");
        Serial.println(commandTopic);
        
        // Pubblica stato iniziale del dispositivo
        publishMqttState();
        
        return true;
    } else {
        Serial.print("Connessione MQTT fallita, rc=");
        Serial.println(mqttClient.state());
        mqttConnected = false;
        return false;
    }
}

// Pubblica lo stato corrente del dispositivo su MQTT
// Formato JSON compatibile con Home Assistant
void publishMqttState() {
    if (!mqttConnected || !mqttEnabled) {
        return;
    }
    
    // Crea documento JSON con tutti i parametri di stato
    DynamicJsonDocument doc(1536);
    doc["state"] = displayOff ? "OFF" : "ON";
    doc["mode"] = currentMode;
    doc["preset"] = currentPreset;  // Aggiungi anche il preset
    
    // Aggiungi nome preset come nella web app
    const char* presetNames[] = {
        "Lento Casuale",      // 0
        "Sfumato Blu",        // 1
        "Matrix Verde",       // 2
        "Lento Arancione",    // 3
        "Matrix Blu",         // 4
        "Matrix Giallo",      // 5
        "Sfumato Verde",      // 6
        "Lento Blu",          // 7
        "Matrix Ciano",       // 8
        "Sfumato Rosa",       // 9
        "Veloce Giallo",      // 10
        "Veloce Bianco",      // 11
        "Veloce Ciano",       // 12
        "Effetto TRON",       // 13
        "Effetto TRON 2",     // 14
        "Effetto Moto",       // 15
        "Effetto Galaga",     // 16
        "Effetto Pacman",     // 17
        "Digitale",           // 18
        "Arkanoid",           // 19
        "Natale",             // 20
        "Neve",               // 21
        "Laser",              // 22
        "Fuoco",              // 23
        "Black Hole",         // 24
        "Fuoco Realistico",   // 25
        "Fuochi Artificio",   // 26
        "Drift",              // 27
        "Plasma",             // 28
        "Aurora",             // 29
        "Radar",              // 30
        "Cielo Stellato",     // 31
        "Battito",            // 32
        "Tetris",             // 33
        "Snake",              // 34
        "Sabbia",             // 35
        "Temporale",          // 36
        "Bolle",              // 37
        "Pong",               // 38
        "Lava Lamp"           // 39
    };
    if (currentPreset < NUM_MODES) {
        doc["mode_name"] = presetNames[currentPreset];
        doc["preset_name"] = presetNames[currentPreset];  // Alias per compatibilità
    } else {
        doc["mode_name"] = "Sconosciuto";
        doc["preset_name"] = "Sconosciuto";
    }
    
    // Aggiungi parametri principali
    doc["brightness"] = map(intBrightness, 0, 255, 0, 100);  // Luminosità in percentuale
    doc["color"]["r"] = clockColors.current.r;                // Colore RGB
    doc["color"]["g"] = clockColors.current.g;
    doc["color"]["b"] = clockColors.current.b;
    doc["time"] = String(currentHour) + ":" +                  // Ora corrente
                  (currentMinute < 10 ? "0" : "") + String(currentMinute) + ":" +
                  (currentSecond < 10 ? "0" : "") + String(currentSecond);
    doc["blink"] = currentBlink;                               // Stato lampeggio
    doc["sleepModeEnabled"] = sleepModeEnabled;               // Spegnimento notturno
    doc["randomModeEnabled"] = randomModeEnabled;              // Modalità random
    doc["randomInterval"] = randomChangeInterval / 1000;       // Intervallo in secondi
    doc["firmwareVersion"] = "1.2.5";                          // Versione firmware
    doc["testHour"] = testHour;                                // Ora di test
    doc["testMinute"] = testMinute;                            // Minuto di test
    
    // Aggiungi informazioni per entità aggiuntive
    char dayStart[6], nightStart[6], sleepStart[6];
    snprintf(dayStart, sizeof(dayStart), "%02d:%02d", dayStartHour, dayStartMinute);
    snprintf(nightStart, sizeof(nightStart), "%02d:%02d", nightStartHour, nightStartMinute);
    snprintf(sleepStart, sizeof(sleepStart), "%02d:%02d", sleepStartHour, sleepStartMinute);
    doc["dayStartTime"] = dayStart;
    doc["nightStartTime"] = nightStart;
    doc["sleepStartTime"] = sleepStart;
    doc["dayBrightness"] = map(dayBrightness, 0, 255, 0, 100);
    doc["nightBrightness"] = map(nightBrightness, 0, 255, 0, 100);
    
    // Informazioni sistema e rete
    if (WiFi.status() == WL_CONNECTED) {
        doc["wifiConnected"] = true;
        doc["ipAddress"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
    } else {
        doc["wifiConnected"] = false;
        doc["ipAddress"] = "";
        doc["rssi"] = 0;
    }
    
    // Determina nome colore corrente
    String colorName = "Personalizzato";
    CRGB current = clockColors.current;
    if (current == CRGB::White) colorName = "Bianco";
    else if (current == CRGB::Blue) colorName = "Blu";
    else if (current == CRGB::Red) colorName = "Rosso";
    else if (current == CRGB::Green) colorName = "Verde";
    else if (current == CRGB::Yellow) colorName = "Giallo";
    else if (current == CRGB(255,165,0)) colorName = "Arancione";
    doc["colorName"] = colorName;
    
    // Converti JSON in stringa e pubblica sul topic stato
    String jsonString;
    serializeJson(doc, jsonString);
    
    String stateTopic = mqttBaseTopic + "/state";
    mqttClient.publish(stateTopic.c_str(), jsonString.c_str(), true);
}

// Inizializza client MQTT con server, porta e callback
void setupMQTT() {
    if (!mqttEnabled) {
        Serial.println("MQTT disabilitato");
        return;
    }
    
    // Configura server, porta, callback e dimensione buffer
    mqttClient.setServer(mqttServer.c_str(), mqttPort);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(2048);  // Buffer grande per configurazione Discovery
    
    Serial.println("MQTT configurato");
    Serial.print("Server: ");
    Serial.println(mqttServer);
    Serial.print("Port: ");
    Serial.println(mqttPort);
    Serial.print("Base Topic: ");
    Serial.println(mqttBaseTopic);
}

// Carica configurazione MQTT salvata in EEPROM
void loadMqttConfigFromEEPROM() {
    // Leggi se MQTT è abilitato
    mqttEnabled = (EEPROM.read(EEPROM_MQTT_ENABLED_ADDR) == 1);
    
    // Carica indirizzo server (max 64 caratteri)
    mqttServer = "";
    for (int i = 0; i < 64; i++) {
        char c = EEPROM.read(EEPROM_MQTT_SERVER_ADDR + i);
        if (c == 0 || c == 255) break;
        mqttServer += c;
    }
    if (mqttServer.length() == 0) mqttServer = MQTT_SERVER_DEFAULT;
    
    // Carica porta (salvata come 2 byte)
    uint16_t portLow = EEPROM.read(EEPROM_MQTT_PORT_ADDR);
    uint16_t portHigh = EEPROM.read(EEPROM_MQTT_PORT_ADDR + 1);
    mqttPort = portLow | (portHigh << 8);
    if (mqttPort == 0 || mqttPort > 65535) mqttPort = MQTT_PORT_DEFAULT;
    
    // Carica username (max 32 caratteri)
    mqttUser = "";
    for (int i = 0; i < 32; i++) {
        char c = EEPROM.read(EEPROM_MQTT_USER_ADDR + i);
        if (c == 0 || c == 255) break;
        mqttUser += c;
    }
    if (mqttUser.length() == 0) mqttUser = MQTT_USER_DEFAULT;
    
    // Carica password (max 32 caratteri)
    mqttPassword = "";
    for (int i = 0; i < 32; i++) {
        char c = EEPROM.read(EEPROM_MQTT_PASSWORD_ADDR + i);
        if (c == 0 || c == 255) break;
        mqttPassword += c;
    }
    if (mqttPassword.length() == 0) mqttPassword = MQTT_PASSWORD_DEFAULT;
    
    // Carica topic base (max 32 caratteri)
    mqttBaseTopic = "";
    for (int i = 0; i < 32; i++) {
        char c = EEPROM.read(EEPROM_MQTT_BASE_TOPIC_ADDR + i);
        if (c == 0 || c == 255) break;
        mqttBaseTopic += c;
    }
    if (mqttBaseTopic.length() == 0) mqttBaseTopic = MQTT_BASE_TOPIC_DEFAULT;
    
    // Carica device name (max 32 caratteri)
    mqttDeviceName = "";
    for (int i = 0; i < 32; i++) {
        char c = EEPROM.read(EEPROM_MQTT_DEVICE_NAME_ADDR + i);
        if (c == 0 || c == 255) break;
        mqttDeviceName += c;
    }
    if (mqttDeviceName.length() == 0) mqttDeviceName = MQTT_DEVICE_NAME_DEFAULT;
    
    Serial.println("Configurazione MQTT caricata da EEPROM:");
    Serial.print("  Abilitato: ");
    Serial.println(mqttEnabled ? "Sì" : "No");
    Serial.print("  Server: ");
    Serial.println(mqttServer);
    Serial.print("  Porta: ");
    Serial.println(mqttPort);
    Serial.print("  User: ");
    Serial.println(mqttUser);
    Serial.print("  Base Topic: ");
    Serial.println(mqttBaseTopic);
    Serial.print("  Device Name: ");
    Serial.println(mqttDeviceName);
}

// Salva configurazione MQTT in EEPROM per persistenza
void saveMqttConfigToEEPROM() {
    // Salva stato abilitato/disabilitato
    EEPROM.write(EEPROM_MQTT_ENABLED_ADDR, mqttEnabled ? 1 : 0);
    
    // Salva indirizzo server (max 64 caratteri)
    for (int i = 0; i < 64; i++) {
        if (i < mqttServer.length()) {
            EEPROM.write(EEPROM_MQTT_SERVER_ADDR + i, mqttServer[i]);
        } else {
            EEPROM.write(EEPROM_MQTT_SERVER_ADDR + i, 0);
        }
    }
    
    // Salva porta (come 2 byte: low e high)
    EEPROM.write(EEPROM_MQTT_PORT_ADDR, mqttPort & 0xFF);
    EEPROM.write(EEPROM_MQTT_PORT_ADDR + 1, (mqttPort >> 8) & 0xFF);
    
    // Salva username (max 32 caratteri)
    for (int i = 0; i < 32; i++) {
        if (i < mqttUser.length()) {
            EEPROM.write(EEPROM_MQTT_USER_ADDR + i, mqttUser[i]);
        } else {
            EEPROM.write(EEPROM_MQTT_USER_ADDR + i, 0);
        }
    }
    
    // Salva password (max 32 caratteri)
    for (int i = 0; i < 32; i++) {
        if (i < mqttPassword.length()) {
            EEPROM.write(EEPROM_MQTT_PASSWORD_ADDR + i, mqttPassword[i]);
        } else {
            EEPROM.write(EEPROM_MQTT_PASSWORD_ADDR + i, 0);
        }
    }
    
    // Salva base topic (max 32 caratteri)
    for (int i = 0; i < 32; i++) {
        if (i < mqttBaseTopic.length()) {
            EEPROM.write(EEPROM_MQTT_BASE_TOPIC_ADDR + i, mqttBaseTopic[i]);
        } else {
            EEPROM.write(EEPROM_MQTT_BASE_TOPIC_ADDR + i, 0);
        }
    }
    
    // Salva device name (max 32 caratteri)
    for (int i = 0; i < 32; i++) {
        if (i < mqttDeviceName.length()) {
            EEPROM.write(EEPROM_MQTT_DEVICE_NAME_ADDR + i, mqttDeviceName[i]);
        } else {
            EEPROM.write(EEPROM_MQTT_DEVICE_NAME_ADDR + i, 0);
        }
    }
    
    EEPROM.commit();
    Serial.println("Configurazione MQTT salvata in EEPROM");
}


#endif // ENABLE_MQTT

// ============================================================================
//  HOOK per setup() e loop()  -  da chiamare dal .ino
// ============================================================================

// Registra le route web aggiuntive (config MQTT + random/test). Chiamare in setup().
void setupMqttWebRoutes() {
    server.on("/setRandomMode",     HTTP_POST, handleSetRandomMode);
    server.on("/setRandomInterval", HTTP_POST, handleSetRandomInterval);
#if ENABLE_MQTT
    server.on("/getMqttStatus",     HTTP_GET,  handleGetMqttStatus);
#endif
}

// Task periodici da chiamare a ogni giro di loop(). Gestisce:
//  - mantenimento connessione MQTT + pubblicazione stato
//  - timeout modalita test (5s)
//  - cambio automatico in modalita random
void mqttPeriodicTasks(uint32_t currentMillis) {
#if ENABLE_MQTT
    if (mqttEnabled) {
        if (!mqttClient.connected()) {
            mqttConnected = false;
            if (currentMillis - lastMqttCheck > 5000) {
                mqttReconnect();
                lastMqttCheck = currentMillis;
            }
        } else {
            mqttClient.loop();
            if (!mqttConnected) {
                mqttConnected = true;
                Serial.println("MQTT riconnesso!");
                publishMqttDiscovery();
            }
        }
        if (mqttConnected && currentMillis - lastMqttPublish > 30000) {
            publishMqttState();
            lastMqttPublish = currentMillis;
        }
    }
#endif

    // --- Timeout modalita test (mostra orario fittizio per 5 secondi) ---
    if (testModeEnabled) {
        if (currentMillis - testModeStartTime >= 5000) {
            testModeEnabled = false;
            EEPROM.write(EEPROM_TEST_MODE_ADDR, 0);
            EEPROM.commit();
            updateDisplay();
#if ENABLE_MQTT
            publishMqttState();
#endif
            Serial.println("Modalita test disattivata, tornato all'orario reale");
        }
    }

    // --- Modalita random: cambia preset allo scadere dell'intervallo ---
    if (randomModeEnabled && !displayOff && !testModeEnabled) {
        if (currentMillis - lastRandomChange >= randomChangeInterval) {
            uint8_t newPreset = random(NUM_MODES);
            int attempts = 0;
            while (newPreset == currentPreset && attempts < 10) {
                newPreset = random(NUM_MODES);
                attempts++;
            }
            currentPreset = newPreset;
            applyPreset(currentPreset);
            EEPROM.write(EEPROM_PRESET_ADDR, currentPreset);
            EEPROM.commit();
            lastRandomChange = currentMillis;
#if ENABLE_MQTT
            publishMqttState();
#endif
            Serial.print("Modalita random cambiata a preset: ");
            Serial.println(currentPreset);
        }
    }
}
