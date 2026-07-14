# Changelog

Tutte le modifiche rilevanti a questo progetto vengono annotate qui.
Formato basato su [Keep a Changelog](https://keepachangelog.com/it/1.1.0/).
Le modifiche riguardano la versione di sviluppo `Oraquadra2_1_3_1_MQTT/`
(V1.3.0 ufficiale + integrazione MQTT/Home Assistant), salvo diversa indicazione.

Branch di lavoro: `mqtt-ha-v130-extra`.

## [1.3.1+MQTT] — 2026-07-14

Versione **1.3.1-mqtt** (base upstream V1.3.0). Cartella rinominata da
`Oraquadra2_1_3_0_MQTT/` a `Oraquadra2_1_3_1_MQTT/` (sketch incluso).
Compilare con core ESP32-C3, Partition Scheme **Minimal SPIFFS**,
librerie **PubSubClient** + **ArduinoJson**.

### Fixed — 2026-07-14 — audit MQTT/Home Assistant

Tutte in `Oraquadra2_1_3_1_MQTT/mqtt_oraquadra.h` (salvo indicazione):

- **Select colore**: aggiunta l'opzione "Personalizzato" alle `options` del
  discovery (+ ramo `else` → `{}` nel `command_template`). Con un colore RGB
  non predefinito HA loggava `Invalid option for select …: 'Personalizzato'`
  ogni 30 s. Buffer del config 1024 → 1536 byte.
- **Luce HA — `brightness_scale: 100`**: lo stato pubblica la luminosità in
  0-100 ma HA assumeva 0-255 (slider a ~39% con luminosità piena, comandi
  dallo slider interpretati male).
- **Luce HA — rimosso `retain: true`**: HA pubblicava i *comandi* retained su
  `/command` e il firmware ri-riceveva l'ultimo comando vecchio a ogni
  riconnessione (retained residuo sul broker già ripulito).
- **Persistenza modalità random**: on/off e intervallo ora vengono riletti al
  boot (`loadMqttConfigFromEEPROM`); prima erano salvati ma mai ricaricati.
  Intervallo salvato in EEPROM in **secondi** (il vecchio formato in ms
  troncava a 16 bit i valori > 65 s); vecchi valori scartati, fallback 30 s.
- **Sensore Blink**: rimosso `device_class: enum` (richiede `options` e stato
  testuale; il valore è 0/1 numerico).
- **Compilazione con `ENABLE_MQTT false`**: gestori web random spostati fuori
  da `#if ENABLE_MQTT` (le route sono registrate incondizionatamente).
- Commenti corretti: preset 0-40, area EEPROM estesa reale 360-464 (`.ino`).

### Added — nuove funzionalità

- **Effetto "Albero di Natale"** portato dalla V1.2.6 alla V1.3.0 come nuovo
  `MODE_TREE` / preset **40 "Albero di Natale"** (albero verde, 12 palline
  lampeggianti, stella in breathing, senza orario). Aggiunto senza rimuovere
  nulla: l'effetto "Goccia" resta intatto. Collegato al select, al dropdown della
  web app e ai nomi preset.
- **Switch dedicato "Albero di Natale"**: nuova entità switch MQTT
  (`mdi:pine-tree`) + toggle nella web app + route web `/setTreeMode`.
- **Sensori diagnostici MQTT** (5 nuove entità + campi nel JSON di stato):
  CPU Temperature, Free Memory, Memory Usage, Uptime, CPU Usage.
- **Controlli V1.3.0 esposti via MQTT**: Testo Scorrevole (switch + testo +
  velocità) e Overlay Digitale (switch), agganciati alle variabili reali del
  firmware riusando la stessa logica EEPROM dei comandi web.

### Changed

- Stringa firmware aggiornata da "1.2.5" a "1.3.1-mqtt" (sensore HA,
  `sw_version` del device, pagina web).

### 2026-06-30 — MQTT robusto dopo erase + config web ripristinata

- **MQTT si auto-abilita su EEPROM vergine** (`loadMqttConfigFromEEPROM`): dopo un
  "Erase All Flash" il flag MQTT veniva letto come disabilitato (byte 0xFF), così
  il firmware non si connetteva al broker e Home Assistant mostrava i vecchi
  messaggi retained (firmware 1.2.5/1.2.6, sensori "Sconosciuto"). Ora, se l'EEPROM
  è vergine, MQTT parte abilitato con i default e li persiste.
- **Configurazione MQTT dal web ripristinata**: route `/setMqttSettings`
  re-registrata (`setupMqttWebRoutes`) — il gestore esisteva ma non era più
  registrato dopo il port su V1.3.0 — e **nuovo pannello MQTT in `webpage.h`**
  (broker, porta, utente, password, base topic, nome, abilita + "Salva e
  riconnetti"). La password "***" lascia invariata quella salvata. Così si cambia
  il broker dal browser senza ricompilare.
- **Versione firmware mostrata nella pagina web** (`webpage.h`): aggiunta riga
  "Firmware 1.3.0-mqtt" sotto il titolo (da tenere allineata a `firmwareVersion`).

### 2026-06-30 — Migliorie Home Assistant

Tutte in `Oraquadra2_1_3_1_MQTT/mqtt_oraquadra.h`.

- **Effetti integrati nella luce**: i 41 preset esposti come `effect_list`
  dell'entità *light* (menu effetti nativo nella card HA; `light.turn_on … effect:`).
  Aggiunti array condiviso `kPresetNames` + `presetIndexFromName()` e gestione del
  comando `{"effect":"<nome>"}` nel callback; campo `effect` nello stato.
- **`entity_category: diagnostic`** su CPU Temp/Usage, Free/Memory Usage, Uptime,
  RSSI, IP, WiFi, Firmware (raggruppati nella sezione Diagnostica di HA).
- **`expire_after: 90`** sui sensori: vanno *non disponibili* se il device tace
  (si combina col Last Will per uno stato online/offline affidabile).
- **`configuration_url`** nel blocco device = `http://<ip>` (link alla web UI dalla
  scheda dispositivo HA).
- **Nuovi sensori**: *WiFi Quality* (% da RSSI), *Consumo Stimato* (W) e *Corrente
  Stimata* (A) dalla somma luminosità dei 256 LED, *Fascia Attiva* (Giorno/Notte/Sleep).
- Aggiunto file `HA_automazioni_oraquadra.yaml` (notifiche su testo scorrevole,
  Albero di Natale automatico, dimming, sleep, allerta offline).

### 2026-06-30 — Fix blocco USB CDC (matrice spenta senza PC)

- **Fix: scritture seriali non bloccanti** in
  `Oraquadra2_1_3_1_MQTT/Oraquadra2_1_3_1_MQTT.ino`. Aggiunto
  `Serial.setTxTimeoutMs(0)` in `setup()` (sotto `#if ARDUINO_USB_CDC_ON_BOOT`).
  Con `USB CDC On Boot: Enabled` su ESP32-C3, le numerose `Serial.print()` si
  bloccavano quando il dispositivo era alimentato senza un PC collegato (buffer
  USB pieno e mai svuotato): il firmware si piantava e la matrice restava spenta
  finché non si apriva il monitor seriale. In alternativa, impostare
  `USB CDC On Boot: Disabled` in Arduino IDE.

### 2026-06-30 — Affidabilità MQTT e durata hardware

Tre interventi localizzati in `Oraquadra2_1_3_1_MQTT/mqtt_oraquadra.h`:

- **Fix (durata hardware): anti-usura flash in modalità random.**
  Nel ciclo random (`mqttPeriodicTasks`) il preset non viene più salvato in
  EEPROM a ogni cambio. Con l'intervallo minimo di 5 secondi la scrittura
  continua usurava la flash emulata dell'ESP32-C3. Il preset random resta in RAM;
  al riavvio si ripristina l'ultimo preset scelto manualmente.

- **Fix (Home Assistant): aggiunto Last Will & Testament (LWT) MQTT.**
  `mqttReconnect()` ora si connette pubblicando un will
  `availability = "offline"` (QoS 1, retained). Se l'orologio cade per mancanza
  di corrente o WiFi senza disconnessione pulita, il broker pubblica "offline" e
  Home Assistant marca le entità come non disponibili, invece di mostrarle ancora
  online.

- **Fix (MQTT): buffer PubSubClient portato da 2048 a 4096 byte** (`setupMQTT`).
  Il JSON di stato (documento da 2560 byte) con diagnostica e testo scorrevole
  poteva avvicinarsi o superare i 2048 byte; oltre la dimensione del buffer
  PubSubClient scarta il messaggio in silenzio, causando aggiornamenti mancanti
  in Home Assistant.

### Note tecniche

- **CPU Usage è una stima**: l'ESP32 non espone un'API diretta; la percentuale è
  ricavata dal rate del loop.
- **Testo scorrevole via MQTT**: gli accenti (à/è/ì…) non vengono convertiti come
  nel pannello web (filtro ASCII). Per testo normale va bene; è l'unico punto da
  rifinire se serve.

---

## Idee proposte (non ancora implementate)

Backlog dei miglioramenti suggeriti, in ordine di impatto:

- **Home Assistant:** marcare i sensori diagnostici come `entity_category: diagnostic`,
  non usare `retain` sulla telemetria ad alta frequenza, aggiungere `expire_after`
  e `configuration_url` nel blocco `device` (link alla web UI da HA).
- **Uso quotidiano:** sensore di luce ambientale (LDR su ADC o BH1750 I²C) per
  luminosità automatica reale, oltre alle 3 fasce orarie fisse.
- **Web/grafica:** anteprima live della matrice 16×16 (canvas), passaggio da
  polling a WebSocket/SSE, manifest PWA (installabile su telefono), tema chiaro/auto
  (`prefers-color-scheme`).
- **Sicurezza:** autenticazione opzionale sul web server.

[1.3.1+MQTT]: https://github.com/lollox80/Oraquadra2/compare/main...mqtt-ha-v130-extra
