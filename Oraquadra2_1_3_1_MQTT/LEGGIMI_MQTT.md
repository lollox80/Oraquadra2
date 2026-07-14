# OraQuadra2 V1.3.1 + MQTT / Home Assistant

Versione **V1.3.0 ufficiale di SurvivalHacking** con l'integrazione **MQTT +
Home Assistant** (il "server collegato") sviluppata da Lollo, ri-portata dalla
sua base V1.2.6 sulla nuova V1.3.0. Versione corrente: **1.3.1-mqtt**.

## Changelog

### V1.3.1+MQTT (14/07/2026) — correzioni MQTT/HA

- **Select colore**: aggiunta l'opzione "Personalizzato" alle options del
  discovery (+ ramo `else` -> `{}` nel command_template). Prima, con un colore
  RGB non predefinito, HA loggava ogni 30 s `Invalid option for select ...:
  'Personalizzato'`. Buffer del config 1024 -> 1536 byte.
- **Luce HA — `brightness_scale: 100`**: lo stato pubblica la luminosita' in
  0-100 ma HA assumeva scala 0-255 (slider a ~39% con luminosita' piena,
  comandi slider interpretati male dal firmware).
- **Luce HA — rimosso `retain: true`**: faceva pubblicare a HA i *comandi*
  retained su `/command`; a ogni riconnessione il firmware ri-riceveva l'ultimo
  comando vecchio. (Retained residuo sul broker gia' ripulito.)
- **Persistenza modalita' random**: on/off e intervallo ora vengono **riletti
  al boot** in `loadMqttConfigFromEEPROM()` (prima erano salvati ma mai
  ricaricati). L'intervallo e' salvato in EEPROM in **secondi** (il vecchio
  formato in ms troncava a 16 bit i valori > 65 s); i valori del vecchio
  formato vengono scartati con fallback a 30 s.
- **Sensore Blink**: rimosso `device_class: enum` (richiedeva `options` e stato
  testuale, ma il valore e' 0/1 numerico).
- **Fix compilazione `ENABLE_MQTT false`**: i gestori web della modalita'
  random sono stati spostati fuori dal blocco `#if ENABLE_MQTT` (le route sono
  registrate sempre da `setupMqttWebRoutes()`).
- Versione firmware: `1.3.1-mqtt` (sensore HA, `sw_version`, pagina web).

### V1.3.0-mqtt e precedenti

Vedi le sezioni sotto (modifiche affidabilita' 30/06/2026, extra V1.3.0) e lo
storico in testa allo sketch `.ino`.

## Modifiche affidabilita (30/06/2026)

Tre interventi in `mqtt_oraquadra.h`, tutti localizzati:

- **Anti-usura flash (modalita random)**: nel ciclo random non si salva piu' il
  preset in EEPROM a ogni cambio (con intervallo minimo di 5s usurava la flash
  emulata). Il preset resta in RAM; al riavvio si ripristina l'ultimo preset
  scelto manualmente. (`mqttPeriodicTasks`)
- **Last Will MQTT (LWT)**: `mqttReconnect()` ora si connette con will
  `availability = "offline"` (QoS 1, retained). Se l'orologio cade senza chiusura
  pulita, il broker pubblica "offline" e Home Assistant marca le entita' come non
  disponibili invece di mostrarle ancora online.
- **Buffer MQTT a 4096** (era 2048): il JSON di stato con diagnostica + testo
  scorrevole poteva avvicinarsi/superare i 2048 byte; oltre il buffer
  PubSubClient scarta il messaggio in silenzio. (`setupMQTT`)

## File di questa cartella

- `Oraquadra2_1_3_1_MQTT.ino` — sketch V1.3.0 con 7 innesti minimi per l'MQTT.
- `webpage.h` — pagina web V1.3.0 (invariata).
- `mqtt_oraquadra.h` — **tutto** il codice MQTT + modalita random + modalita test
  (config, callback comandi, auto-discovery, stato, riconnessione, hook di loop).

## Cosa fa

- **Configurazione broker**: di default usa i tuoi parametri reali
  (`192.168.1.100`, utente `mqtt`). Si cambiano in cima a `mqtt_oraquadra.h`
  (`MQTT_*_DEFAULT`) oppure via le route web `/setMqttSettings` e `/getMqttStatus`
  (gia' registrate; vengono anche salvate in EEPROM).
- **Home Assistant auto-discovery**: luce (on/off + luminosita + colore RGB),
  selettore effetto con **tutti i 40 preset** della V1.3.0 (inclusi i nuovi
  Arkanoid..Lava Lamp), sensori (ora, WiFi/RSSI, IP, stato), switch lampeggio,
  fasce giorno/sera/notte, **switch modalita random**, **modalita test** con
  ora/minuto fittizi, pulsante riavvio.
- **Modalita random**: cambia effetto automaticamente a intervalli.
- **Modalita test**: mostra un orario fittizio per 5 secondi (per provare le parole).

## Modifiche tecniche rispetto alla tua V1.2.6

- EEPROM ampliata a **1024 byte**; la config MQTT e' stata **riallocata a partire
  da 512** perche' nella V1.3.0 gli indirizzi storici (24-217) sono ora occupati
  dal testo scorrevole (area 49-248). Le altre impostazioni restano agli indirizzi
  V1.3.0.
- Selettore effetto e nomi preset estesi da 0-18 a 0-39.
- Override "modalita test" applicato in un solo punto (`updateCurrentTimeFromTZ`).

## Compilazione (IMPORTANTE)

- Libreria richiesta: **PubSubClient** (knolleary) — installala da Gestione Librerie.
  (ArduinoJson e' gia' richiesta dalla V1.3.0.)
- **Partition Scheme: "Minimal SPIFFS"** (Strumenti -> Partition Scheme): lo sketch
  supera la partizione app di default (requisito gia' della V1.3.0).
- Primo aggiornamento da versioni precedenti **via USB**, non OTA.
- I tre file devono stare nella **stessa cartella** dello sketch.
- Per disabilitare del tutto l'MQTT: `#define ENABLE_MQTT false` (in cima al `.ino`).

## Pannello web MQTT (ripristinato il 30/06/2026)

Il **pannello web** per impostare il broker dal browser (broker, porta, utente,
password, base topic, nome dispositivo, abilita + "Salva e riconnetti") e' stato
**reinserito** in `webpage.h`, ed e' di nuovo registrata la route `/setMqttSettings`.
La password "***" lascia invariata quella salvata. Il broker si puo' quindi
cambiare dal browser senza ricompilare.

## Aggiornamenti V1.3.0-extra

Aggiunte rispetto al port MQTT base (tutte selezionabili da web e da Home Assistant):

### 1. Nuovo effetto "Albero di Natale" (preset 40 / `MODE_TREE` 33)
- Effetto importato verbatim dalla V1.2.6: albero verde a strati, tronco,
  stella lampeggiante e 12 palline colorate che si muovono con effetto fade.
- A schermo intero, **senza orario** (`manualModeSelected = true`, nessun overlay).
- Selezionabile da: dropdown effetti web (categoria Ambient, "Albero di Natale"),
  select Mode in Home Assistant, o preset 40 via MQTT (`{"preset":40}`).

### 2. Switch dedicato "Albero di Natale" (web + MQTT)
- **Web**: toggle nella pagina + route `GET /setTreeMode?enabled=1|0`
  (1 -> `applyPreset(40)`, 0 -> `applyPreset(0)` torna all'orologio).
- **MQTT**: entità `switch` HA (`<deviceId>_tree_mode`, icona `mdi:pine-tree`).
  Comando `{"treeModeEnabled":true|false}`; stato `treeModeEnabled` pubblicato.

### 3. Sensori diagnostici (5 nuove entità sensor HA)
- **CPU Temperature** (`temperatureRead()`, °C), **Free Memory** (`ESP.getFreeHeap()`, B),
  **Memory Usage** (% heap usato), **Uptime** (secondi), **CPU Usage** (%).
- **NOTA: la "CPU Usage" e' una STIMA**, non un valore reale. L'ESP32 non espone
  un carico CPU diretto; viene dedotto contando i giri di `loop()` al secondo
  (`cpuUsageTick()`/`estimatedCpuUsage()`) e confrontandoli con il massimo
  osservato dinamicamente: piu' i giri/sec calano, piu' alta e' la stima del carico.
- Stringa firmware aggiornata a **"1.3.0-mqtt"**.

### 4. Esposizione MQTT dei controlli V1.3.0 (testo scorrevole + overlay digitale)
- Controlli gia' presenti nel firmware V1.3.0, ora agganciati a MQTT riusando le
  stesse variabili globali e lo stesso salvataggio EEPROM degli handler web:
  - **switch "Testo Scorrevole"** (`scrollEnabled`, `mdi:text-shadow`)
  - **text "Testo Scorrevole"** (`scrollText` -> `customScrollText`, salvato in EEPROM)
  - **number "Velocita' Scorrimento"** (`scrollSpeed` -> `scrollTextSpeedMs`, 10-500 ms)
  - **switch "Overlay Digitale"** (`digitalOverlayEnabled`, `mdi:clock-digital`)
- Tutti i relativi campi di stato sono pubblicati in `publishMqttState`
  (capienza del documento JSON alzata da 1536 a 2560 byte).

## Struttura repository e aggiornamenti futuri

- `main` allineato all'upstream SurvivalHacking; `mqtt-homeassistant` = questo lavoro.
- Aggiornare dall'upstream:
  `git checkout main && git fetch upstream && git merge upstream/main && git push origin main`
  poi `git checkout mqtt-homeassistant && git merge main`.
- Le cartelle ufficiali (`Oraquadra2_1_3_0`, ecc.) restano identiche all'upstream:
  le tue modifiche vivono solo in `Oraquadra2_1_3_1_MQTT/`.
