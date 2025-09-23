# Device Sensor Firmware

Firmware ESP32-C6/S3 pour module capteurs environnementaux (SHT41, MQ-135, capteur de lumière) avec relais.

## Fonctionnalités
- Acquisition SHT41 via I2C (T° / RH)
- Lecture MQ-135 sur ADC avec calibration R0 persistante (NVS)
- Capteur lumière analogique
- Commande relais (T-Relay S3) avec failsafe OFF
- CLI UART pour calibration rapide (`mq135.r0`, `relay`)
- ESPNOW sécurisé vers la passerelle (topics JSON alignés avec cloud)

## Build
```bash
idf.py set-target esp32c6
idf.py menuconfig
idf.py build flash monitor
```

`partitions.csv` et `sdkconfig.defaults` définissent les partitions NVS + factory.
