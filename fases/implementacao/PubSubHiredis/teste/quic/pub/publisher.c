#define _POSIX_C_SOURCE 199309L
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

static nng_socket *g_sock;
#define CONN 1
#define SUB 2
#define PUB 3

double time_connection;

// Variáveis para rastrear o estado de reconexão
static int is_reconnecting = 0;
static uint32_t reconnection_delay = 100; // Atraso em milissegundos(100 ms padrao)
//o delay de reconexao é definido como o dobro do intervalo de publicação

conf_quic config_user = {
    .tls = {
        .enable = false,
        .cafile = "",
        .certfile = "",
        .keyfile = "",
        .key_password = "",
        .verify_peer = true,
        .set_fail = true,
    },
    .multi_stream = false,
    .qos_first = false,
    .qkeepalive = 30,
    .qconnect_timeout = 60,
    .qdiscon_timeout = 30,
    .qidle_timeout = 30,
};

static int connect_cb(void *rmsg, void *arg)
{
    if (is_reconnecting) {
        printf("[Reconnected][%s]...\n", (char *)arg);
        is_reconnecting = 0; // Reset reconnection flag
        printf("Waiting for %u milliseconds after reconnection...\n", reconnection_delay);
        nng_msleep(reconnection_delay); // Usar a variável de atraso
    } else {
        printf("[Connected][%s]...\n", (char *)arg);
    }
    return 0;
}

static int disconnect_cb(void *rmsg, void *arg)
{
    printf("[Disconnected][%s]...\n", (char *)arg);
    is_reconnecting = 1; // Set the flag to indicate reconnection is needed
    return 0;
}

static int msg_send_cb(void *rmsg, void *arg)
{
    printf("[Msg Sent][%s]...\n", (char *)arg);

    // Use arg to indicate when the message is sent
    *((int *)arg) = 1;
    return 0;
}

static nng_msg *mqtt_msg_compose(int type, int qos, char *topic, char *payload)
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
        nng_mqtt_msg_set_publish_payload(msg, (uint8_t *)payload, strlen(payload));
    }

    return msg;
}

char *tempo_para_varchar()
{
    struct timespec tempo_atual;
    clock_gettime(CLOCK_REALTIME, &tempo_atual);

    char *tempo_varchar = (char *)malloc(MAX_STR_LEN * sizeof(char));
    if (tempo_varchar == NULL) {
        perror("Erro ao alocar memória");
        exit(EXIT_FAILURE);
    }

    snprintf(tempo_varchar, MAX_STR_LEN, "%ld.%09ld", tempo_atual.tv_sec, tempo_atual.tv_nsec);

    return tempo_varchar;
}

void publish(const char *topic, int q, int *msg_sent)
{
    
    *msg_sent = 0; // Reset the flag
	char *tempo_atual_varchar = tempo_para_varchar();
    nng_msg *msg = mqtt_msg_compose(PUB, q, (char *)topic, tempo_para_varchar());
    printf("Tempo atual em varchar: %s\n", tempo_atual_varchar);
    nng_sendmsg(*g_sock, msg, NNG_FLAG_ALLOC);

    // Wait for the message to be sent
    while (!(*msg_sent)) {
        nng_msleep(1); // Sleep for a short duration to avoid busy waiting
    }
	*msg_sent = 0; // Reset the flag
    free(tempo_atual_varchar);
}

int client(int type, const char *url, const char *qos, const char *topic, const char *numero_pub, const char *interval)
{
    nng_socket sock;
    int rv, q, qpub, i;
    nng_msg *msg;
    const char *arg = "CLIENT FOR QUIC";
    uint32_t intervalo = atoi(interval);
    int msg_sent = 0; // Flag to indicate message sent
	reconnection_delay = intervalo*2; // Set reconnection delay to 2x the interval

    qpub = atoi(numero_pub);

    if (qpub <= 0) {
        printf("Número de publicações inválido.\n");
        printf("Número de publicações deve ser maior que 0.\n");
        return 0;
    }

    if ((rv = nng_mqtt_quic_client_open_conf(&sock, url, &config_user)) != 0) {
        printf("error in quic client open.\n");
    }

    if (0 != nng_mqtt_quic_set_connect_cb(&sock, connect_cb, (void *)arg) ||
        0 != nng_mqtt_quic_set_disconnect_cb(&sock, disconnect_cb, (void *)arg) ||
        0 != nng_mqtt_quic_set_msg_send_cb(&sock, msg_send_cb, &msg_sent)) {
        printf("error in quic client cb set.\n");
    }
    g_sock = &sock;

    msg = mqtt_msg_compose(CONN, 0, NULL, NULL);
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_REALTIME, &start_time);
    nng_sendmsg(sock, msg, NNG_FLAG_ALLOC);
    clock_gettime(CLOCK_REALTIME, &end_time);
    long seconds = end_time.tv_sec - start_time.tv_sec;
    long nanoseconds = end_time.tv_nsec - start_time.tv_nsec;
    double elapsed = seconds + nanoseconds * 1e-9;
    time_connection = elapsed;
    printf("Time taken to connect: %.9f seconds\n", elapsed);

    if (qos) {
        q = atoi(qos);
        if (q < 0 || q > 2) {
            printf("Qos should be in range(0~2).\n");
            q = 0;
        }
    }

    if (numero_pub) {
        for (i = 0; i < qpub; i++) {
            publish(topic, q, &msg_sent);
            nng_msleep(intervalo);
        }
    }


#if defined(NNG_SUPP_SQLITE)
    sqlite_config(&sock, MQTT_PROTOCOL_VERSION_v311);
#endif

    printf("terminou o envio\n");
    printf("Time taken to connect: %.9f seconds\n", time_connection);
    nng_close(sock);
    fprintf(stderr, "Done.\n");

    return (0);
}

static void printf_helper(char *exec)
{
    fprintf(stderr, "Usage: %s <url> <qos> <topic> <num_packets> <interval>\n", exec);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        goto error;
    }

    client(PUB, argv[1], argv[2], argv[3], argv[4], argv[5]);

    return 0;

error:
    printf_helper(argv[0]);
    return 0;
}
