#!/bin/bash

# Verificar se o certificado já existe
if [ ! -f certificate.pem ]; then
    echo "Generating certificate..."
    openssl genpkey -algorithm RSA -out private_key.pem
    openssl req -new -x509 -key private_key.pem -out certificate.pem -days 365
    echo "Certificate generated."
else
    echo "Certificate already exists."
fi

