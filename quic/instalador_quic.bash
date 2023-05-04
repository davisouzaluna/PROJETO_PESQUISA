#!/bin/bash

# Verifica se o pacote paho-mqtt está instalado
if ! python -c "import paho.mqtt" &> /dev/null
then
    echo "O pacote paho-mqtt não está instalado. Instalando..."
    pip install paho-mqtt
fi

# Verifica se o pacote aiomqtt está instalado
if ! python -c "import aiomqtt" &> /dev/null
then
    echo "O pacote aiomqtt não está instalado. Instalando..."
    pip install aiomqtt
fi

# Verifica se o pacote aioquic está instalado
if ! python -c "import aioquic" > /dev/null 2>&1; then
    echo "O pacote aioquic não está instalado. Instalando..."
    pip install aioquic
    pip install aiodns
    pip install --upgrade aioquic
else
    echo "O pacote aioquic já está instalado."
fi

# Verifica se os arquivos do certificado digital existem
if [ ! -f mosquitto.org.crt ] || [ ! -f mosquitto.org.key ] || [ ! -f ca.crt ]
then
    echo "Gerando Chave privada"
    openssl genrsa -out client.key
    echo "Gerando certificado digital"
    openssl req -out client.csr -key client.key -new
fi

echo "Pronto para a comunicação MQTT."
