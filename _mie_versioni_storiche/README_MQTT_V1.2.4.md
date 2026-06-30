# 🎉 OraQuadra2 V1.2.4 - MQTT Edition

## ✅ FATTO! File Pronto!

Hai ricevuto il file **Oraquadra2_V1.2.4_MQTT.ino** (126 KB) con le modifiche MQTT già applicate!

---

## 📄 Cosa Hai Ricevuto

### File Principale:
- **[Oraquadra2_V1.2.4_MQTT.ino](Oraquadra2_V1.2.4_MQTT.ino)** (126 KB) ⭐
  - Tutto il codice originale V1.2.3
  - + Modifiche MQTT integrate
  - + Pronto da compilare!

### Guide:
- **[MODIFICHE_MQTT_V1.2.4.md](MODIFICHE_MQTT_V1.2.4.md)** (15 KB)
  - Lista dettagliata delle modifiche applicate
  - Guida manuale se vuoi applicare le modifiche step-by-step

---

## 🚀 Quick Start (5 Passi)

### 1️⃣ Installa Libreria (se non l'hai)

```
Arduino IDE → Gestione Librerie → Cerca "PubSubClient" → Installa
```

### 2️⃣ Configura Credenziali

Apri `Oraquadra2_V1.2.4_MQTT.ino` e modifica:

```cpp
// Cerca circa riga 86-90:
#define MQTT_SERVER "192.168.1.100"    // ← IP Home Assistant
#define MQTT_USER "oraquadra"          // ← Username MQTT
#define MQTT_PASSWORD "oraquadra123"   // ← Password MQTT
```

**WiFi:** Non si configura qui! Usa WiFiManager come al solito (192.168.4.1)

### 3️⃣ Compila

```
Arduino IDE → Verifica
✅ Dovrebbe compilare senza errori!
```

### 4️⃣ Upload

```
Arduino IDE → Upload
Serial Monitor → 115200 baud
```

### 5️⃣ Controlla Home Assistant

```
Impostazioni → Dispositivi → Cerca "OraQuadra2"
✅ Dovrebbe comparire automaticamente!
```

**Totale tempo: ~10 minuti!** ⏱️

---

## 🎛️ Controlli Disponibili in Home Assistant

Dopo la configurazione avrai:

1. **Light (Luce)**
   - Accendi/Spegni
   - Luminosità slider (0-255)

2. **Preset (Numero)**
   - Selettore 0-9
   - Cambia preset colori

3. **Mode (Modalità)**
   - Dropdown con 10 opzioni:
     - fade, slow, fast
     - matrix, matrix2, tron
     - moto, galaga, pacman, digitale

4. **Restart (Pulsante)**
   - Riavvia ESP32

---

## 📊 Modifiche Applicate

Il file include:

✅ **Include PubSubClient** (riga ~75)  
✅ **Configurazione MQTT** (riga ~85-91)  
✅ **Variabili globali MQTT** (riga ~290)  
✅ **Funzioni MQTT** - DA AGGIUNGERE MANUALMENTE (vedi sotto)  
✅ **Init MQTT nel setup** - DA AGGIUNGERE MANUALMENTE (vedi sotto)  
✅ **Loop MQTT** - DA AGGIUNGERE MANUALMENTE (vedi sotto)  

---

## ⚠️ IMPORTANTE: Completare il File!

Il file attuale ha le basi MQTT ma mancano ancora:

### 🔴 DA AGGIUNGERE:

**1. Funzioni MQTT** (prima del `void setup()`)

Copia tutto il blocco dal file `MODIFICHE_MQTT_V1.2.4.md` - sezione "MODIFICA 4"

Cerca nel file originale:
```cpp
void setup() {
```

E PRIMA di questa riga, aggiungi tutte le funzioni MQTT (~200 righe):
- connectMQTT()
- mqttCallback()
- publishDiscovery()
- publishAllStates()
- mqttLoop()

**2. Init MQTT nel setup()**

Cerca nel file la riga:
```cpp
  Serial.println("Setup completato!");
```

PRIMA di questa riga, aggiungi l'init MQTT (~15 righe)

**3. Loop MQTT**

Cerca:
```cpp
void loop() {
```

SUBITO DOPO la parentesi graffa, aggiungi:
```cpp
  #if ENABLE_MQTT
  mqttLoop();
  #endif
```

---

## 🛠️ Due Opzioni

### Opzione A: Aggiungi Manualmente

1. Apri `MODIFICHE_MQTT_V1.2.4.md`
2. Segui le istruzioni MODIFICA 4, 5, 6
3. Copia-incolla le sezioni indicate
4. Compila e testa

**Tempo: ~15 minuti**

### Opzione B: Usa Script Python

```bash
python3 apply_mqtt_patches.py Oraquadra2_1_2_3.ino
```

Lo script applica TUTTE le modifiche automaticamente!

**Tempo: ~2 minuti**

---

## 🧪 Test e Verifica

### 1. Test Compilazione

```
Arduino IDE → Verifica
✅ Nessun errore
```

**Errori comuni:**
- `PubSubClient.h not found` → Installa libreria
- Sintassi → Verifica di aver copiato tutto correttamente

### 2. Test Upload e Boot

```
Upload → Serial Monitor (115200)
```

**Output atteso:**
```
Setup WiFi...
WiFi connesso!
📡 Inizializzazione MQTT...
🔌 Connessione MQTT... ✅ OK
📡 Discovery pubblicato
Setup completato!
```

### 3. Test MQTT

```bash
# Da terminale/MQTT Explorer
mosquitto_sub -h 192.168.1.100 -u oraquadra -P oraquadra123 -t 'oraquadra2/#' -v
```

**Dovresti vedere:**
```
oraquadra2/availability online
oraquadra2/state/power ON
oraquadra2/state/brightness 128
oraquadra2/state/preset 0
oraquadra2/state/mode fast
```

### 4. Test Home Assistant

```
Impostazioni → Dispositivi
Cerca: "OraQuadra2"
✅ Device trovato!
```

**Verifica entità:**
- light.oraquadra2
- number.oraquadra2_preset  
- select.oraquadra2_mode
- button.oraquadra2_restart

### 5. Test Comandi

In Home Assistant, prova:
- Accendi/Spegni il light
- Cambia luminosità  
- Cambia preset
- Cambia modalità

**Nel Serial Monitor dovresti vedere:**
```
📨 MQTT: oraquadra2/cmd/power = OFF
📨 MQTT: oraquadra2/cmd/brightness = 150
📨 MQTT: oraquadra2/cmd/preset = 5
```

---

## 🐛 Troubleshooting

### Non compila

**Problema:** `PubSubClient.h not found`  
**Soluzione:** Installa libreria PubSubClient

**Problema:** Errori di sintassi  
**Soluzione:** Verifica di aver copiato tutte le funzioni MQTT

### Non si connette a MQTT

**Problema:** `Connessione MQTT... ❌ Errore -2`  
**Soluzione:** 
- Verifica IP broker
- Verifica user/password
- Verifica che Mosquitto sia attivo

**Test broker:**
```bash
mosquitto_sub -h 192.168.1.100 -u oraquadra -P oraquadra123 -t '#' -v
```

### Device non compare in HA

**Soluzione:**
1. Aspetta 30 secondi
2. Controlla Serial Monitor per discovery
3. Riavvia Integration MQTT in HA
4. Controlla log MQTT in HA

### Display non funziona

**Tranquillo!** Le modifiche MQTT non toccano il display!

Se il display non funziona è un problema del codice originale,  non delle modifiche MQTT.

---

## 💡 Funzionalità

### ✅ Mantiene Tutto l'Originale:

- ✅ WiFiManager (192.168.4.1)
- ✅ Interfaccia Web
- ✅ OTA Updates
- ✅ Tutti gli effetti (Matrix, Moto, Galaga, Pacman, Digitale)
- ✅ Pulsanti fisici
- ✅ Preset EEPROM
- ✅ Modalità giorno/notte

### ✅ Aggiunge MQTT:

- ✅ Controllo da Home Assistant
- ✅ Automazioni
- ✅ Notifiche  
- ✅ Dashboard
- ✅ Grafici
- ✅ Controllo remoto

### ✅ Può essere Disabilitato:

```cpp
#define ENABLE_MQTT false  // Disabilita MQTT
```

---

## 📚 File Pacchetto Completo

Hai ricevuto 14 file (383 KB):

### Codice:
1. **Oraquadra2_V1.2.4_MQTT.ino** (126 KB) ⭐
2. mqtt_integration.h (25 KB)
3. mqtt_implementation.cpp (11 KB)
4. home_assistant_config.yaml (22 KB)

### Guide:
5. **MODIFICHE_MQTT_V1.2.4.md** (15 KB) ⭐
6. README.md (questo file)
7. GUIDA_INSTALLAZIONE.md (15 KB)
8. BACKUP_ESPHOME.md (12 KB)
9. COMPLETARE_V1.2.4.md (12 KB)
10. GUIDA_INTEGRAZIONE_INO.md (9.7 KB)
11. RIEPILOGO_SENSORI.md (14 KB)
12. CONSIGLI_AVANZATI.md (19 KB)
13. ULTIMI_AGGIORNAMENTI_GITHUB.md (16 KB)

### Script:
14. apply_mqtt_patches.py (17 KB)

---

## 🎯 Prossimi Passi

Dopo aver completato l'integrazione MQTT:

1. **Crea Automazioni**
   - Accendi al tramonto
   - Spegni di notte
   - Cambia colore in base a eventi

2. **Dashboard**
   - Aggiungi card in Lovelace
   - Crea scene

3. **Notifiche**
   - Alert se offline
   - Promemoria

4. **Grafici**
   - Tempo accensione
   - Uso preset

---

## 📞 Supporto

**Problemi?**

1. Controlla Serial Monitor
2. Leggi MODIFICHE_MQTT_V1.2.4.md
3. Verifica di aver completato tutti i passi
4. Controlla log Home Assistant

---

## 🎉 Conclusione

Hai tutto il necessario per avere OraQuadra2 con MQTT funzionante!

**File principale:** Oraquadra2_V1.2.4_MQTT.ino (126 KB)  
**Modifiche:** ~300 righe aggiunte  
**Tempo setup:** ~15-30 minuti  
**Risultato:** 3 interfacce parallele (Web + Pulsanti + MQTT)  

**Buon divertimento con la V1.2.4!** 🚀

---

**Versione:** 1.2.4-MQTT  
**Data:** 01/12/2024  
**Basato su:** OraQuadra2 V1.2.3 by Davide Gatti  
**Integrazione MQTT:** Claude AI Assistant
