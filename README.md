# Sistema de Detecção de Lâmpadas Queimadas em Iluminação Pública

Projeto de IoT desenvolvido para a disciplina de Internet das Coisas — Universidade Presbiteriana Mackenzie.

## Alunos

- Aline Fernanda da Silva
- Matheus Lucas Leite
- Roger Hideki Kuwahara

## Descrição

Sistema embarcado com ESP32 que monitora a luminosidade de lâmpadas públicas via sensor LDR e publica alertas de falha em tempo real usando o protocolo MQTT. O relé permite o acionamento remoto da lâmpada via comandos enviados pelo broker.

## Componentes

- ESP32 DevKit V1
- Sensor LDR (luminosidade)
- Módulo Relé
- LED (simulação da lâmpada)
- Broker MQTT: `broker.hivemq.com`

## Tópicos MQTT

| Tópico | Direção | Descrição |
|---|---|---|
| `iluminacao/luminosidade` | Publica | Valor ADC do LDR (0–4095) |
| `iluminacao/status` | Publica | `NORMAL` ou `FALHA_DETECTADA` |
| `iluminacao/rele` | Publica | `LIGADO` ou `DESLIGADO` |
| `iluminacao/comando` | Assina | `LIGAR` ou `DESLIGAR` |

## Arquivos

- `sketch.ino` — firmware do ESP32
- `diagram.json` — diagrama de montagem (Wokwi)
- `libraries.txt` — dependências (Wokwi)
- `mqtt_tester.py` — script Python para testar a comunicação MQTT

## Simulação

Abra o projeto no [Wokwi](https://wokwi.com) importando os arquivos `sketch.ino` e `diagram.json`. Para monitorar as mensagens MQTT, acesse o [HiveMQ WebSocket Client](https://www.hivemq.com/demos/websocket-client/) e inscreva-se no tópico `iluminacao/#`.
