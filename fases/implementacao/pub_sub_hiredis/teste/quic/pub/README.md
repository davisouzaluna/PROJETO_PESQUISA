# Publisher MQTT/QUIC utilizando o NanoSDK

## O que é feito ?

Nessa implementação, é enviado o tempo atual em nanosegundos para um dado tópico

## Fluxo de execução do teste

- Caso você queira publicar no tópico `teste/quic`, na url do broker `mqtt-quic://broker.emqx.io:14567` e com o qos `0` e que queira publicar 1000 pacotes:

    ```bash
    ./publisher mqtt-quic://broker.emqx.io:14567 0 teste/quic 1000 10
    ```

- Ele vai se conectar no broker:

    ```c
    msg = mqtt_msg_compose(CONN, 0, NULL, NULL);
    nng_sendmsg(sock, msg, NNG_FLAG_ALLOC);
    ```

- Ele vai publicar no tópico:

    ```c
    if (numero_pub){
		for(i=0;i<qpub;i++){
            publish(topic,q);
			nng_msleep(10); //10 ms entre cada mensagem, um atraso para o subscriber lidar com as mensagens
        }}
    ```
- Função de publicação:

    ```c
    void publish(const char *topic, int q){
			char *tempo_atual_varchar = tempo_para_varchar();
			nng_msg *msg = mqtt_msg_compose(PUB, q, (char *)topic, tempo_atual_varchar);
			printf("Tempo atual em varchar: %s\n", tempo_atual_varchar);
			nng_sendmsg(*g_sock, msg, NNG_FLAG_ALLOC);
    };
    ```



___

# Observações importantes:

- O atraso entre o envio de uma mensagem e outra se deve ao fato de que quando foi feito alguns testes com um atraso menor, o subscriber não conseguiu lidar com as mensagens, tendo uma perda de 20% ou mais(em 1000 pacotes enviados, somente cerca de 600 eram manipulados).
- O tempo atual é obtido com a função: `clock_gettime(CLOCK_REALTIME, &tempo_atual);`.

