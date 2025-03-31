# Subscriber MQTT/QUIC utilizando o NanoSDK

## O que é feito? 

É feito um teste de latencia entre o tempo do publisher e o tempo do subscriber. E essas informações são salvas no redis-server.

## Fluxo de execução do teste de Latência:

- Caso você se subscreva no tópico `teste/quic` na url `mqtt-quic://broker.emqx.io:14567` com o qos `0` e com a chave redis `quic`

```bash
./sub sub "mqtt-quic://broker.emqx.io:14567" 0 teste/quic quic
```

- Ele irá se conectar ao broker e depois se subscrever, além de definir a chave redis(que por padrão é valores):
```c
case SUB:
			
        g_redis_key= redis_key; //aqui eu defino o valor da variavel global para que o callback possa acessar
		msg = mqtt_msg_compose(CONN, 0, NULL);
		nng_sendmsg(sock, msg, NNG_FLAG_ALLOC);
		subscription(&sock, topic, q);
		printf("subscrito em : %s\n", topic);
```

- Logo depois ele irá se subscrever:

```c
void subscription(nng_socket *sock, const char *topic, int qos) {
    nng_msg *msg = mqtt_msg_compose(SUB, qos, (char *)topic);
    if (msg == NULL) {
        printf("Failed to compose subscribe message.\n");
        return;
    }
    int rv = nng_sendmsg(*sock, msg, NNG_FLAG_ALLOC);
    if (rv != 0) {
        printf("Failed to send subscribe message: %d\n", rv);
		nng_msleep(1000);//esperar umm segundo para tentar novamente
    } else {
        //printf("Successfully subscribed to topic: %s\n", topic);
    }
	return rv,msg;
}
```

- Quando alguma mensagem chega no subscriber, há um callback, que irá tratar a mensagem e por conseguinte enviar para o redis-server. Aqui é onde também é feito o teste de latência:

```c
static int
msg_recv_cb(void *rmsg, void * arg)
{	
	struct timespec tempo_sub;
	tempo_sub = tempo_atual_timespec();
	printf("[Msg Arrived][%s]...\n", (char *)arg);
	nng_msg *msg = rmsg;
	uint32_t topicsz, payloadsz;

	char *topic   = (char *)nng_mqtt_msg_get_publish_topic(msg, &topicsz);
	char *payload = (char *)nng_mqtt_msg_get_publish_payload(msg, &payloadsz);

	struct timespec tempo_pub;
	long long diferenca;
	tempo_pub = string_para_timespec(payload);
	diferenca = diferenca_tempo(tempo_sub, tempo_pub);//tempo do sub é o mais recente
	char *valor_redis;
	valor_redis = diferenca_para_varchar(diferenca);

   
	printf("topic   => %.*s\n"
	       "payload => %.*s\n",topicsz, topic, payloadsz, payload);
	//store_in_redis(valor_redis, g_redis_key);
	store_in_redis_async_call(valor_redis, g_redis_key);
	return 0;
	
}

```

# Observações importantes

- Na URL é importante que você coloque no campo de protocolo `mqtt-quic` na URL;
- A porta padrão para o protocolo QUIC no broker emqx é `14567`;
- No teste de latência a informação está sendo salva no redis por meio de uma requisição assíncrona, caso queira uma requiseção síncrona, utilize a função: `store_in_redis`;

---

