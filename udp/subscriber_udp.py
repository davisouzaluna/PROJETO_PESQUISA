import paho.mqtt.client as mqtt

broker_address = "127.0.0.1" # Endereço IP do servidor MQTT
broker_port = 1883 # Porta padrão do MQTT
topic = "meu_topico"

def on_connect(client, userdata, flags, rc):
    print("Conectado com sucesso ao broker MQTT")

def on_message(client, userdata, message):
    print("Mensagem recebida no tópico: ", message.topic)
    print("Conteúdo da mensagem: ", str(message.payload.decode("utf-8")))

client = mqtt.Client(transport="udp", broker=broker_address)
client.on_connect = on_connect
client.on_message = on_message

client.connect(broker_address, broker_port)
client.subscribe(topic)

client.loop_forever()


#Considerações:

#O broker deve aceitar esse protocolo para poder haver comunicação