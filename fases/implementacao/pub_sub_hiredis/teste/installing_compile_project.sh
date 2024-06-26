#!/bin/bash

#verificar se um pacote está instalado
check_and_install_package() {
    package=$1
    if dpkg -l | grep -q "^ii  $package"; then
        echo "$package já está instalado."
    else
        echo "Instalando $package..."
        sudo apt-get install -y $package
    fi
}

# Atualiza a lista de pacotes do SO
echo "Atualizando a lista de pacotes..."
sudo apt-get update

# Dependências necessárias para o NanoSDK
dependencies=(
    cmake
    build-essential
    liblttng-ust-dev
    lttng-tools
    libssl-dev
    ninja-build
    libc6-dev
    gcc-multilib
)

# Verifica e instala cada dependência
for package in "${dependencies[@]}"; do
    check_and_install_package $package
done

# Instalar o Redis
if dpkg -l | grep -q "^ii  redis-server"; then
    echo "Redis já está instalado."
else
    echo "Instalando Redis..."
    sudo apt update
    sudo apt upgrade -y
    sudo apt install -y redis-server
fi

# Verifica o status do Redis
sudo systemctl status redis-server

echo "Instalação concluída."


is_compiled() {
    dir=$1
    binary_file=$2
    if [ -f "${dir}/build/${binary_file}" ]; then
        return 0
    else
        return 1
    fi
}

#Instalando o hiredis
if [ ! -d "./hiredis" ]; then
    echo "Clonando e instalando o hiredis..."
    git clone https://github.com/redis/hiredis.git
    cd hiredis
    mkdir build
    cd build
    cmake ..
    make && sudo make install
    cd ../..
else
    if is_compiled "./hiredis" "libhiredis.so"; then
        echo "hiredis já está compilado."
    else
        echo "Compilando o hiredis..."
        cd hiredis
        mkdir -p build
        cd build
        cmake ..
        make && sudo make install
        cd ../..
    fi
fi

#Instalando o NanoSDK
if [ ! -d "./NanoSDK" ]; then
    echo "Clonando e instalando o NanoSDK..."
    git clone https://github.com/emqx/NanoSDK.git
    cd NanoSDK
    git submodule update --init --recursive 
    mkdir build && cd build
    cmake -G Ninja -DBUILD_SHARED_LIBS=OFF -DNNG_ENABLE_QUIC=ON ..
    ninja
    sudo ninja install
    cd ../..
else
    if is_compiled "./NanoSDK" "libnng.a"; then
        echo "NanoSDK já está compilado."
    else
        echo "Compilando o NanoSDK..."
        cd NanoSDK
        mkdir -p build
        cd build
        cmake -G Ninja -DBUILD_SHARED_LIBS=OFF -DNNG_ENABLE_QUIC=ON -DNNG_ENABLE_TLS=ON ..
        ninja
        sudo ninja install
        cd ../..
    fi
fi
