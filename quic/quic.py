import asyncio
import ssl
from aioquic.asyncio import QuicConnectionProtocol, serve
from aioquic.quic.connection import QuicConnection
from paho.mqtt.client import MQTTMessage
from paho.mqtt.client import MQTT_ERR_SUCCESS, MQTT_ERR_NO_CONN


class MqttOverQuic:

    def __init__(self, host, port, ssl_context, connect_callback, message_callback):
        self.host = host
        self.port = port
        self.ssl_context = ssl_context
        self.connection = None
        self.protocol = None
        self.connect_callback = connect_callback
        self.message_callback = message_callback
    
    @staticmethod
    async def connect():
        #connection = QuicConnection(
         #   alpn_protocols=["mqtt"],
          #  quic_logger=None,
        #)
        protocol = QuicConnectionProtocol(
            QuicConnection,
            create_stream=self._create_stream,
            stream_data_received=self._stream_data_received,
        )

        # Connect QUIC
        _, _ = await asyncio.wait_for(
            asyncio.gather(
                asyncio.ensure_future(
                    self._connect_quic(protocol, connection)
                ),
                asyncio.ensure_future(
                    self._connect_mqtt()
                )
            ),
            timeout=10
        )

        self.connection = connection
        self.protocol = protocol

    def _create_stream(self, protocol):
        return protocol.create_stream(0)

    def _stream_data_received(self, stream_id, data):
        if stream_id == 0:
            self.connect_callback(self, MQTT_ERR_SUCCESS)

        else:
            self.message_callback(
                self, None, MQTTMessage(
                    topic=data[2:].decode(), payload=None
                )
            )

    async def _connect_quic(self, protocol, connection):
        _, _ = await connection.connect(
            self.host,
            self.port,
            configuration=None,
            create_protocol=lambda: protocol,
            ssl=self.ssl_context,
        )

    async def _connect_mqtt(self):
        pass

    def publish(self, topic, payload=None, qos=0, retain=False):
        if not self.connection or not self.protocol:
            return MQTT_ERR_NO_CONN

        # Send MQTT message
        message = bytearray()
        message.append(0x30)
        message.extend(len(topic).to_bytes(2, byteorder="big"))
        message.extend(topic.encode())
        self.protocol.send_data(
            1, bytes(message)
        )

        return MQTT_ERR_SUCCESS

    def subscribe(self, topic, qos=0):
        if not self.connection or not self.protocol:
            return MQTT_ERR_NO_CONN

        # Send MQTT message
        message = bytearray()
        message.append(0x82)
        message.extend(len(topic).to_bytes(2, byteorder="big"))
        message.extend(topic.encode())
        message.append(qos)
        self.protocol.send_data(
            1, bytes(message)
        )

        return MQTT_ERR_SUCCESS

#função que chama o módulo acima. Preferi deixar tudo junto por enquanto

async def main():
    # Certificado digital do cliente
    ssl_context = ssl.create_default_context(
        purpose=ssl.Purpose.SERVER_AUTH,
        cafile="ca.crt",
    )
    ssl_context.load_cert_chain(
        "client.crt",
        keyfile="client.key",
    )
    
    mqtt = MqttOverQuic(
        host="test.mosquitto.org",
        port=8883,
        ssl_context=ssl_context,
        connect_callback=lambda c, r: print(f"Connected with result code {r}"),
        message_callback=lambda c, t, m: print(f"Received message on topic '{m.topic}'")
    )

    async def on_connect(client, result_code):
        print(f"Connected with result code {result_code}")
        # Subscrever no tópico ao conectar
        client.subscribe("test/topic")

    async def on_message(client, userdata, message):
        print(f"Received message '{message.payload.decode()}' on topic '{message.topic}'")
        message_callback=lambda c, u, m: print(f"Received message '{m.topic}'")

    await mqtt.connect()
    mqtt.subscribe("test/topic")

    while True:
        # Publish message to MQTT broker over QUIC
        message = input("Enter a message: ")
        MqttOverQuic.publish("test/topic", payload=message)

if __name__ == "__main__":
    asyncio.run(main())

   
