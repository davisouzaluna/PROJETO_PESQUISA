// Author: eeff <eeff at eeff dot dev>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "nng/mqtt/mqtt_client.h"
#include "nng/nng.h"
#include "nng/supplemental/util/platform.h"
#include <pthread.h>
#include <hiredis/hiredis.h>
long long start_publish_time;
long long end_publish_time;

#define MAX_STR_LEN 30
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#define BILLION 1000000000

// Subcommand
#define PUBLISH "pub"

double time_connection;
const char *g_redis_key;

void fatal(const char *msg, int rv) {
    fprintf(stderr, "%s: %s\n", msg, nng_strerror(rv));
}

typedef struct {
    const char *value;
    const char *redis_key;
} RedisParams;


void store_in_redis(const char *value, const char *redis_key) {
    // Conectar ao servidor Redis na porta 6379 (padrão)
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

    // Salvar o valor no Redis com a chave fornecida ou "valores" como padrão
    const char *key = redis_key && *redis_key ? redis_key : "valores";
    redisReply *reply = redisCommand(context, "RPUSH %s \"%s\"", key, value);
    if (reply == NULL) {
        printf("Erro ao salvar os dados no Redis\n");
        redisFree(context);
        return;
    }
    printf("Valor salvo com sucesso no Redis: %s ,na chave: %s\n", value, redis_key );
    freeReplyObject(reply);

    // Encerrar a conexão com o servidor Redis
    redisFree(context);
}


void* store_in_redis_async(void *params) {
    RedisParams *redis_params = (RedisParams *)params;
    store_in_redis(redis_params->value, redis_params->redis_key);
    free(params); // Libera a memória alocada para os parametros
    return NULL;
}

void store_in_redis_async_call(const char *value, const char *redis_key) {
    pthread_t thread;
    RedisParams *params = malloc(sizeof(RedisParams));
    if (params == NULL) {
        perror("Erro ao alocar memória para os parâmetros da thread");
        exit(EXIT_FAILURE);
    }
    params->value = value;
    params->redis_key = redis_key;

    if (pthread_create(&thread, NULL, store_in_redis_async, params) != 0) {
        perror("Erro ao criar a thread");
        free(params); // Libera a memória alocada em caso de falha na criação da thread
        exit(EXIT_FAILURE);
    }

    // opcional: Se não precisar esperar a thread terminar, você pode desanexá-la, por enquanto preferi deixar a thread rodando
    pthread_detach(thread);
}




long long tempo_atual_nanossegundos() {
    struct timespec tempo_atual;
    clock_gettime(CLOCK_REALTIME, &tempo_atual);

    // Converter segundos para nanossegundos e adicionar nanossegundos
    return tempo_atual.tv_sec * BILLION + tempo_atual.tv_nsec;
}
char *diferenca_para_varchar(long long diferenca) {
    char *tempo_varchar = (char *)malloc(MAX_STR_LEN * sizeof(char));
    if (tempo_varchar == NULL) {
        perror("Erro ao alocar memória");
        exit(EXIT_FAILURE);
    }
    snprintf(tempo_varchar, MAX_STR_LEN, "%lld.%09lld", diferenca / 1000000000LL, diferenca % 1000000000LL);
    return tempo_varchar;
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

int keepRunning = 1;
void intHandler(int dummy) {
    keepRunning = 0;
    fprintf(stderr, "\nclient exit(0).\n");
    exit(0);
}

static nng_msg *
mqtt_msg_compose(const char *type, int qos, char *topic){

    nng_msg *msg;
	nng_mqtt_msg_alloc(&msg, 0);
    if(type = "CONN"){
        nng_mqtt_msg_set_packet_type(msg, NNG_MQTT_CONNECT);
        nng_mqtt_msg_set_connect_proto_version(msg, 4);
        nng_mqtt_msg_set_connect_keep_alive(msg, 60);
        nng_mqtt_msg_set_connect_clean_session(msg, true);
    }
    return msg;
}



// Connect to the given address.
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
    // Marcar o início do tempo
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_REALTIME, &start_time);

    nng_dialer_set_ptr(*dialer, NNG_OPT_MQTT_CONNMSG, connmsg);
    nng_dialer_start(*dialer, NNG_FLAG_NONBLOCK);
    clock_gettime(CLOCK_REALTIME, &end_time);

    // Calcular a diferença
    long seconds = end_time.tv_sec - start_time.tv_sec;
    long nanoseconds = end_time.tv_nsec - start_time.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    time_connection = elapsed;
    printf("Time taken to connect: %.9f seconds\n", elapsed);

    return (0);
}

// Publish a message to the given topic and with the given QoS.
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
    char *valor_aleatorio = "hehe";
    nng_mqtt_msg_set_publish_payload(pubmsg, (uint8_t *)valor_aleatorio, strlen(valor_aleatorio));



    // Definir o tópico da mensagem
    nng_mqtt_msg_set_publish_topic(pubmsg, topic);

    if (verbose) {
        uint8_t print[1024] = {0};
        nng_mqtt_msg_dump(pubmsg, print, 1024, true);
        printf("%s\n", print);
    }

    printf("Publishing to '%s' with char '%s'...\n", topic, valor_aleatorio);

    // Marcar o início do tempo
    start_publish_time = tempo_atual_nanossegundos();
    // Enviar a mensagem
    if ((rv = nng_sendmsg(sock, pubmsg, NNG_FLAG_NONBLOCK)) != 0) {
        fatal("nng_sendmsg", rv);
    } else{
        end_publish_time = tempo_atual_nanossegundos();
    }
    const char *diff = diferenca_para_varchar(end_publish_time - start_publish_time);

    store_in_redis_async_call(diff, g_redis_key);
    
    

    return rv;
}


struct pub_params {
    nng_socket *sock;
    const char *topic;
    uint8_t qos;
    bool verbose;
    uint32_t interval;
};

void publish_cb(void *args) {
    struct pub_params *params = args;

    do {
        // Chama client_publish que agora gera o payload com o timestamp
        client_publish(*params->sock, params->topic, params->qos, params->verbose);

        // Espera o intervalo especificado antes de publicar novamente, se interval > 0
        if (params->interval > 0) {
            nng_msleep(params->interval); // Usa a função de sono da NNG para esperar
        }
    } while (params->interval > 0 && keepRunning); // Continua enquanto interval for positivo

    printf("thread_exit\n");
}


int main(const int argc, const char **argv) {
    nng_socket sock;
    nng_dialer dialer;

    if (argc < 6 || 0 != strcmp(argv[1], PUBLISH)) {
        goto error;
    }
    const char *url = argv[2];
    uint8_t qos = atoi(argv[3]);
    const char *topic = argv[4];
    uint32_t interval_ms = atoi(argv[5]);
    uint32_t num_packets = argc > 6 ? atoi(argv[6]) : 1;
    const char *redis_key = argc > 7 ? argv[7] : "valores"; 
    g_redis_key = redis_key;

    int rv = 0;
    char *verbose_env = getenv("VERBOSE");
    bool verbose = (verbose_env != NULL && strcmp(verbose_env, "1") == 0);

    printf("verbose is %d\n", verbose);
    client_connect(&sock, &dialer, url, verbose);

    // Setar a flag para tratamento de interrupção de saída com Ctrl+C
    signal(SIGINT, intHandler);

    /*Parte do código onde é enviado os pacotes somente com um intervalo de tempo entre cada(laço infinito) 

    while (keepRunning) {
        client_publish(sock, topic, qos, verbose);

        if (interval_ms > 0) {
            nng_msleep(interval_ms); // Espera o intervalo especificado antes de publicar novamente
        }
    }
    */

    

   for (uint32_t i = 0; i < num_packets && keepRunning; i++) {
        client_publish(sock, topic, qos, verbose);

        if (interval_ms > 0) {
            nng_msleep(interval_ms); // Espera o intervalo especificado antes de publicar novamente
        }
    }
    printf("Time taken to connect: %.9f seconds\n", time_connection);

    if ((rv = nng_close(sock)) != 0) {
        fatal("nng_close", rv);
    }

    return 0;

error:
    fprintf(stderr, "Usage: %s %s url qos topic interval_ms [num_packets] [redis_key]\n", argv[0], PUBLISH);
    return 1;
}
