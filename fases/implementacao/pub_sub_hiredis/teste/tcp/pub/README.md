# Publisher MQTT/TCP utilizando NanoSDK

## O que é feito ?

Essa implementação envia o tempo atual para o broker.

## Fluxo de execução do teste

- Caso você queira:

    - publicar no tópico `teste/topico/tcp`
    - na url(broker) `mqtt-tcp://broker.emqx.io:1883`
    - com o qos`0`
    - com 10 milisegundos entre cada pacote: `10`
    - Enviar 11 pacotes
    ```bash
    ./pub_tcp mqtt-tcp://broker.emqx.io:1883 0 teste/topico/tcp 10 11
    ```
- Ele vai se conectar no broker:
    ```c
    int client_connect(nng_socket *sock, nng_dialer *dialer, const char *url, bool verbose) {
    int rv;

    if ((rv = nng_mqtt_client_open(sock)) != 0) {
        fatal("nng_socket", rv);
    }

    if ((rv = nng_dialer_create(dialer, *sock, url)) != 0) {
        fatal("nng_dialer_create", rv);
    }

    // create a CONNECT message
    /* CONNECT */
    nng_msg *connmsg;
    nng_mqtt_msg_alloc(&connmsg, 0);
    nng_mqtt_msg_set_packet_type(connmsg, NNG_MQTT_CONNECT);
    nng_mqtt_msg_set_connect_proto_version(connmsg, 4);
    nng_mqtt_msg_set_connect_keep_alive(connmsg, 60);
    nng_mqtt_msg_set_connect_user_name(connmsg, "nng_mqtt_client");
    nng_mqtt_msg_set_connect_password(connmsg, "secrets");
    nng_mqtt_msg_set_connect_will_msg(connmsg, (uint8_t *)"bye-bye", strlen("bye-bye"));
    nng_mqtt_msg_set_connect_will_topic(connmsg, "will_topic");
    nng_mqtt_msg_set_connect_clean_session(connmsg, true);

    printf("Connecting to server ...\n");
    nng_dialer_set_ptr(*dialer, NNG_OPT_MQTT_CONNMSG, connmsg);
    nng_dialer_start(*dialer, NNG_FLAG_NONBLOCK);

    return (0);
    }
    ```
- Ele vai publicar a quantidade de pacotes e durante um dado tempo entre os pacotes:

    ```c
       for (uint32_t i = 0; i < num_packets && keepRunning; i++) {
        client_publish(sock, topic, qos, verbose);

        if (interval_ms > 0) {
            nng_msleep(interval_ms); // Espera o intervalo especificado antes de publicar novamente
        }
    }

    ```
- Função de publicação:

    ```c
    int client_publish(nng_socket sock, const char *topic, uint8_t qos, bool verbose) {
    int rv;

    // Criar uma mensagem PUBLISH
    nng_msg *pubmsg;
    nng_mqtt_msg_alloc(&pubmsg, 0);
    nng_mqtt_msg_set_packet_type(pubmsg, NNG_MQTT_PUBLISH);
    nng_mqtt_msg_set_publish_dup(pubmsg, 0);
    nng_mqtt_msg_set_publish_qos(pubmsg, qos);
    nng_mqtt_msg_set_publish_retain(pubmsg, 0);

    // Gerar o timestamp atual e usá-lo como payload
    char *tempo_atual_varchar = tempo_para_varchar();
    nng_mqtt_msg_set_publish_payload(pubmsg, (uint8_t *)tempo_atual_varchar, strlen(tempo_atual_varchar));

    // Definir o tópico da mensagem
    nng_mqtt_msg_set_publish_topic(pubmsg, topic);

    if (verbose) {
        uint8_t print[1024] = {0};
        nng_mqtt_msg_dump(pubmsg, print, 1024, true);
        printf("%s\n", print);
    }

    printf("Publishing to '%s' with timestamp '%s'...\n", topic, tempo_atual_varchar);

    // Enviar a mensagem
    if ((rv = nng_sendmsg(sock, pubmsg, NNG_FLAG_NONBLOCK)) != 0) {
        fatal("nng_sendmsg", rv);
    }

    // Liberar a memória alocada para o timestamp
    free(tempo_atual_varchar);

    return rv;
    }

    ```

---
# Observações importantes

- No campo protocolo da URL deve ser eutilizado `mqtt-tcp` pois isso é uma padronização do NNG. 

- Para fins de comparação entre esse protocolo e o protocolo QUIC, use o atraso de `10ms` entre cada pacote(no campo interval).



