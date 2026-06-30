# OraQuadra2 V1.3.0 + MQTT / Home Assistant

Questa cartella contiene la versione **V1.3.0 ufficiale di SurvivalHacking** con
l'aggiunta dell'integrazione **MQTT + Home Assistant** (il "server collegato")
sviluppata da Lollo.

## Struttura del repository

- Le cartelle `Oraquadra2_1_2_x` / `Oraquadra2_1_3_0` / `OraQuadra2_arduino` / `Oraquadra2PE_*`
  sono **identiche all'upstream** SurvivalHacking. Non vanno modificate: cosi
  gli aggiornamenti futuri si tirano senza conflitti con `git fetch upstream && git merge`.
- `Oraquadra2_1_3_0_MQTT/` e la **mia versione** = V1.3.0 + MQTT. Tutte le
  modifiche personali vivono solo qui.
- `_mie_versioni_storiche/` contiene le versioni MQTT precedenti (base V1.2.6) e
  la documentazione, conservate per riferimento.

## Branch git

- `main` -> allineato all'upstream SurvivalHacking.
- `mqtt-homeassistant` -> questo lavoro (V1.3.0 + MQTT).

## Aggiornare dall'upstream in futuro

```
git checkout main
git fetch upstream
git merge upstream/main
git push origin main
# poi riportare le novita nel mio branch:
git checkout mqtt-homeassistant
git merge main
```

## Cosa aggiunge l'integrazione MQTT

- Configurazione MQTT salvata in EEPROM e impostabile dalla pagina web
  (server, porta, utente, password) - nessuna ricompilazione necessaria.
- Auto-discovery in Home Assistant: luce (on/off + luminosita), selettore
  effetto (33 effetti della V1.3.0), pulsante riavvio, sensori (ora, WiFi).
- Pubblicazione automatica dello stato e riconnessione automatica al broker.

## Compilazione (IMPORTANTE)

- Libreria richiesta: **PubSubClient** (knolleary) - installala da Gestione Librerie.
- **Partition Scheme: "Minimal SPIFFS"** (Strumenti -> Partition Scheme), perche
  lo sketch supera la partizione app di default (requisito gia della V1.3.0).
- Primo aggiornamento da versioni precedenti **via USB**, non OTA.
- I file (`.ino` + `webpage.h` + `mqtt_oraquadra.h`) devono stare nella stessa
  cartella dello sketch.
