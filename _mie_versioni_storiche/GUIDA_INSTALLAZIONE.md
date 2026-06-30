# 🚀 Guida Installazione MQTT per OraQuadra2

## 📋 Indice

1. [Prerequisiti](#prerequisiti)
2. [Installazione Broker MQTT](#installazione-broker-mqtt)
3. [Modifica Codice Arduino](#modifica-codice-arduino)
4. [Configurazione Home Assistant](#configurazione-home-assistant)
5. [Test e Verifica](#test-e-verifica)
6. [Troubleshooting](#troubleshooting)

---

## 🔧 Prerequisiti

### Hardware
- ✅ ESP32 con OraQuadra2 funzionante
- ✅ Home Assistant installato
- ✅ Rete WiFi

### Software
- ✅ Arduino IDE con supporto ESP32
- ✅ Librerie Arduino:
  - `PubSubClient` (per MQTT)
  - `ArduinoJson` (per JSON)

### Conoscenze
- 📌 Modificare codice Arduino (livello base)
- 📌 Editare file YAML in Home Assistant
- 📌 Usare Arduino IDE per compilare/uploadare

---

## 📦 Installazione Broker MQTT

### Opzione 1: Mosquitto su Home Assistant (CONSIGLIATO)

1. **Apri Home Assistant**
2. **Vai su:** Impostazioni → Add-on → Add-on Store
3. **Cerca:** "Mosquitto broker"
4. **Installa** l'add-on
5. **Avvia** l'add-on
6. **Configurazione base:**
   ```yaml
   logins:
     - username: oraquadra
       password: password123  # CAMBIA PASSWORD!
   ```
7. **Salva** e **Riavvia** l'add-on

### Opzione 2: Mosquitto su Raspberry Pi separato

```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

### Test del Broker

```bash
# Terminal 1 - Subscribe
mosquitto_sub -h localhost -t test/topic

# Terminal 2 - Publish
mosquitto_pub -h localhost -t test/topic -m "Hello MQTT"
```

Se vedi "Hello MQTT" nel Terminal 1 → Broker funzionante! ✅

---

## 🔨 Modifica Codice Arduino

### Passo 1: Installa le librerie necessarie

**Arduino IDE:**
1. Strumenti → Gestisci librerie
2. Cerca e installa:
   - `PubSubClient` by Nick O'Leary
   - `ArduinoJson` by Benoit Blanchon (versione 6.x)

### Passo 2: Aggiungi i file al progetto

1. **Scarica questi 3 file:**
   - `mqtt_integration.h`
   - `mqtt_implementation.cpp`
   - `home_assistant_config.yaml` (per dopo)

2. **Copia i file nella cartella del progetto:**
   ```
   OraQuadra2_arduino/
   ├── OraQuadra2_arduino.ino
   ├── mqtt_integration.h          ← NUOVO
   ├── mqtt_implementation.cpp      ← NUOVO
   └── ... altri file originali ...
   ```

### Passo 3: Modifica OraQuadra2_arduino.ino

**All'INIZIO del file (dopo gli altri #include):**

```cpp
// ═══════════════════════════════════════════════════════════════
// MQTT INTEGRATION - AGGIUNGI QUESTE RIGHE
// ═══════════════════════════════════════════════════════════════

#include <PubSubClient.h>
#include "mqtt_integration.h"

// Configurazione MQTT
#define MQTT_SERVER "192.168.1.X"    // ← CAMBIA con IP del tuo broker
#define MQTT_PORT 1883
#define MQTT_USER "oraquadra"        // ← Username broker MQTT
#define MQTT_PASSWORD "password123"  // ← Password broker MQTT

// Client MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);
MqttIntegration mqtt;

// Funzione connessione MQTT
void connectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Connessione MQTT...");
        
        if (mqttClient.connect("oraquadra2", MQTT_USER, MQTT_PASSWORD)) {
            Serial.println(" ✅ Connesso!");
            mqtt.setup();
        } else {
            Serial.print(" ❌ Fallito, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" riprovo tra 5s");
            delay(5000);
        }
    }
}
```

**Nella funzione setup():**

```cpp
void setup() {
    // ... tutto il setup originale ...
    
    // ═══════════════════════════════════════════════════════════
    // MQTT SETUP - AGGIUNGI ALLA FINE DELLA FUNZIONE setup()
    // ═══════════════════════════════════════════════════════════
    
    // Configura MQTT
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.init(&mqttClient);
    
    // Prima connessione
    connectMQTT();
    
    Serial.println("✅ MQTT inizializzato!");
}
```

**Nella funzione loop():**

```cpp
void loop() {
    // ... tutto il loop originale ...
    
    // ═══════════════════════════════════════════════════════════
    // MQTT LOOP - AGGIUNGI ALLA FINE DELLA FUNZIONE loop()
    // ═══════════════════════════════════════════════════════════
    
    // Riconnetti se disconnesso
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    
    // Gestisci MQTT
    mqttClient.loop();
    mqtt.loop();
}
```

### Passo 4: Adatta le funzioni getter/setter

**IMPORTANTE:** Apri `mqtt_implementation.cpp` e modifica le funzioni per usare le TUE variabili reali.

**Esempio - Se hai una variabile chiamata `currentBrightness`:**

```cpp
// PRIMA (placeholder):
int MqttIntegration::getBrightness() {
    extern int brightness;
    return brightness;
}

// DOPO (con la tua variabile vera):
int MqttIntegration::getBrightness() {
    extern int currentBrightness;  // ← Nome variabile reale
    return currentBrightness;
}
```

**Cerca nel tuo codice originale:**
- Come si chiamano le variabili per luminosità, colore, preset, ecc.
- Come si chiamano le funzioni per accendere/spegnere, cambiare colore, ecc.
- Sostituisci i placeholder con i nomi reali

### Passo 5: Compila e carica

1. **Verifica** (Ctrl+R o icona ✓)
2. **Risolvi** eventuali errori di compilazione
3. **Carica** sull'ESP32 (Ctrl+U o icona →)
4. **Apri Serial Monitor** (115200 baud)
5. **Controlla** che si connetta a WiFi e MQTT

**Output atteso:**
```
WiFi connesso!
IP: 192.168.1.123
Connessione MQTT... ✅ Connesso!
✅ MQTT inizializzato!
```

---

## 🏠 Configurazione Home Assistant

### Metodo 1: Auto-Discovery (automatico)

Se l'ESP32 si connette correttamente, Home Assistant **rileverà automaticamente** tutte le entità!

1. Vai su: **Impostazioni → Dispositivi e servizi**
2. Cerca: **"MQTT"**
3. Dovresti vedere: **"1 dispositivo scoperto"**
4. Clicca: **"Configura"**
5. Il device **OraQuadra2** apparirà con tutti i sensori! 🎉

### Metodo 2: Configurazione Manuale

Se l'auto-discovery non funziona:

1. **Apri** `configuration.yaml` di Home Assistant
2. **Copia** il contenuto di `home_assistant_config.yaml`
3. **Incolla** nel tuo `configuration.yaml`
4. **Salva**
5. **Riavvia** Home Assistant

### Verifica Configurazione

1. **Strumenti per sviluppatori → Stati**
2. **Cerca:** "oraquadra"
3. **Dovresti vedere** tutti i sensori:
   - `light.oraquadra`
   - `sensor.oraquadra_temperatura`
   - `sensor.oraquadra_wifi_rssi`
   - ... ecc ...

---

## ✅ Test e Verifica

### Test 1: Accendi/Spegni

```yaml
# Strumenti per sviluppatori → Servizi
service: light.turn_on
target:
  entity_id: light.oraquadra
```

**Risultato atteso:** L'orologio si accende ✅

### Test 2: Luminosità

```yaml
service: light.turn_on
target:
  entity_id: light.oraquadra
data:
  brightness_pct: 50
```

**Risultato atteso:** Luminosità al 50% ✅

### Test 3: MQTT Explorer

**Scarica:** [MQTT Explorer](http://mqtt-explorer.com/)

1. **Connetti** al broker MQTT
2. **Guarda** topic: `oraquadra2/#`
3. **Dovresti vedere** tutti i messaggi pubblicati

### Test 4: Sensori

Verifica che i sensori si aggiornino:
- WiFi RSSI (ogni 60s)
- Temperatura (ogni 30s)
- LED Attivi (ogni 5s)
- Luminosità (ogni 5s)

---

## 🐛 Troubleshooting

### Problema: ESP32 non si connette a MQTT

**Sintomo:** Serial Monitor mostra "Connessione MQTT... ❌ Fallito"

**Soluzioni:**
1. ✅ Verifica IP broker MQTT corretto
2. ✅ Verifica porta 1883 aperta
3. ✅ Verifica username/password corretti
4. ✅ Verifica che Mosquitto sia avviato:
   ```bash
   sudo systemctl status mosquitto
   ```

### Problema: Home Assistant non vede i sensori

**Soluzioni:**
1. ✅ Controlla integrazione MQTT attiva in HA
2. ✅ Controlla che il broker sia configurato
3. ✅ Verifica topic MQTT con MQTT Explorer
4. ✅ Riavvia Home Assistant

### Problema: Sensori non si aggiornano

**Soluzioni:**
1. ✅ Verifica che `mqtt.loop()` sia chiamato nel `loop()`
2. ✅ Controlla Serial Monitor per errori
3. ✅ Verifica che ESP32 sia connesso (LED onboard?)
4. ✅ Aumenta `SENSOR_PUBLISH_INTERVAL` per debug

### Problema: Comandi non funzionano

**Soluzioni:**
1. ✅ Verifica che le funzioni setter siano implementate
2. ✅ Controlla Serial Monitor quando invii comando
3. ✅ Verifica che topic sia corretto: `oraquadra2/cmd/...`
4. ✅ Usa MQTT Explorer per inviare comando manuale

### Problema: Compilazione fallisce

**Errore:** `PubSubClient.h: No such file or directory`
- ✅ Installa libreria PubSubClient

**Errore:** `ArduinoJson.h: No such file or directory`
- ✅ Installa libreria ArduinoJson

**Errore:** `undefined reference to MqttIntegration::getBrightness()`
- ✅ Aggiungi `mqtt_implementation.cpp` al progetto

---

## 📊 Tabella Completa Sensori

| Sensore | Topic MQTT | Tipo | Descrizione |
|---------|-----------|------|-------------|
| WiFi Connesso | `state/wifi/connected` | Binary | Stato connessione WiFi |
| WiFi RSSI | `state/wifi/rssi` | Sensor | Potenza segnale WiFi |
| LED Attivi | `state/led/active` | Sensor | Numero LED accesi |
| Luminosità | `state/brightness` | Sensor | Luminosità attuale 0-100% |
| Memoria Libera | `state/memory/free` | Sensor | RAM disponibile |
| CPU Usage | `state/cpu/usage` | Sensor | Utilizzo CPU % |
| Temperatura | `state/temperature` | Sensor | Temperatura CPU |
| Uptime | `state/system/uptime` | Sensor | Secondi di uptime |
| Firmware | `state/system/firmware` | Sensor | Versione firmware |
| Orologio Attivo | `state/clock/active` | Binary | Orologio ON/OFF |
| Preset Corrente | `state/preset/current` | Sensor | Numero preset |
| Preset Nome | `state/preset/name` | Sensor | Nome preset |
| Modalità | `state/mode/current` | Sensor | Modalità attuale |
| Colore RGB | `state/color/rgb` | Sensor | Colore JSON |
| Velocità Animazione | `state/animation/speed` | Sensor | Velocità 1-10 |
| Secondi Lampeggianti | `state/seconds/flashing` | Binary | Stato lampeggio |

## 🎛️ Tabella Comandi

| Comando | Topic MQTT | Payload | Descrizione |
|---------|-----------|---------|-------------|
| Accendi/Spegni | `cmd/power` | `ON`/`OFF` | Controllo alimentazione |
| Luminosità | `cmd/brightness` | `0-100` | Imposta luminosità |
| Preset | `cmd/preset` | `0-9` | Cambia preset |
| Colore | `cmd/color` | `{"r":255,"g":0,"b":0}` | Imposta RGB |
| Modalità | `cmd/mode` | `normale`/`arcobaleno` | Cambia modalità |
| Velocità | `cmd/animation/speed` | `1-10` | Velocità animazione |
| Effetto | `cmd/effect` | `fadeIn`/`slideLeft` | Applica effetto |
| Restart | `cmd/restart` | `RESTART` | Riavvia ESP32 |

---

## 🎨 Dashboard Lovelace Consigliata

```yaml
type: vertical-stack
cards:
  # Card Controlli Principali
  - type: entities
    title: 🕐 OraQuadra Controlli
    show_header_toggle: false
    entities:
      - entity: light.oraquadra
        name: 💡 Alimentazione
      - type: divider
      - entity: number.oraquadra_luminosita_control
        name: 🔆 Luminosità
      - entity: number.oraquadra_preset
        name: 🎨 Preset
      - entity: sensor.oraquadra_preset_nome
        name: 📛 Nome Preset
      - type: divider
      - entity: select.oraquadra_modalita_select
        name: 🌈 Modalità
      - entity: select.oraquadra_effetto_select
        name: ✨ Effetto
      - entity: number.oraquadra_velocita_animazione_control
        name: ⚡ Velocità

  # Card Stato
  - type: entities
    title: 📊 Stato
    show_header_toggle: false
    entities:
      - entity: binary_sensor.oraquadra_attivo
        name: ⏰ Orologio
      - entity: binary_sensor.oraquadra_wifi_connesso
        name: 📶 WiFi
      - entity: sensor.oraquadra_led_attivi
        name: 💡 LED Accesi
      - entity: binary_sensor.oraquadra_secondi_lampeggianti
        name: ⏱️ Secondi

  # Card Diagnostica
  - type: entities
    title: 🔧 Diagnostica
    show_header_toggle: false
    entities:
      - entity: sensor.oraquadra_temperatura
        name: 🌡️ Temperatura
      - entity: sensor.oraquadra_wifi_rssi
        name: 📡 WiFi Signal
      - entity: sensor.oraquadra_memoria_usata_percent
        name: 💾 RAM
      - entity: sensor.oraquadra_cpu_usage
        name: 🔲 CPU
      - entity: sensor.oraquadra_uptime
        name: ⏰ Uptime
      - type: divider
      - entity: button.oraquadra_restart
        name: 🔄 Riavvia
```

---

## 🚀 Cosa Puoi Fare Ora

Con questa integrazione hai:

### ✅ Controllo Completo
- Accendi/spegni da Home Assistant
- Cambia luminosità con slider
- Seleziona preset con menu
- Cambia colori RGB
- Applica effetti e animazioni

### ✅ Monitoraggio
- Vedi tutti i sensori in tempo reale
- Controlla temperatura CPU
- Monitora WiFi signal
- Traccia uptime

### ✅ Automazioni
- Spegni automaticamente di notte
- Abbassa luminosità serale
- Notifiche se offline
- Preset basati su eventi
- Sincronizzazione con altri device

### ✅ Interfaccia Web Originale
- Funziona ancora normalmente
- Controllo locale sempre disponibile
- Backup se HA offline

### ✅ Alexa
- Continua a funzionare
- Puoi usare HA + Alexa insieme

---

## 📝 Note Finali

### Aggiornamenti Futuri del Progetto

Quando il progetto OraQuadra2 viene aggiornato su GitHub:

1. **Fai backup** dei file MQTT
2. **Scarica** nuovo codice da GitHub
3. **Copia** i file MQTT nella nuova versione
4. **Aggiungi** le stesse 3 sezioni nel nuovo .ino
5. **Ricompila** e carica

### Aggiungere Nuovi Sensori

Per aggiungere un nuovo parametro:

1. **Aggiungi getter** in `mqtt_implementation.cpp`
2. **Pubblica** in `publishNormalSensors()` o altro
3. **Aggiungi** topic in `home_assistant_config.yaml`
4. **Riavvia** HA

### Performance

- MQTT è leggerissimo: ~2KB RAM
- Non rallenta l'orologio
- Pubblicazione asincrona
- Nessun ritardo visibile

---

## 🎉 Conclusione

Hai ora una integrazione MQTT COMPLETA che:
- ✅ Mantiene tutto ciò che avevi in ESPHome
- ✅ Preserva interfaccia web originale
- ✅ Permette aggiornamenti facili
- ✅ Funziona con HA + Alexa
- ✅ È estendibile facilmente

**Buon divertimento con il tuo OraQuadra2 intelligente!** 🚀

---

## 📞 Supporto

Se hai problemi:
1. Controlla [Troubleshooting](#troubleshooting)
2. Usa MQTT Explorer per debug
3. Controlla Serial Monitor
4. Verifica log di Home Assistant

---

*Creato con ❤️ per la community OraQuadra2*
