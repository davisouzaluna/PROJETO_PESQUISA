# Usando uma imagem do ubuntu 20.04
FROM ubuntu:20.04

# Definindo o mantenedor do container
LABEL maintainer="Sdavi738@gmail.com.com"

# Defina variáveis de ambiente para evitar prompts interativos durante a instalação
ENV DEBIAN_FRONTEND=noninteractive

# Atualize a lista de pacotes e instale as dependências necessárias
RUN apt-get update && \
    apt-get install -y cmake build-essential liblttng-ust-dev lttng-tools libssl-dev ninja-build libc6-dev gcc-multilib git && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Clonando o repositório NanoSDK e entrando no diretório...
RUN git clone https://github.com/emqx/NanoSDK && \
    cd NanoSDK && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake -G Ninja -DBUILD_SHARED_LIBS=OFF -DNNG_ENABLE_QUIC=ON ..

# Defiindo o workdir
WORKDIR /NanoSDK/build

# Comando para manter o container em execução, iterativo.
CMD [ "bash" ]
