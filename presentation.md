# Auslieferung ARDEP v2

## Features
- `sysbuild` -> Bau von Bootloader und Firmware in einem Schritt
- **Memeory Layout** angepasst (kein A und B Partitionen, sondern 2nd Stage Bootloader mit UDS etc.)
- **UDS** support für alle Mercedes relevanten Services
- **Power-IO Shield** support (inkl. Samples)
- ARDEP **Flash-Runner** ersetzt den `ardep flash` command
- **Blackmagic OCD** / Ardep v2 support
- Zephyr versions update auf **v4.2**
- Auführliche Samples für UDS support
- Dokumentation angepast auf ARDEP v2 und Zephyr v4.2
- Windows Support (inkl. Dokumentation)

# Demo Big Bus

`n` ARDEP v2 Boards über den Host automatsich ein Firmware upgread durchgeführt. Danach wird ein Datum über die Geräte gereicht und vom den einzelnen Boards signiert.

## Demo-Aufbau

- Man nehme `n` viele Ardeps mit `n > 1` und `n < 8`.
- Jedes Board wird mit einer Firmware geflashed
    ```sh
    west build -b ardep samples/uds_bus_sample -p --sysbuild -- -DSB_CONFIG_UDS_BASE_ADDR=0
    ```
    wobei die `DSB_CONFIG_UDS_BASE_ADDR` für jedes Board erhöhrt werden muss (individuelle Adresse des Boards)
- Die Boards und der Host werden über `CAN` miteinander verbunden
- Um die Logs von eiem Board über `CAN` zu erhalten füre auf dem Host aus:
    ```sh
    python3 samples/uds_bus_sample/can_log_recv.py -id 0x100
    ```
    Wobei die id (`0x100`) der Adresse des board (`n`) `+ 0x100` entspricht
- Starte die Demo:
    ```
    python3 samples/uds_bus_sample/client.py -b "ardep" -c can0 -u -n 5
    ```
    wobei `5` die (mximale) Anzahl der ARDEPS am `CAN` Bus ist

### Ablauf der `client.py` scripts

- Client baut `n` firmwares (`-n 5`) in einem separaten Ordner (`builds/fw0`, `builds/fw1` ...)
- Client überprüft wie viele ARDEP Boards am `CAN` Bus hängen (wie `Tester Present` UDS Service)
- Für jedes Board:
    - Client ändert die Diagnostic Session auf `Programming`
    - Client löscht die App auf dem Board
    - Client überträgt die neue Firmware
    - Client löst einen Reset aus
- Für jedes Board:
    - Client setzt eine Empfänger- und Sendeadresse, sodass die Boards eine Versandkette bilden (`Write Data By Identier` UDS Service)
        - Ein Board wartet auf eine Nachricht von der*Empfängeradresse*, signiert dies und sendet die Nachricht weiter an die Versandadresse
        - *Signatur* heißt in diesem Fall ein `xor` von Nachricht und physicalischen Source Address wird dem Datenpaket angehängt
- Client startet auf einem der Boards eine Asynchrone Routine über den `Routine Control` UDS Service
    - Die Routine auf dem Board generiert ein Datenbyte und sendet diese an seine Versandadresse und wartet auf dem Empfäng der signierten Nachricht auf der Empfangsadresse. (Nach dem Empfang ist die Routine erfolgreich abgeschlossen)
- Client prüft regelmäßig, ob die Routine abgeschlossen ist
- Client ruft das Ergebnis der Routine ab
- Client validiert, dass die Signaturen in der korrekten Reihenfolge vorliegen

# Demo UDS simple

Wir benötigen ein ARDEP Baord (es kann auch am Aufbau von obiger Demo hängen)

- Bauen und Flashen des samples
    ```sh
    west build -b ardep samples/uds -p
    python3 firmware_loader/client.py -c can0 -f build/zephyr/zephyr.signed.bin -r
    ```
- Ausführen des Samples mit
    ```sh
    python3 samples/uds/client.py -c can0
    ```

### Ablauf der `client.py` scripts
- Demonstration der Änderung der Session
- Schreiben und Lesen von mehreren Datan-Identifiern inklusive `IO-Control` mit LED
- Read/Write Memory by Address (16 Bytes)
- Demonstration vom DTC Service
- Demonstration von Routine-Control (synchron und asynchron)
- Security Access Level anhand eine Data Identier, der nur mit Security Access gelesen werden kann
- Authentication für Data Identifier (wie Security Access)