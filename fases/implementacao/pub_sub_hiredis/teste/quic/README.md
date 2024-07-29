
# Teste com MQTT/QUIC utilizando NanoSDK

Nessa pasta contém duas subpastas:

``` markdown
 /pub : Se refere ao publisher. Ela vai enviar o tempo atual para o broker QUIC(Emqx)
```

```markdown
/sub : Se refere ao subscriber. Ela vai obter o tempo atual antes do método de obter a mensagem e
```

## Passos necessários para a execução do teste:

1. Você precisa que o projeto NanoSDK esteja compilado com a flag do protocolo QUIC. No bash de verificação de instalação já é feito isso, mas por via das dúvidas(ou caso tenha algum erro), dentro da pasta do NanoSDK execute:
    ```bash
    cd build
    cmake -G Ninja -DBUILD_SHARED_LIBS=OFF -DNNG_ENABLE_QUIC=ON ..
    ninja
    sudo ninja install
    ```


2. Tenha certeza de que o hiredis e o redis-cli está instalado no seu sistema. O instalador de pendências(arquivo bash `installing_compile_project.sh`) já verifica isso, mas tenha certeza disso;


3. Crie uma pasta(caso não exista) com o nome de build, que é onde irá ser armazenada seu executável. E compile. Nesse caso irei fazer isso para o publisher, mas o mesmo deve ser feito para o subscriber:

    ```bash
    cd pub
    mkdir build && cd build
    cmake .. && make
    ```
4. Faça o teste;

---



