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
#include <pthread.h>
#include <hiredis/hiredis.h>
#include <time.h>

#define MAX_STR_LEN 30
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

static nng_socket *g_sock;

#define CONN 1
#define PUB 3

struct timespec start_time, end_time;
struct timespec start_time_pub, end_time_pub;
double time_connection;

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
    .qconnect_timeout = 60, // pode ser alterado
    .qdiscon_timeout = 30,
    .qidle_timeout = 30,
};

typedef struct {
    const char *value;
    const char *redis_key;
} RedisParams;

void store_in_redis(const char *value, const char *redis_key) {
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

    const char *key = redis_key && *redis_key ? redis_key : "valores";
    redisReply *reply = redisCommand(context, "RPUSH %s \"%s\"", key, value);
    if (reply == NULL) {
        printf("Erro ao salvar os dados no Redis\n");
        redisFree(context);
        return;
    }
    printf("Valor salvo com sucesso no Redis: %s ,na chave: %s\n", value, redis_key);
    freeReplyObject(reply);

    redisFree(context);
}

void* store_in_redis_async(void *params) {
    RedisParams *redis_params = (RedisParams *)params;
    store_in_redis(redis_params->value, redis_params->redis_key);
    free((char*)redis_params->value);
    free((char*)redis_params->redis_key);
    free(params); // Libera a memória alocada para os parâmetros
    return NULL;
}

void store_in_redis_async_call(const char *value, const char *redis_key) {
    pthread_t thread;
    RedisParams *params = malloc(sizeof(RedisParams));
    if (params == NULL) {
        perror("Erro ao alocar memória para os parâmetros da thread");
        exit(EXIT_FAILURE);
    }
    params->value = strdup(value);
    params->redis_key = strdup(redis_key);

    if (pthread_create(&thread, NULL, store_in_redis_async, params) != 0) {
        perror("Erro ao criar a thread");
        free((char*)params->value);
        free((char*)params->redis_key);
        free(params);
        exit(EXIT_FAILURE);
    }

    pthread_detach(thread);
}

static int connect_cb(void *rmsg, void *arg) {
    printf("[Connected][%s]...\n", (char *)arg);
    return 0;
}

static int disconnect_cb(void *rmsg, void *arg) {
    printf("[Disconnected][%s]...\n", (char *)arg);
    return 0;
}

static int msg_send_cb(void *rmsg, void *arg) {
    clock_gettime(CLOCK_REALTIME, &end_time_pub);
    printf("[Msg Sent][%s]...\n", (char *)arg);
    *((int *)arg) = 1;
    return 0;
}

static nng_msg *mqtt_msg_compose(int type, int qos, char *topic, char *payload) {
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

void publish(const char *topic, int q, int *msg_sent) {
    *msg_sent = 0;
    char *byte_payload = "hehe"; // Usar uma função para gerar o payload se necessário
    nng_msg *msg = mqtt_msg_compose(PUB, q, (char *)topic, byte_payload);
    nng_sendmsg(*g_sock, msg, NNG_FLAG_ALLOC);

    while (!(*msg_sent)) {
        nng_msleep(1); // Sleep for a short duration to avoid busy waiting
    }
    *msg_sent = 0; // Reset the flag
}

int client(int type, const char *url, const char *qos, const char *topic, const char *numero_pub, const char *interval, const char *redis_key) {
    nng_socket sock;
    int rv, sz, q, qpub, i;
    nng_msg *msg;
    const char *arg = "CLIENT FOR QUIC";
    uint32_t intervalo = atoi(interval);
    int msg_sent = 0; // Flag to indicate message sent

    if (redis_key == NULL) {
        redis_key = "valores";
    }

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

    // MQTT Connect...
    msg = mqtt_msg_compose(CONN, 0, NULL, NULL);
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

    for (i = 0; i < qpub; i++) {
        clock_gettime(CLOCK_REALTIME, &start_time_pub);
        publish(topic, q, &msg_sent);
        clock_gettime(CLOCK_REALTIME, &end_time_pub);
        long segundos_pub = end_time_pub.tv_sec - start_time_pub.tv_sec;
        long nanosegundos_pub = end_time_pub.tv_nsec - start_time_pub.tv_nsec;
        double diff_pub = segundos_pub + nanosegundos_pub * 1e-9;

        // Conversão do tipo(double em varchar)
        char buf[64];
        sprintf(buf, "%.9f", diff_pub);
        printf("valor convertido: %s\n", buf);

        // Salvar no Redis
        store_in_redis_async_call(buf, redis_key);

        printf("latencia do publisher: %.9f segundos\n", diff_pub);
        nng_msleep(intervalo);
    }

    nng_close(sock);
    printf("terminou o envio\n");
    printf("Time taken to connect: %.9f seconds\n", time_connection);
    fprintf(stderr, "Done.\n");

    return 0;
}

static void printf_helper(char *exec) {
    fprintf(stderr, "Usage: %s <url> <qos> <topic> <num_packets> <interval> <[redis-key]>\n", exec);
    fprintf(stderr, "url: the broker url\n");
    fprintf(stderr, "qos: qos level(0~2)\n");
    fprintf(stderr, "topic: the topic name\n");
    fprintf(stderr, "num_packets: the number of packet to publish\n");
    fprintf(stderr, "interval: interval between sending two messages (ms)\n");
    fprintf(stderr, "redis-key: Optional, Redis key to store the values\n");
}

int main(int argc, char **argv) {
    if (argc < 6) {
        printf_helper(argv[0]);
        return 0;
    }
    if (argc == 6) {
        client(2, argv[1], argv[2], argv[3], argv[4], argv[5], NULL);
    } else {
        client(2, argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
    }

    return 0;
}
