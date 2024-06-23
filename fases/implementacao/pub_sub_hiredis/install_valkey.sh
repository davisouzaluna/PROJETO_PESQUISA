#!/bin/bash

INSTALL_DIR="$(pwd)/Valkey"
if [ ! -d "$INSTALL_DIR" ]; then
    echo "Criando diretório para instalação: $INSTALL_DIR"
    mkdir -p "$INSTALL_DIR" || exit 1
fi

cd "$INSTALL_DIR" || exit


if [ -z "$(ls -A $INSTALL_DIR)" ]; then
    
    git clone https://github.com/valkey-io/valkey . || exit 1
fi


sudo apt install tcl #dependencia necessaria
make
sudo make install
make test

if [ $? -eq 0 ]; then
    echo "valkey foi instalado com sucesso."
else
    echo "Erro ao instalar valkey."
fi
