#!/bin/bash

#instalador de pendências para a utilização correta 
#do programa

#Precisamos do: Mosquitto, mosquitto-clients, pip(para a partir dele instalarmos o paho-mqtt)
#E após tudo isso iniciarmos o broker.


# Verifica se o pacote pip está instalado
if ! command -v pip &> /dev/null # Esse comando descarta a saída padrão(true). Ou seja, caso seja false, não irá ser imprimido nada na tela, já que a saída padrão é a função do dado comando
then
    echo "pip não está instalado. Instalando..."
    sudo apt-get update
    sudo apt-get install python3-pip -y
fi

# Verifica se o pacote paho-mqtt está instalado
if ! python3 -c "import paho.mqtt.client" &> /dev/null # Esse comando descarta a saída padrão(true). Ou seja, caso seja false, não irá ser imprimido nada na tela, já que a saída padrão é a função do dado comando
then
    echo "paho-mqtt não está instalado. Instalando..."
    sudo pip install paho-mqtt
fi

# Verifica se o pacote mosquitto está instalado
if ! command -v mosquitto &> /dev/null # Esse comando descarta a saída padrão(true). Ou seja, caso seja false, não irá ser imprimido nada na tela, já que a saída padrão é a função do dado comando
then
    echo "mosquitto não está instalado. Instalando..."
    sudo apt-get update
    sudo apt-get install mosquitto -y
fi

# Verifica se o pacote mosquitto-clients está instalado
if ! command -v mosquitto_sub &> /dev/null # Esse comando descarta a saída padrão(true). Ou seja, caso seja false, não irá ser imprimido nada na tela, já que a saída padrão é a função do dado comando
then
    echo "mosquitto-clients não está instalado. Instalando..."
    sudo apt-get update
    sudo apt-get install mosquitto-clients -y
fi

if which emqx >/dev/null; then

    echo "EMQ X está instalado no sistema."
else
    curl -s https://assets.emqx.com/scripts/install-emqx-deb.sh | sudo bash
    sudo apt-get install emqx
    sudo systemctl start emqx
fi

sudo systemctl start emqx #starta o broker do emqx

#O broker do mosquitto só trata informações TCP


#Nesse código há algumas linhas desnececessárias, como a instalação do 
#broker mosquitto, mas eu irei consertar melhor e colocar um instalador de pendências
#específico para o protocolo quic e udp