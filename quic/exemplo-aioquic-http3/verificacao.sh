#!/bin/bash

# Função para verificar e instalar uma dependência
check_dependency() {
    dependency=$1
    package=$2

    if python -c "import $dependency" 2>/dev/null; then
        echo "$dependency is installed."
    else
        echo "$dependency is not installed. Installing $package..."
        python3 -m pip install $package
        if [ $? -eq 0 ]; then
            echo "Successfully installed $package."
        else
            echo "Failed to install $package."
        fi
    fi
}

check_dependency "argparse" "argparse"
check_dependency "asyncio" "asyncio"
check_dependency "aioquic" "aioquic"
check_dependency "wsproto" "wsproto"
check_dependency "cryptography" "cryptography"
check_dependency "sphinx_autodoc_typehints" "sphinx-autodoc-typehints"
check_dependency "sphinxcontrib-asyncio" "sphinxcontrib-asyncio"
check_dependency "requests" "requests"

python3 -m pip install --upgrade werkzeug

#O que funciona(pelo que eu testei:)

pip install -e .
pip install asgiref dnslib "flask<2.2" httpbin starlette "werkzeug<2.1" wsproto