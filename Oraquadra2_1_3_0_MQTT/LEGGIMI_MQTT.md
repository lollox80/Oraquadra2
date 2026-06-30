# OraQuadra2 V1.3.0 + MQTT / Home Assistant

Versione **V1.3.0 ufficiale di SurvivalHacking** con l'integrazione **MQTT +
Home Assistant** (il "server collegato") sviluppata da Lollo, ri-portata dalla
sua base V1.2.6 sulla nuova V1.3.0.

## File di questa cartella

- `Oraquadra2_1_3_0_MQTT.ino` — sketch V1.3.0 con 7 innesti minimi per l'MQTT.
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

## Unico punto opzionale ancora da fare

Il **pannello web** per impostare il broker dal browser (presente nella tua V1.2.6)
non e' stato reinserito nella nuova `webpage.h` per non rischiare conflitti con la
struttura riscritta della pagina V1.3.0. Non e' bloccante: il broker si configura
dai `MQTT_*_DEFAULT` o via le route web gia' attive. Se lo vuoi, lo aggiungo come
passo successivo.

## Struttura repository e aggiornamenti futuri

- `main` allineato all'upstream SurvivalHacking; `mqtt-homeassistant` = questo lavoro.
- Aggiornare dall'upstream:
  `git checkout main && git fetch upstream && git merge upstream/main && git push origin main`
  poi `git checkout mqtt-homeassistant && git merge main`.
- Le cartelle ufficiali (`Oraquadra2_1_3_0`, ecc.) restano identiche all'upstream:
  le tue modifiche vivono solo in `Oraquadra2_1_3_0_MQTT/`.
