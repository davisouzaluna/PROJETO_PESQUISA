# Subscriber MQTT/TCP

## O que é feito ?

Essa implementação realiza um teste de latência, salvando a diferença dos valores do publisher e o tempo atual em que se recebeu a mensagem no subscriber no redis-server.

## Fluxo de execução do teste

- Vamos supor que:
    - Que o tópico seja: `teste/topico/tcp` 
    - Que a URL do Broker seja: `mqtt-tcp://broker.emqx.io:1883`
    - O QoS seja: `0`
    - A chave redis seja: `teste-tcp`

    ```bash
    ./sub_tcp sub mqtt-tcp://broker.emqx.io:1883 0 teste/topico/tcp teste-tcp
    ```

- Ele vai se conectar no Broker:

    ```c
    client_connect(&sock, &dailer, url, verbose);
    ```
- Ele vai se subscrever assíncronamente:

    ```c
    // Asynchronous subscription
    nng_mqtt_client *client = nng_mqtt_client_alloc(sock, &send_callback, NULL, true);
    nng_mqtt_subscribe_async(client, subscriptions,
        sizeof(subscriptions) / sizeof(nng_mqtt_topic_qos), NULL);
    ```
    - Caso queira se subscrever síncronamente observe dentro de NanoSDK no diretório: `src/suplemental/mqtt/mqtt_public.c` para fazer as alterações necessárias

- Ele faz o teste de Latência:

    ```c
    while (keepRunning) {
        nng_msg *msg;
        uint8_t *payload;
        uint32_t payload_len;
        int rv;
        struct timespec tempo_sub; // Variável para armazenar o tempo atual
        struct timespec tempo_pub; // Variável para armazenar o tempo de publicação
        long long diferenca;
        if ((rv = nng_recvmsg(sock, &msg, 0)) != 0) {
            fatal("nng_recvmsg", rv);
            continue;
        }

        // We should only receive publish messages
        assert(nng_mqtt_msg_get_packet_type(msg) == NNG_MQTT_PUBLISH);

        payload = nng_mqtt_msg_get_publish_payload(msg, &payload_len);
        tempo_sub = tempo_atual_timespec();//Tempo atual do subscriber(logo depois de receber a mensagem)
        tempo_pub = string_para_timespec(payload); //Transformando o tempo do publisher em timespec
        diferenca = diferenca_tempo(tempo_sub, tempo_pub); //Realizando a subtração entre os dois
        char *valor_redis;
        valor_redis = diferenca_para_varchar(diferenca);//Transformando a diferenca em um tipo aceito para se armazenar no REDIS-SERVER
        store_in_redis_async_call(valor_redis, redis_key);//Salvando numa chamada assíncrona

        print80("Received: ", (char *)payload, payload_len, true);

        if (verbose) {
            memset(buff, 0, sizeof(buff));
            nng_mqtt_msg_dump(msg, buff, sizeof(buff), true);
            printf("%s\n", buff);
        }

        nng_msg_free(msg);
    }
    ```

---

# Observações importantes

- O protocolo da URL é por padrão: `mqtt-tcp`


