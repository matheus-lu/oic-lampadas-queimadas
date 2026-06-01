/*
 * Sistema de Detecção de Lâmpadas Queimadas em Iluminação Pública
 * Plataforma: ESP32
 * Sensores: LDR (luminosidade)
 * Atuador: Relé + LED (simulação da lâmpada)
 * Comunicação: MQTT via Wi-Fi (broker.hivemq.com)
 *
 * Tópicos MQTT:
 *   iluminacao/luminosidade  → publica leitura do LDR (0-4095)
 *   iluminacao/status        → publica "NORMAL" ou "FALHA_DETECTADA"
 *   iluminacao/rele          → publica estado do relé "LIGADO"/"DESLIGADO"
 *   iluminacao/comando       → assina comandos remotos "LIGAR" / "DESLIGAR"
 */

#include <WiFi.h>
#include <PubSubClient.h>

// ── Configurações de rede ─────────────────────────────────────────────────────
const char* SSID     = "Wokwi-GUEST";
const char* PASSWORD = "";

// ── Broker MQTT ───────────────────────────────────────────────────────────────
const char* MQTT_BROKER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;
const char* CLIENT_ID   = "esp32_iluminacao_pub";

// ── Tópicos ───────────────────────────────────────────────────────────────────
const char* TOPIC_LUZ     = "iluminacao/luminosidade";
const char* TOPIC_STATUS  = "iluminacao/status";
const char* TOPIC_RELE    = "iluminacao/rele";
const char* TOPIC_COMANDO = "iluminacao/comando";

// ── Pinos ─────────────────────────────────────────────────────────────────────
const int PIN_LDR  = 34;   // ADC – sensor de luminosidade
const int PIN_RELE = 13;   // saída digital – relé (e LED)
const int PIN_LED  = 2;    // LED onboard (feedback visual)

// ── Limiar de falha ───────────────────────────────────────────────────────────
// Abaixo deste valor (0-4095) considera-se lâmpada apagada/queimada
const int LIMIAR_FALHA = 500;

// ── Intervalo de publicação (ms) ──────────────────────────────────────────────
const unsigned long INTERVALO_PUB = 3000;

// ── Objetos globais ───────────────────────────────────────────────────────────
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

bool         releAtivo      = false;
unsigned long ultimaPub     = 0;

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg.trim();

  Serial.printf("[MQTT] Recebido em '%s': %s\n", topic, msg.c_str());

  if (String(topic) == TOPIC_COMANDO) {
    if (msg == "LIGAR") {
      releAtivo = true;
      digitalWrite(PIN_RELE, HIGH);
      digitalWrite(PIN_LED,  HIGH);
      mqtt.publish(TOPIC_RELE, "LIGADO");
      Serial.println("[RELE] Ligado por comando remoto.");
    } else if (msg == "DESLIGAR") {
      releAtivo = false;
      digitalWrite(PIN_RELE, LOW);
      digitalWrite(PIN_LED,  LOW);
      mqtt.publish(TOPIC_RELE, "DESLIGADO");
      Serial.println("[RELE] Desligado por comando remoto.");
    }
  }
}

void conectarWiFi() {
  Serial.printf("\n[WiFi] Conectando a %s", SSID);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[WiFi] Conectado. IP: %s\n", WiFi.localIP().toString().c_str());
}

void conectarMQTT() {
  while (!mqtt.connected()) {
    Serial.printf("[MQTT] Conectando ao broker %s...\n", MQTT_BROKER);
    if (mqtt.connect(CLIENT_ID)) {
      Serial.println("[MQTT] Conectado!");
      mqtt.subscribe(TOPIC_COMANDO);
      Serial.printf("[MQTT] Inscrito em: %s\n", TOPIC_COMANDO);
    } else {
      Serial.printf("[MQTT] Falha (rc=%d). Tentando em 3s...\n", mqtt.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_RELE, OUTPUT);
  pinMode(PIN_LED,  OUTPUT);
  digitalWrite(PIN_RELE, LOW);
  digitalWrite(PIN_LED,  LOW);

  conectarWiFi();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(callbackMQTT);
  conectarMQTT();
}

void loop() {
  // Mantém conexão MQTT ativa
  if (!mqtt.connected()) {
    conectarMQTT();
  }
  mqtt.loop();

  unsigned long agora = millis();
  if (agora - ultimaPub >= INTERVALO_PUB) {
    ultimaPub = agora;

    // Leitura do LDR
    int leitura = analogRead(PIN_LDR);
    Serial.printf("[LDR] Leitura: %d\n", leitura);

    // Publica valor de luminosidade
    char bufLuz[16];
    snprintf(bufLuz, sizeof(bufLuz), "%d", leitura);
    mqtt.publish(TOPIC_LUZ, bufLuz);

    // Avalia status da lâmpada
    const char* status;
    if (leitura < LIMIAR_FALHA) {
      status = "FALHA_DETECTADA";
      // Acende LED interno como alerta
      digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    } else {
      status = "NORMAL";
    }

    mqtt.publish(TOPIC_STATUS, status);
    Serial.printf("[MQTT] Publicado → %s: %s | %s: %s\n",
                  TOPIC_LUZ, bufLuz, TOPIC_STATUS, status);
  }
}
