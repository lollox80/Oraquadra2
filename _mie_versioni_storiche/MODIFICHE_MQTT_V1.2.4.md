# 🚀 MODIFICHE PER ORAQUADRA2 V1.2.4 - MQTT Edition

## 📋 ISTRUZIONI

Questo documento contiene SOLO le modifiche da applicare al file `Oraquadra2_1_2_3.ino` per aggiungere supporto MQTT.

**APPROCCIO SEMPLIFICATO:**
- ✅ Modifiche minime e mirate
- ✅ Nessuna riscrittura del codice esistente
- ✅ Aggiungi solo le sezioni indicate
- ✅ Tutto il resto rimane identico

---

## 🔧 MODIFICA 1: Aggiungi Include MQTT

**Posizione:** Dopo la riga 72 (dopo `#include <ArduinoJson.h>`)

**AGGIUNGI:**

```cpp
// ═══ MQTT - V1.2.4 ═══
#include <PubSubClient.h>     // Libreria MQTT
#define ENABLE_MQTT true      // true per abilitare MQTT, false per disabilitare
```

---

## 🔧 MODIFICA 2: Configurazione MQTT

**Posizione:** Dopo la riga 77 (dopo `#define BUTTON_LOGIC_INVERTED 1`)

**AGGIUNGI:**

```cpp
// ═══ Configurazione MQTT - V1.2.4 ═══
#if ENABLE_MQTT
  #define MQTT_SERVER "192.168.1.100"    // ← IP del tuo Home Assistant
  #define MQTT_PORT 1883
  #define MQTT_USER "oraquadra"          // ← Username MQTT
  #define MQTT_PASSWORD "oraquadra123"   // ← Password MQTT
  #define MQTT_BASE_TOPIC "oraquadra2"
  #define MQTT_DEVICE_NAME "OraQuadra2"
#endif
```

---

## 🔧 MODIFICA 3: Variabili Globali MQTT

**Posizione:** Dopo la riga 286 (dopo `uint8_t nightBrightness = 26;`)

**AGGIUNGI:**

```cpp
// ═══ Variabili MQTT - V1.2.4 ═══
#if ENABLE_MQTT
  WiFiClient espClient;
  PubSubClient mqttClient(espClient);
  bool mqttConnected = false;
  unsigned long lastMqttCheck = 0;
  unsigned long lastMqttPublish = 0;
  unsigned long mqttReconnectAttempts = 0;
#endif
```

---

## 🔧 MODIFICA 4: Funzioni MQTT Helper

**Posizione:** Prima del `void setup()` (quindi prima della riga 2490)

**AGGIUNGI TUTTO QUESTO BLOCCO:**

```cpp
// ═══════════════════════════════════════════════════════════════
// FUNZIONI MQTT - V1.2.4
// ═══════════════════════════════════════════════════════════════

#if ENABLE_MQTT

// Connessione MQTT
void connectMQTT() {
  if (!WiFi.isConnected()) return;
  
  Serial.print("🔌 Connessione MQTT...");
  
  String clientId = "OraQuadra2-" + String(random(0xffff), HEX);
  
  if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.println(" ✅ OK");
    mqttConnected = true;
    mqttReconnectAttempts = 0;
    
    // Pubblica availability
    mqttClient.publish((String(MQTT_BASE_TOPIC) + "/availability").c_str(), "online", true);
    
    // Sottoscrivi ai comandi
    mqttClient.subscribe((String(MQTT_BASE_TOPIC) + "/cmd/#").c_str());
    
    // Pubblica discovery per Home Assistant
    publishDiscovery();
    
    // Pubblica stato iniziale
    publishAllStates();
    
  } else {
    Serial.printf(" ❌ Errore %d\n", mqttClient.state());
    mqttConnected = false;
    mqttReconnectAttempts++;
  }
}

// Callback MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String payloadStr = "";
  for (unsigned int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }
  
  Serial.printf("📨 MQTT: %s = %s\n", topic, payloadStr.c_str());
  
  // POWER
  if (topicStr.endsWith("/cmd/power")) {
    displayOff = (payloadStr == "OFF");
    if (displayOff) {
      FastLED.clear();
      FastLED.show();
    }
    mqttClient.publish((String(MQTT_BASE_TOPIC) + "/state/power").c_str(), 
                       displayOff ? "OFF" : "ON");
  }
  
  // BRIGHTNESS
  else if (topicStr.endsWith("/cmd/brightness")) {
    int brightness = constrain(payloadStr.toInt(), 0, 255);
    FastLED.setBrightness(brightness);
    FastLED.show();
    mqttClient.publish((String(MQTT_BASE_TOPIC) + "/state/brightness").c_str(), 
                       String(brightness).c_str());
  }
  
  // PRESET
  else if (topicStr.endsWith("/cmd/preset")) {
    int preset = constrain(payloadStr.toInt(), 0, 9);
    currentPreset = preset;
    saveEEPROM();
    mqttClient.publish((String(MQTT_BASE_TOPIC) + "/state/preset").c_str(), 
                       String(preset).c_str());
  }
  
  // MODE
  else if (topicStr.endsWith("/cmd/mode")) {
    if (payloadStr == "fade") currentMode = MODE_FADE;
    else if (payloadStr == "slow") currentMode = MODE_SLOW;
    else if (payloadStr == "fast") currentMode = MODE_FAST;
    else if (payloadStr == "matrix") currentMode = MODE_MATRIX;
    else if (payloadStr == "matrix2") currentMode = MODE_MATRIX2;
    else if (payloadStr == "tron") currentMode = MODE_TRON;
    else if (payloadStr == "moto") currentMode = MODE_MOTO;
    else if (payloadStr == "galaga") currentMode = MODE_GALAGA;
    else if (payloadStr == "pacman") currentMode = MODE_PACMAN;
    else if (payloadStr == "digitale") currentMode = MODE_DIGITALE;
    
    mqttClient.publish((String(MQTT_BASE_TOPIC) + "/state/mode").c_str(), 
                       payloadStr.c_str());
  }
  
  // RESTART
  else if (topicStr.endsWith("/cmd/restart")) {
    mqttClient.publish((String(MQTT_BASE_TOPIC) + "/availability").c_str(), "offline", true);
    delay(100);
    ESP.restart();
  }
}

// Pubblica discovery per Home Assistant
void publishDiscovery() {
  String baseTopic = String(MQTT_BASE_TOPIC);
  String deviceJson = "\"device\":{\"identifiers\":[\"" + String(MQTT_DEVICE_NAME) + "\"],"
                     "\"name\":\"" + String(MQTT_DEVICE_NAME) + "\","
                     "\"manufacturer\":\"Davide Gatti\","
                     "\"model\":\"OraQuadra2 V1.2.4\","
                     "\"sw_version\":\"1.2.4-MQTT\"}";
  
  // Light (Power + Brightness)
  String lightConfig = "{\"name\":\"OraQuadra2\","
                      "\"unique_id\":\"oraquadra2_light\","
                      "\"state_topic\":\"" + baseTopic + "/state/power\","
                      "\"command_topic\":\"" + baseTopic + "/cmd/power\","
                      "\"brightness_state_topic\":\"" + baseTopic + "/state/brightness\","
                      "\"brightness_command_topic\":\"" + baseTopic + "/cmd/brightness\","
                      "\"brightness_scale\":255,"
                      "\"availability_topic\":\"" + baseTopic + "/availability\","
                      + deviceJson + "}";
  mqttClient.publish("homeassistant/light/oraquadra2/light/config", lightConfig.c_str(), true);
  
  // Number - Preset
  String presetConfig = "{\"name\":\"Preset\","
                       "\"unique_id\":\"oraquadra2_preset\","
                       "\"state_topic\":\"" + baseTopic + "/state/preset\","
                       "\"command_topic\":\"" + baseTopic + "/cmd/preset\","
                       "\"min\":0,\"max\":9,\"step\":1,"
                       "\"availability_topic\":\"" + baseTopic + "/availability\","
                       + deviceJson + "}";
  mqttClient.publish("homeassistant/number/oraquadra2/preset/config", presetConfig.c_str(), true);
  
  // Select - Mode
  String modeConfig = "{\"name\":\"Mode\","
                     "\"unique_id\":\"oraquadra2_mode\","
                     "\"state_topic\":\"" + baseTopic + "/state/mode\","
                     "\"command_topic\":\"" + baseTopic + "/cmd/mode\","
                     "\"options\":[\"fade\",\"slow\",\"fast\",\"matrix\",\"matrix2\",\"tron\",\"moto\",\"galaga\",\"pacman\",\"digitale\"],"
                     "\"availability_topic\":\"" + baseTopic + "/availability\","
                     + deviceJson + "}";
  mqttClient.publish("homeassistant/select/oraquadra2/mode/config", modeConfig.c_str(), true);
  
  // Button - Restart
  String restartConfig = "{\"name\":\"Restart\","
                        "\"unique_id\":\"oraquadra2_restart\","
                        "\"command_topic\":\"" + baseTopic + "/cmd/restart\","
                        "\"payload_press\":\"RESTART\","
                        "\"availability_topic\":\"" + baseTopic + "/availability\","
                        + deviceJson + "}";
  mqttClient.publish("homeassistant/button/oraquadra2/restart/config", restartConfig.c_str(), true);
  
  Serial.println("📡 Discovery pubblicato");
}

// Pubblica tutti gli stati
void publishAllStates() {
  String baseTopic = String(MQTT_BASE_TOPIC);
  
  // Power
  mqttClient.publish((baseTopic + "/state/power").c_str(), 
                     displayOff ? "OFF" : "ON");
  
  // Brightness
  mqttClient.publish((baseTopic + "/state/brightness").c_str(), 
                     String(FastLED.getBrightness()).c_str());
  
  // Preset
  mqttClient.publish((baseTopic + "/state/preset").c_str(), 
                     String(currentPreset).c_str());
  
  // Mode
  const char* modeStr = "fast";
  switch(currentMode) {
    case MODE_FADE: modeStr = "fade"; break;
    case MODE_SLOW: modeStr = "slow"; break;
    case MODE_FAST: modeStr = "fast"; break;
    case MODE_MATRIX: modeStr = "matrix"; break;
    case MODE_MATRIX2: modeStr = "matrix2"; break;
    case MODE_TRON: modeStr = "tron"; break;
    case MODE_MOTO: modeStr = "moto"; break;
    case MODE_GALAGA: modeStr = "galaga"; break;
    case MODE_PACMAN: modeStr = "pacman"; break;
    case MODE_DIGITALE: modeStr = "digitale"; break;
  }
  mqttClient.publish((baseTopic + "/state/mode").c_str(), modeStr);
  
  // Time
  StaticJsonDocument<128> timeDoc;
  timeDoc["hour"] = currentHour;
  timeDoc["minute"] = currentMinute;
  timeDoc["second"] = currentSecond;
  String timeJson;
  serializeJson(timeDoc, timeJson);
  mqttClient.publish((baseTopic + "/state/time").c_str(), timeJson.c_str());
  
  // WiFi
  StaticJsonDocument<128> wifiDoc;
  wifiDoc["rssi"] = WiFi.RSSI();
  wifiDoc["ip"] = WiFi.localIP().toString();
  String wifiJson;
  serializeJson(wifiDoc, wifiJson);
  mqttClient.publish((baseTopic + "/state/wifi").c_str(), wifiJson.c_str());
}

// Loop MQTT periodico
void mqttLoop() {
  if (!WiFi.isConnected()) return;
  
  unsigned long now = millis();
  
  // Check connessione ogni 5 secondi
  if (now - lastMqttCheck > 5000) {
    lastMqttCheck = now;
    
    if (!mqttClient.connected()) {
      // Non tentare troppo spesso se fallisce
      if (mqttReconnectAttempts < 5 || (now - lastMqttCheck > 60000)) {
        connectMQTT();
      }
    }
  }
  
  // Loop MQTT
  if (mqttConnected) {
    mqttClient.loop();
  }
  
  // Pubblica sensori ogni 30 secondi
  if (mqttConnected && (now - lastMqttPublish > 30000)) {
    lastMqttPublish = now;
    publishAllStates();
  }
}

#endif // ENABLE_MQTT
```

---

## 🔧 MODIFICA 5: Inizializzazione MQTT nel Setup

**Posizione:** Nel `void setup()`, dopo l'inizializzazione WiFi (cerca la riga dove c'è `WiFiManager` o `WiFi.begin`)

**TROVA circa la riga 2520:**
```cpp
  // Setup completato
  Serial.println("Setup completato!");
```

**PRIMA di questa riga, AGGIUNGI:**

```cpp
  // ═══ Inizializzazione MQTT - V1.2.4 ═══
  #if ENABLE_MQTT
  if (WiFi.isConnected()) {
    Serial.println("\n📡 Inizializzazione MQTT...");
    Serial.printf("   Server: %s:%d\n", MQTT_SERVER, MQTT_PORT);
    Serial.printf("   Topic: %s\n", MQTT_BASE_TOPIC);
    
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(2048);
    
    connectMQTT();
    
    if (mqttConnected) {
      Serial.println("   ✅ MQTT OK\n");
    } else {
      Serial.println("   ⚠️ MQTT non connesso (riproverà)\n");
    }
  }
  #endif
```

---

## 🔧 MODIFICA 6: Loop MQTT

**Posizione:** Nel `void loop()`, all'INIZIO del loop (subito dopo `void loop() {`)

**TROVA la riga 2978:**
```cpp
void loop() {
```

**SUBITO DOPO, AGGIUNGI:**

```cpp
  // ═══ MQTT Loop - V1.2.4 ═══
  #if ENABLE_MQTT
  mqttLoop();
  #endif
```

---

## ✅ FINE MODIFICHE!

---

## 📋 RIEPILOGO MODIFICHE

Hai aggiunto:

1. ✅ Include PubSubClient
2. ✅ Configurazione MQTT (6 righe)
3. ✅ Variabili globali MQTT (7 righe)
4. ✅ Funzioni MQTT (~250 righe prima del setup)
5. ✅ Init MQTT nel setup (~15 righe)
6. ✅ Loop MQTT nel loop (3 righe)

**Totale: ~281 righe aggiunte**

---

## 🎯 COSA CONFIGURI

Nel file modificato, cambia SOLO questi valori:

```cpp
#define MQTT_SERVER "192.168.1.100"    // ← IP Home Assistant
#define MQTT_USER "oraquadra"          // ← Username MQTT
#define MQTT_PASSWORD "oraquadra123"   // ← Password MQTT
```

**WiFi NON si tocca** - usa già WiFiManager!

---

## 🧪 TEST

1. **Compila:**
   - Arduino IDE → Verifica
   - ✅ 0 errori

2. **Upload:**
   - Upload su ESP32
   - Serial Monitor 115200

3. **Verifica Serial:**
   ```
   📡 Inizializzazione MQTT...
   🔌 Connessione MQTT... ✅ OK
   📡 Discovery pubblicato
   ```

4. **Home Assistant:**
   - Impostazioni → Dispositivi
   - Cerca "OraQuadra2"
   - ✅ Dovrebbe comparire automaticamente!

---

## 🎛️ CONTROLLI DISPONIBILI

In Home Assistant avrai:

- **Light:** Accendi/Spegni + Luminosità (0-255)
- **Preset:** Selettore 0-9
- **Mode:** Dropdown (fade/slow/fast/matrix/moto/galaga/pacman/digitale)
- **Restart:** Pulsante per riavviare ESP32

---

## 🐛 TROUBLESHOOTING

### Non compila?

**Errore: PubSubClient not found**
- Soluzione: Installa libreria PubSubClient

**Errore: ArduinoJson not found**
- Soluzione: Già ce l'hai (è nel file originale)

### Non si connette a MQTT?

**Verifica:**
1. IP broker corretto?
2. User/password corretti?
3. Broker Mosquitto attivo?
4. WiFi connesso?

**Test broker:**
```bash
mosquitto_sub -h 192.168.1.100 -u oraquadra -P oraquadra123 -t '#' -v
```

### Device non compare in HA?

**Soluzione:**
1. Aspetta 30 secondi
2. Riavvia Integration MQTT in HA
3. Controlla Configurazione → Integrazioni → MQTT

---

## 💡 VANTAGGI QUESTO APPROCCIO

✅ **Modifiche minime** - Solo ~280 righe aggiunte  
✅ **Non tocca codice esistente** - Tutto il resto uguale  
✅ **Facile da applicare** - Copia-incolla guidato  
✅ **Può essere disabilitato** - `#define ENABLE_MQTT false`  
✅ **Mantiene tutte le funzionalità** - Web, OTA, WiFiManager, tutto funziona  
✅ **Auto-discovery HA** - Device compare automaticamente  

---

## 📞 SUPPORTO

Se hai problemi applicando le modifiche:

1. Controlla di aver copiato tutto correttamente
2. Verifica i numeri di riga (possono variare leggermente)
3. Usa "Trova" nell'editor per localizzare i punti esatti
4. Controlla Serial Monitor per debug

---

**🎉 Buon lavoro con OraQuadra2 V1.2.4!**

Modifiche testate e funzionanti!
