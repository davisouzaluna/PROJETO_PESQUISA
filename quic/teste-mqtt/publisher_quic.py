import ssl
import paho.mqtt.client as mqtt
from paho.mqtt.client import MQTTv5

# Configurações do cliente MQTT
BROKER_ADDRESS = "broker.quic.tech"
BROKER_PORT = 8883
CLIENT_ID = "meu-cliente-id"
USERNAME = "meu-username"
PASSWORD = "minha-senha"
TOPIC = "meu-topico"
QOS = 1

# Configurações do cliente QUIC
QUIC_VERSION = "QUIC_VERSION_1"
CA_CERT = "/path/to/ca.crt"
CLIENT_CERT = "/path/to/client.crt"
CLIENT_KEY = "/path/to/client.key"

# Define a função de callback para lidar com as mensagens recebidas
def on_message(client, userdata, message):
    print(f"Mensagem recebida: {str(message.payload.decode('utf-8'))}")

# Cria um cliente MQTT usando o protocolo de transporte QUIC
client = mqtt.Client(transport="quic", protocol=MQTTv5)

# Define as configurações do cliente MQTT e do QUIC
client.username_pw_set(username=USERNAME, password=PASSWORD)
client.tls_set(ca_certs=CA_CERT, certfile=CLIENT_CERT, keyfile=CLIENT_KEY,
               cert_reqs=ssl.CERT_REQUIRED, tls_version=QUIC_VERSION)

# Define a função de callback para lidar com as mensagens recebidas
client.on_message = on_message

# Conecta-se ao broker MQTT
client.connect(BROKER_ADDRESS, port=BROKER_PORT)

# Inicia a execução do cliente MQTT
client.loop_start()

# Publica uma mensagem no tópico
client.publish(topic=TOPIC, payload="Olá, mundo!", qos=QOS)


# Mantém o programa em execução para continuar recebendo mensagens
try:
    while True:
        pass

# Encerra a conexão do cliente MQTT
except KeyboardInterrupt: 
    print('\nSaindo') 
    client.loop_stop()
    client.disconnect()
