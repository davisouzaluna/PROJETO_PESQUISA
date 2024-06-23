#!/bin/bash

INSTALL_DIR="$(pwd)/hiredis"
if [ ! -d "$INSTALL_DIR" ]; then
    echo "Criando diretório para instalação: $INSTALL_DIR"
    mkdir -p "$INSTALL_DIR" || exit 1
fi

cd "$INSTALL_DIR" || exit


if [ -z "$(ls -A $INSTALL_DIR)" ]; then
    
    git clone https://github.com/redis/hiredis.git . || exit 1
fi

make
sudo make install

if [ $? -eq 0 ]; then
    echo "hiredis foi instalado com sucesso."
else
    echo "Erro ao instalar hiredis."
fi
