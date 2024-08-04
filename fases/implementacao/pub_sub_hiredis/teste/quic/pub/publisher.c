
// Publish timestamp to `topic` with `qos` on `url` for `time_pub` times.:
// ```
// $ ./publisher "mqtt-quic://127.0.0.1:14567" 0 topic 1000

//#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>
#include <nng/mqtt/mqtt_client.h>
#include <nng/mqtt/mqtt_quic.h>
#include "msquic.h"

#include <time.h>

#define MAX_STR_LEN 30
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

static nng_socket * g_sock;
#define CONN 1
#define SUB 2
#define PUB 3

double time_connection;

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



static nng_msg *
mqtt_msg_compose(int type, int qos, char *topic, char *payload)
{
	// Mqtt connect message
	nng_msg *msg;
	nng_mqtt_msg_alloc(&msg, 0);

	if (type == CONN) {
		nng_mqtt_msg_set_packet_type(msg, NNG_MQTT_CONNECT);

		nng_mqtt_msg_set_connect_proto_version(msg, 4);
		nng_mqtt_msg_set_connect_keep_alive(msg, 30);
		nng_mqtt_msg_set_connect_clean_session(msg, true);
	} else if (type == PUB) {
		nng_mqtt_msg_set_packet_type(msg, NNG_MQTT_PUBLISH);

		nng_mqtt_msg_set_publish_dup(msg, 0);
		nng_mqtt_msg_set_publish_qos(msg, qos);
		nng_mqtt_msg_set_publish_retain(msg, 0);
		nng_mqtt_msg_set_publish_topic(msg, topic);
		nng_mqtt_msg_set_publish_payload(
		    msg, (uint8_t *) payload, strlen(payload));
	}

	return msg;
}

/*
static char* timenow() {
#include <time.h>

struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts); // Você pode mudar para CLOCK_REALTIME se preferir
    int sz;

    // Calcula o tamanho necessário da string
    sz = snprintf(NULL, 0, "%ld", ts.tv_nsec);

    // Aloca memória para a string
    char *data = (char *)malloc(sz + 1); // Adiciona 1 para o caractere nulo de terminação

    if (data == NULL) {
        fprintf(stderr, "Erro ao alocar memória.\n");
        return NULL;
    }

    // Converte o valor de ts.tv_nsec para uma string de caracteres
    snprintf(data, sz + 1, "%ld", ts.tv_nsec);

    return data;
}
*/

// Função para obter o tempo atual e convertê-lo em uma string
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

void publish(const char *topic, int q){
			char *tempo_atual_varchar = tempo_para_varchar();
			nng_msg *msg = mqtt_msg_compose(PUB, q, (char *)topic, tempo_atual_varchar);
			printf("Tempo atual em varchar: %s\n", tempo_atual_varchar);
			nng_sendmsg(*g_sock, msg, NNG_FLAG_ALLOC);
};

int
client(int type, const char *url, const char *qos, const char *topic, const char *numero_pub, const char *interval)
{

	nng_socket  sock;
	int         rv, sz, q, qpub, i;
	nng_msg *   msg;
	const char *arg = "CLIENT FOR QUIC";
	uint32_t intervalo = atoi(interval);
	/*
	// Open a quic socket without configuration
	if ((rv = nng_mqtt_quic_client_open(&sock, url)) != 0) {
		printf("error in quic client open.\n");
	}
	
	*/
	qpub = atoi(numero_pub);

	if(qpub<=0){
		printf("Número de publicações inválido.\n");
		printf("Número de publicações deve ser maior que 0.\n");
		return 0;
	
	}

	if ((rv = nng_mqtt_quic_client_open_conf(&sock, url, &config_user)) != 0) {
		printf("error in quic client open.\n");
	}

	if (0 != nng_mqtt_quic_set_connect_cb(&sock, connect_cb, (void *)arg) ||
	    0 != nng_mqtt_quic_set_disconnect_cb(&sock, disconnect_cb, (void *)arg) ||
	    0 != nng_mqtt_quic_set_msg_send_cb(&sock, msg_send_cb, (void *)arg)) {
		printf("error in quic client cb set.\n");
	}
	g_sock = &sock;

	//Teste para descobrir o tempo de envio da mensagem de conexão

	// MQTT Connect...
	msg = mqtt_msg_compose(CONN, 0, NULL, NULL);
	// Marcar o início do tempo
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_REALTIME, &start_time);
	nng_sendmsg(sock, msg, NNG_FLAG_ALLOC);
	clock_gettime(CLOCK_REALTIME, &end_time);
	// Calcular a diferença
    long seconds = end_time.tv_sec - start_time.tv_sec;
    long nanoseconds = end_time.tv_nsec - start_time.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    time_connection = elapsed;
    printf("Time taken to connect: %.9f seconds\n", elapsed);

	if (qos) {
		q = atoi(qos);
		if (q < 0 || q > 2) {
			printf("Qos should be in range(0~2).\n");
			q = 0;
		}
	}

	if (numero_pub){

	

		
		for(i=0;i<qpub;i++){
		/*
		If data is NULL, the payload is the time in ns.
		*/
		
			/*
			struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC_RAW, &ts);//Posso alterar essa variável para CLOCK_REALTIME
			sz = snprintf(data, 128, "%ld", ts.tv_nsec);
			*/
			//data = timenow();
			//printf("Data: %s\n", data);
			
		

			publish(topic,q);
			nng_msleep(intervalo);//necessário um atraso pro subscriber poder lidar com as mensagens
			//printf("Publicação %d\n", i+1);
		//msg = mqtt_msg_compose(PUB, q, (char *)topic, (char *)data);
		//nng_sendmsg(*g_sock, msg, NNG_FLAG_ALLOC);
		
		}
	}else{
		msg = mqtt_msg_compose(SUB, q, (char *)topic, NULL);
		nng_sendmsg(*g_sock, msg, NNG_FLAG_ALLOC);
	
	}
	if ((rv = nng_mqtt_quic_client_open_conf(&sock, url, &config_user)) != 0) {
		printf("error in quic client open.\n");
	}

#if defined(NNG_SUPP_SQLITE)
	sqlite_config(&sock, MQTT_PROTOCOL_VERSION_v311);
#endif

     printf("terminou o envio\n");
	 printf("Time taken to connect: %.9f seconds\n", time_connection);   	
	//nng_msleep(0);
	nng_close(sock);//aqui demora um pouco pois ele está desalocando os recursos
	fprintf(stderr, "Done.\n");

	return (0);

}

static void
printf_helper(char *exec)
{
	fprintf(stderr, "Usage: %s <url> <qos> <topic> <num_packets> <interval>\n", exec);
	//fprintf(stderr, "Usage: %s <url> <qos> <topic>\n", exec);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        goto error;
    }
	
    //client(PUB, argv[1], argv[2], argv[3], argv[4], argv[5]);
	client(PUB, argv[1], argv[2], argv[3], argv[4], argv[5]);
	//printf("Time taken to connect: %.9f seconds\n", time_connection);
    
    return 0;


error:

	printf_helper(argv[0]);
	return 0;
}