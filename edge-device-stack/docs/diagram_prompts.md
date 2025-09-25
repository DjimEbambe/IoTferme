# Prompts & Diagrammes pour Présentations IoTferme

Ce document regroupe des scripts Mermaid/PlantUML et des prompts prêts à l'emploi pour illustrer l'architecture Cloud ↔ Edge ↔ Passerelle ↔ Devices ainsi que les principaux flux du projet IoTferme.

## 1. Vue d'ensemble (Mermaid)
```mermaid
graph TD
    subgraph Cloud
        A["MQTT Broker TLS"]
        B["web-gateway (REST/WS)"]
        C[iot-streamer]
        D[ml-engine]
    end

    subgraph Edge
        E[Raspberry Pi 5<br/>edge-agent]
        E -->|USB CDC MsgPack| F
    end

    subgraph Gateway
        F[ESP32-C6<br/>gateway_simple_c6]
    end

    subgraph Devices
        G1[ESP32-S3 T-RelayS3]
        G2[ESP32-C6 Sensor Nodes]
    end

    G1 -->|ESP-NOW| F
    G2 -->|ESP-NOW| F
    F -->|COBS/CRC16 MsgPack| E
    E -->|MQTT/TLS QoS1| A
    A -->|MQTT Subscriptions| B
    A -->|MQTT Streams| C
    C -->|Inferences| D
    D -->|Alerts/Insights| A

    B -->|HTTPS UI| User[(Opérateurs)]
```

## 2. Flux télémétrie (Mermaid Sequence)
```mermaid
sequenceDiagram
    participant Device as Capteur ESP32
    participant GW as Passerelle ESP32-C6
    participant Edge as Edge Agent (Pi 5)
    participant Broker as MQTT Broker
    participant Stream as iot-streamer
    participant ML as ml-engine

    Device->>GW: ESP-NOW (télémétrie + idempotency)
    GW->>Edge: USB CDC (COBS + MsgPack)
    Edge->>Broker: MQTT publish telemetry (QoS1)
    Broker-->>Stream: MQTT subscription
    Stream-->>ML: Flux temps réel / Feature vector
    ML-->>Broker: MQTT publish insights/events
    Broker-->>Edge: MQTT command/insight (si applicable)
```

## 3. Flux commande/ACK (Mermaid Sequence)
```mermaid
sequenceDiagram
    participant CloudUI as web-gateway UI/API
    participant Broker as MQTT Broker
    participant Edge as Edge Agent
    participant GW as Passerelle ESP32-C6
    participant Device as Actionneur ESP32

    CloudUI->>Broker: MQTT publish cmd (topic .../cmd)
    Broker-->>Edge: MQTT subscription cmd
    Edge->>GW: USB CDC MsgPack {type:"cmd"}
    GW->>Device: ESP-NOW cmd payload
    Device-->>GW: ESP-NOW ack
    GW-->>Edge: USB CDC MsgPack {type:"ack"}
    Edge-->>Broker: MQTT publish ack
    Broker-->>CloudUI: MQTT subscription ack / web push
```

## 4. Cycle de vie capteur (Mermaid State)
```mermaid
stateDiagram-v2
    [*] --> Boot
    Boot --> WarmupMQ135 : ENABLE_REAL_SENSORS?
    WarmupMQ135 --> Idle
    Boot --> Idle : Mode Synthétique
    Idle --> Measure : timer g_period_ms
    Measure --> Publish : ESP-NOW send telemetry
    Publish --> Idle
    Idle --> Command : cmd reçu (ESP-NOW)
    Command --> ApplyRelay : relay_control_enabled && commande valide
    ApplyRelay --> Idle
    Command --> UpdateFeatures : type == feature
    UpdateFeatures --> Idle
```

## 5. Déploiement (Mermaid "C4 style")
```mermaid
flowchart LR
    subgraph LocalSite[Ranch / Ferme]
        subgraph EdgeNode[Raspberry Pi 5]
            EA[edge-agent.service]
            SQLite[(SQLite backlog)]
            USB[/USB CDC/]
        end
        subgraph GatewayNode[Passerelle ESP32-C6]
            GWFirmware[gateway_simple_c6]
        end
        subgraph Sensors[ESP32 Capteurs/Actionneurs]
            S1[T-RelayS3]
            S2[ESP32-C6 Sensor]
        end
    end

    subgraph CloudCluster[VPS / Cloud]
        MQTTBroker[(MQTT TLS)]
        WebGW[web-gateway]
        Streamer[iot-streamer]
        MLEngine[ml-engine]
        DB[(InfluxDB/Storage)]
    end

    S1 & S2 -->|ESP-NOW| GWFirmware
    GWFirmware -->|USB CDC MsgPack| USB
    USB --> EA
    EA -->|MQTT/TLS| MQTTBroker
    MQTTBroker --> WebGW
    MQTTBroker --> Streamer
    Streamer --> DB
    Streamer --> MLEngine
    MLEngine --> MQTTBroker
    WebGW -->|REST/WS| Operators
```

## 6. Component Diagram (PlantUML)
```plantuml
@startuml
!define RECTANGLE class

package "Cloud" {
  RECTANGLE MQTT_Broker
  RECTANGLE Web_Gateway <<Service>>
  RECTANGLE IoT_Streamer <<Service>>
  RECTANGLE ML_Engine <<Service>>
  RECTANGLE TimeSeriesDB <<Store>>
}

package "Edge (Pi 5)" {
  RECTANGLE Edge_Agent <<FastAPI>>
  RECTANGLE SQLite_Backlog <<DB>>
}

node "Gateway" {
  RECTANGLE ESP32C6_Gateway
}

package "Devices" {
  RECTANGLE ESP32S3_Sensor
  RECTANGLE ESP32C6_Sensor
}

ESP32S3_Sensor --> ESP32C6_Gateway : ESP-NOW
ESP32C6_Sensor --> ESP32C6_Gateway : ESP-NOW
ESP32C6_Gateway --> Edge_Agent : USB CDC + MsgPack
Edge_Agent --> MQTT_Broker : MQTT/TLS QoS1
Edge_Agent --> SQLite_Backlog
MQTT_Broker --> Web_Gateway
MQTT_Broker --> IoT_Streamer
IoT_Streamer --> TimeSeriesDB
IoT_Streamer --> ML_Engine
ML_Engine --> MQTT_Broker
Web_Gateway --> Users
@enduml
```

## 7. Prompts (texte) pour supports projet
- **Pitch architecture** :
  > « Explique en 150 mots comment IoTferme relie capteurs ESP32 via ESP-NOW à une passerelle ESP32-C6, puis au Raspberry Pi 5 qui publie en MQTT/TLS vers notre cluster cloud (web-gateway, iot-streamer, ml-engine). Mets en avant la latence, la résilience offline et la modularité des firmwares. »

- **Risques & mitigations** :
  > « Liste sous forme de tableau les principaux risques techniques (perte USB, panne MQTT, dérive capteurs) et les contre-mesures implémentées dans IoTferme. »

- **Roadmap** :
  > « Propose une roadmap sur 3 releases pour IoTferme, couvrant sécurisation avancée, observabilité et optimisation énergétique. »

Ces prompts peuvent être utilisés avec un LLM pour générer des supports écrits complémentaires.
