import asyncio
import pathlib
import ssl
import paho.mqtt.client as mqtt
from aioquic.asyncio import connect
from aioquic.quic.configuration import QuicConfiguration


async def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    # Inscreve-se no tópico desejado
    client.subscribe("test/topic")


async def on_message(client, userdata, msg):
    print(msg.topic + " " + str(msg.payload))


async def main():
    # Configuração do QUIC
    config = QuicConfiguration(is_client=True)

    # Certificado digital do cliente
    ssl_context = ssl.create_default_context(
        purpose=ssl.Purpose.SERVER_AUTH,
        cafile=pathlib.Path("ca.crt")
    )
    ssl_context.load_cert_chain(
        pathlib.Path("client.crt"),
        keyfile=pathlib.Path("client.key")
    )

    # Estabelecimento da conexão QUIC com o broker MQTT
    async with connect(
        "test.mosquitto.org",
        8883,
        configuration=config,
        create_protocol=mqtt.create_quic_connection,
        ssl=ssl_context
    ) as conn:
        client = mqtt.Client(transport=conn)
        client.on_connect = on_connect
        client.on_message = on_message
        client.tls_set(ca_certs="ca.crt")

        # Conexão com o broker MQTT
        await client.connect("test.mosquitto.org", 8883)

        # Publicação de uma mensagem no tópico
        client.publish("test/topic", "Hello, World!")

        # Loop de eventos do MQTT
        client.loop_forever()


if __name__ == "__main__":
    asyncio.run(main())
