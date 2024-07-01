#!/bin/bash

# instalar EMQX no Linux ou macOS
install_emqx_unix() {
  echo "Instalando EMQX no $1..."
  docker run -d --name emqx \
    -p 1883:1883 \
    -p 8083:8083 \
    -p 8084:8084 \
    -p 8883:8883 \
    -p 18083:18083 \
    -p 14567:14567/udp \
    -v /path/to/certs:/etc/certs \
    -e EMQX_LISTENERS__QUIC__DEFAULT__keyfile="/etc/certs/key.pem" \
    -e EMQX_LISTENERS__QUIC__DEFAULT__certfile="/etc/certs/cert.pem" \
    -e EMQX_LISTENERS__QUIC__DEFAULT__ENABLED=true \
    emqx/emqx:5.7.1
}

# instalar EMQX no Windows
install_emqx_windows() {
  echo "Instalando EMQX no Windows..."
  docker run -d --name emqx \
    -p 1883:1883 \
    -p 8083:8083 \
    -p 8084:8084 \
    -p 8883:8883 \
    -p 18083:18083 \
    -p 14567:14567/udp \
    -v /mnt/c/path/to/certs:/etc/certs \
    -e EMQX_LISTENERS__QUIC__DEFAULT__keyfile="/etc/certs/key.pem" \
    -e EMQX_LISTENERS__QUIC__DEFAULT__certfile="/etc/certs/cert.pem" \
    -e EMQX_LISTENERS__QUIC__DEFAULT__ENABLED=true \
    emqx/emqx:5.7.1
}

# Detectar SO
OS=$(uname -s)

case "$OS" in
  Linux*)
    install_emqx_unix "Linux"
    ;;
  Darwin*)
    install_emqx_unix "macOS"
    ;;
  CYGWIN*|MINGW*|MSYS*)
    install_emqx_windows
    ;;
  *)
    echo "Sistema operacional desconhecido: $OS"
    exit 1
    ;;
esac
