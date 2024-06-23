//
// The application has three sub-commands: `conn`and `sub`.
// The `conn` sub-command connects to the server.
// The `sub` sub-command subscribes to the given topic filter and blocks
// waiting for incoming messages.
//
// # Example:
//
// Connect to the specific server:
// ```
// $ ./sub conn 'mqtt-quic://127.0.0.1:14567'
// ```
//
// Subscribe to `topic` and waiting for messages and save the difference between the time of the message and the time of the subscription in Redis:
// ```
// $ ./sub sub "mqtt-quic://127.0.0.1:14567" 0 topic
// ```
//

#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>
#include <nng/mqtt/mqtt_quic.h>
#include <nng/mqtt/mqtt_client.h>
#include <hiredis/hiredis.h>
#include <time.h>
#include <sys/time.h>

#include "msquic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR_LEN 30
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#define BILLION 1000000000
#define CONN 1
#define SUB 2
#define PUB 3

static nng_socket * g_sock;

conf_quic config_user = {
	.tls = {
		.enable = false,
		.cafile = "",
		.certfile = "",
		.keyfile  = "",
		.key_password = "",
		.verify_peer = true,
		.set_fail = true,
	},
	.multi_stream = false,
	.qos_first  = false,
	.qkeepalive = 30,
	.qconnect_timeout = 60,
	.qdiscon_timeout = 30,
	.qidle_timeout = 30,
};

void store_in_redis(const char *value) {
    // Conectar ao servidor Redis na porta 6379(nem sei se é padrão)
    redisContext *context = redisConnect("127.0.0.1", 6379);
    if (context == NULL || context->err) {
        if (context) {
            printf("Erro na conexão com o Redis: %s\n", context->errstr);
            redisFree(context);
        } else {
            printf("Erro na alocação do contexto do Redis\n");
        }
        return;
    }

    printf("Conectado ao servidor Redis\n");


    // Salvar o valor no Redis com a chave valores
    redisReply *reply = redisCommand(context, "RPUSH valores \"%s\"", value);
    if (reply == NULL) {
        printf("Erro ao salvar os dados no Redis\n");
        redisFree(context);
        return;
    }
    printf("Valor salvo com sucesso no Redis: %s\n", value);
    freeReplyObject(reply);

    // Encerrar a conexão com o servidor Redis
    redisFree(context);
}

static void
fatal(const char *msg, int rv)
{
	fprintf(stderr, "%s: %s\n", msg, nng_strerror(rv));
}

struct timespec string_para_timespec(char *tempo_varchar) {
    struct timespec tempo;
    char *ponto = strchr(tempo_varchar, '.');
    if (ponto != NULL) {
        *ponto = '\0'; // separa os segundos dos nanossegundos
        tempo.tv_sec = atol(tempo_varchar);
        tempo.tv_nsec = atol(ponto + 1);
    } else {
        tempo.tv_sec = atol(tempo_varchar);
        tempo.tv_nsec = 0;
    }
    return tempo;
}

//retorna o temppo de nanosegundos em long long
long long tempo_atual_nanossegundos() {
    struct timespec tempo_atual;
    clock_gettime(CLOCK_REALTIME, &tempo_atual);

    // Converter segundos para nanossegundos e adicionar nanossegundos


return tempo_atual.tv_sec * BILLION + tempo_atual.tv_nsec;
}

//retorna o tempo atual em timespec
struct timespec tempo_atual_timespec() {
    struct timespec tempo_atual;
    clock_gettime(CLOCK_REALTIME, &tempo_atual);

    return tempo_atual;
}


char *tempo_para_varchar() {
    struct timespec tempo_atual;
    clock_gettime(CLOCK_REALTIME, &tempo_atual);
    
    // Convertendo o tempo para uma string legível
    char *tempo_varchar = (char *)malloc(MAX_STR_LEN * sizeof(char));
    if (tempo_varchar == NULL) {
        perror("Erro ao alocar memória");
        exit(EXIT_FAILURE);
    }

    snprintf(tempo_varchar, MAX_STR_LEN, "%ld.%09ld", tempo_atual.tv_sec, tempo_atual.tv_nsec);

    // Retornando a string de tempo
    return tempo_varchar;
}

// Função para calcular a diferença entre dois tempos em nanossegundos
long long diferenca_tempo(struct timespec tempo1, struct timespec tempo2) {
    long long diff_sec = (long long)(tempo1.tv_sec - tempo2.tv_sec);
    long long diff_nsec = (long long)(tempo1.tv_nsec - tempo2.tv_nsec);
    return diff_sec * 1000000000LL + diff_nsec;
}

// Função para converter diferença de tempo em string
char *diferenca_para_varchar(long long diferenca) {
    char *tempo_varchar = (char *)malloc(MAX_STR_LEN * sizeof(char));
    if (tempo_varchar == NULL) {
        perror("Erro ao alocar memória");
        exit(EXIT_FAILURE);
    }
    snprintf(tempo_varchar, MAX_STR_LEN, "%lld.%09lld", diferenca / 1000000000LL, diferenca % 1000000000LL);
    return tempo_varchar;
}

static nng_msg *
mqtt_msg_compose(int type, int qos, char *topic)
{
	// Mqtt connect message
	nng_msg *msg;
	nng_mqtt_msg_alloc(&msg, 0);

	if (type == CONN) {
		nng_mqtt_msg_set_packet_type(msg, NNG_MQTT_CONNECT);

		nng_mqtt_msg_set_connect_proto_version(msg, 4);
		nng_mqtt_msg_set_connect_keep_alive(msg, 30);
		nng_mqtt_msg_set_connect_clean_session(msg, true);
	} else if (type == SUB) {
		nng_mqtt_msg_set_packet_type(msg, NNG_MQTT_SUBSCRIBE);

		nng_mqtt_topic_qos subscriptions[] = {
			{
				.qos   = qos,
				.topic = {
					.buf    = (uint8_t *) topic,
					.length = strlen(topic)
				}
			},
		};
		int count = sizeof(subscriptions) / sizeof(nng_mqtt_topic_qos);

		nng_mqtt_msg_set_subscribe_topics(msg, subscriptions, count);
	} 

	return msg;
}

static int
connect_cb(void *rmsg, void * arg)
{
	printf("[Connected][%s]...\n", (char *)arg);
	return 0;
}

static int
disconnect_cb(void *rmsg, void * arg)
{
	printf("[Disconnected][%s]...\n", (char *)arg);
	return 0;
}

static int
msg_send_cb(void *rmsg, void * arg)
{
	printf("[Msg Sent][%s]...\n", (char *)arg);
	return 0;
}


//callback do subscriber
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
	store_in_redis(valor_redis);
	return 0;
	
}




int
client(int type, const char *url, const char *qos, const char *topic)
{
	nng_socket  sock;
	int         rv, sz, q;
	nng_msg *   msg;
	const char *arg = "CLIENT FOR QUIC";

	/*
	// Open a quic socket without configuration
	if ((rv = nng_mqtt_quic_client_open(&sock, url)) != 0) {
		printf("error in quic client open.\n");
	}
	*/

	if ((rv = nng_mqtt_quic_client_open_conf(&sock, url, &config_user)) != 0) {
		printf("error in quic client open.\n");
	}


	if (0 != nng_mqtt_quic_set_connect_cb(&sock, connect_cb, (void *)arg) ||
	    0 != nng_mqtt_quic_set_disconnect_cb(&sock, disconnect_cb, (void *)arg) ||
	    0 != nng_mqtt_quic_set_msg_recv_cb(&sock, msg_recv_cb, (void *)arg) ||
	    0 != nng_mqtt_quic_set_msg_send_cb(&sock, msg_send_cb, (void *)arg)) {
		printf("error in quic client cb set.\n");
	}
	g_sock = &sock;

	// MQTT Connect...
	msg = mqtt_msg_compose(CONN, 0, NULL);
	nng_sendmsg(sock, msg, NNG_FLAG_ALLOC);

	if (qos) {
		q = atoi(qos);
		if (q < 0 || q > 2) {
			printf("Qos should be in range(0~2).\n");
			q = 0;
		}
	}

	switch (type) {
	case CONN:
		break;
	case SUB:
		msg = mqtt_msg_compose(SUB, q, (char *)topic);
		nng_sendmsg(*g_sock, msg, NNG_FLAG_ALLOC);

		break;

    
	default:
		printf("Unknown command.\n");
	}

	for (;;)
		nng_msleep(100); //inserindo um tempo de 100ms para o subscriber para fazer com que a função ocorra de forma correta

	nng_close(sock);
	fprintf(stderr, "Done.\n");

	return (0);
}

static void
printf_helper(char *exec)
{
	fprintf(stderr, "Usage: %s conn <url>\n"
	                "       %s sub  <url> <qos> <topic>\n", exec, exec);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	int rc;

	if (argc < 3) {
		goto error;
	}
	if (0 == strncmp(argv[1], "conn", 4) && argc == 3) {
		client(CONN, argv[2], NULL, NULL);
	}
	else if (0 == strncmp(argv[1], "sub", 3)  && argc == 5) {
		client(SUB, argv[2], argv[3], argv[4]);
	}
	
	else {
		goto error;
	}

	return 0;

error:

	printf_helper(argv[0]);
	return 0;
}
