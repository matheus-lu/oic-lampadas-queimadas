"""
Testador MQTT - Sistema de Iluminação Pública
Mede tempos de resposta para sensor (LDR) e atuador (relé).

Instalar dependência:
    pip install paho-mqtt

Uso:
    python mqtt_tester.py
"""

import paho.mqtt.client as mqtt
import time
import statistics

BROKER = "broker.hivemq.com"
PORT   = 1883

TOPIC_LUZ     = "iluminacao/luminosidade"
TOPIC_STATUS  = "iluminacao/status"
TOPIC_RELE    = "iluminacao/rele"
TOPIC_COMANDO = "iluminacao/comando"

# Armazena medições
tempos_sensor   = []   # tempo entre publicação do ESP32 e recebimento aqui
tempos_atuador  = []   # tempo entre envio do comando e confirmação do relé

t_comando_enviado = None
recebendo_confirmacao = False

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[✓] Conectado ao broker {BROKER}")
        client.subscribe(TOPIC_LUZ)
        client.subscribe(TOPIC_STATUS)
        client.subscribe(TOPIC_RELE)
        print(f"[✓] Inscrito nos tópicos de monitoramento\n")
    else:
        print(f"[✗] Falha na conexão (rc={rc})")

def on_message(client, userdata, msg):
    global t_comando_enviado, recebendo_confirmacao
    t_recebido = time.time()
    payload = msg.payload.decode()

    if msg.topic == TOPIC_LUZ:
        # Tempo de resposta do sensor: estimado pelo timestamp de chegada
        # (o ESP32 publica a cada 3s; registramos o tempo de chegada)
        print(f"[LDR]    Luminosidade recebida: {payload}")

    elif msg.topic == TOPIC_STATUS:
        print(f"[STATUS] {payload}")

    elif msg.topic == TOPIC_RELE:
        if recebendo_confirmacao and t_comando_enviado is not None:
            dt = (t_recebido - t_comando_enviado) * 1000  # ms
            tempos_atuador.append(dt)
            print(f"[RELÉ]   Confirmação recebida: {payload} | Tempo: {dt:.1f} ms")
            recebendo_confirmacao = False
        else:
            print(f"[RELÉ]   Estado: {payload}")

def enviar_comando(client, comando):
    global t_comando_enviado, recebendo_confirmacao
    t_comando_enviado = time.time()
    recebendo_confirmacao = True
    client.publish(TOPIC_COMANDO, comando)
    print(f"\n[CMD]    Enviando comando: {comando}")

def main():
    client = mqtt.Client(client_id="testador_python_001")
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"Conectando ao broker {BROKER}:{PORT}...")
    client.connect(BROKER, PORT, keepalive=60)
    client.loop_start()

    time.sleep(2)  # aguarda conexão

    print("=" * 55)
    print("  TESTE DE TEMPO DE RESPOSTA DO ATUADOR (RELÉ)")
    print("=" * 55)
    print("Realizando 4 medições alternando LIGAR/DESLIGAR...\n")

    for i in range(4):
        comando = "LIGAR" if i % 2 == 0 else "DESLIGAR"
        enviar_comando(client, comando)
        time.sleep(3)   # aguarda confirmação

    print("\n" + "=" * 55)
    print("  AGUARDANDO LEITURAS DO SENSOR (LDR) - 4 ciclos")
    print("=" * 55)

    t_inicio = time.time()
    t_ultima_leitura = None
    contagem_sensor = 0

    while contagem_sensor < 4:
        # Registra tempo entre chegadas de mensagens do sensor
        agora = time.time()
        if t_ultima_leitura is not None:
            dt = (agora - t_ultima_leitura) * 1000
            if dt > 2000:  # evita ruído
                tempos_sensor.append(dt)
                contagem_sensor += 1
                print(f"[SENSOR] Medição {contagem_sensor}: {dt:.1f} ms de intervalo")
        t_ultima_leitura = agora
        time.sleep(3.1)

    # ── Resultados ──────────────────────────────────────────────────────────
    print("\n" + "=" * 55)
    print("  RESULTADOS")
    print("=" * 55)

    print("\nAtuador (Relé):")
    for i, t in enumerate(tempos_atuador, 1):
        print(f"  Medição {i}: {t:.1f} ms")
    if tempos_atuador:
        print(f"  Média: {statistics.mean(tempos_atuador):.1f} ms")

    print("\nSensor (LDR):")
    for i, t in enumerate(tempos_sensor, 1):
        print(f"  Medição {i}: {t:.1f} ms")
    if tempos_sensor:
        print(f"  Média: {statistics.mean(tempos_sensor):.1f} ms")

    client.loop_stop()
    client.disconnect()
    print("\n[✓] Teste concluído.")

if __name__ == "__main__":
    main()
