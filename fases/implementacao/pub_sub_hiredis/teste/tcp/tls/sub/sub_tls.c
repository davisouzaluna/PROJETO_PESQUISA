#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <hiredis/hiredis.h>

#include <nng/mqtt/mqtt_client.h>
#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>
#include <nng/supplemental/tls/tls.h>

#define MAX_STR_LEN 30
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#define BILLION 1000000000

void fatal(const char *msg, int rv)
{
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
    printf("Valor salvo com sucesso no Redis: %s ,na chave: %s\n", value, redis_key);
    freeReplyObject(reply);

    // Encerrar a conexão com o servidor Redis
    redisFree(context);
}

void *store_in_redis_async(void *params) {
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

long long diferenca_tempo(struct timespec tempo1, struct timespec tempo2) {
    long long diff_sec = (long long)(tempo1.tv_sec - tempo2.tv_sec);
    long long diff_nsec = (long long)(tempo1.tv_nsec - tempo2.tv_nsec);
    return diff_sec * 1000000000LL + diff_nsec;
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



typedef struct {
    nng_socket sock;
    nng_dialer dialer;
    const char *url;
} reconnect_info;

int keepRunning = 1;
void intHandler(int dummy)
{
    (void) dummy;
    keepRunning = 0;
    fprintf(stderr, "\nclient exit(0).\n");
    exit(0);
}

static void disconnect_cb(nng_pipe p, nng_pipe_ev ev, void *arg)
{
    int reason = 0;
    nng_pipe_get_int(p, NNG_OPT_MQTT_DISCONNECT_REASON, &reason);
    printf("%s: disconnected! RC [%d] \n", __FUNCTION__, reason);

    reconnect_info *info = (reconnect_info *)arg;

    int rv;
    printf("Attempting to reconnect...\n");

    // Attempt to restart the dialer
    if ((rv = nng_dialer_start((info->dialer), NNG_FLAG_NONBLOCK)) != 0) {
        fprintf(stderr, "nng_dialer_start: %s\n", nng_strerror(rv));
    } else {
        printf("Reconnection successful.\n");
    }
}


static void connect_cb(nng_pipe p, nng_pipe_ev ev, void *arg)
{
    int reason;
    nng_pipe_get_int(p, NNG_OPT_MQTT_CONNECT_REASON, &reason);
    printf("%s: connected! RC [%d] \n", __FUNCTION__, reason);
    (void) ev;
    (void) arg;
}

void loadfile(const char *path, void **datap, size_t *lenp)
{
    FILE * f;
    size_t total_read      = 0;
    size_t allocation_size = BUFSIZ;
    char * fdata;
    char * realloc_result;

    if ((f = fopen(path, "rb")) == NULL) {
        fprintf(stderr, "Cannot open file %s: %s", path, strerror(errno));
        exit(1);
    }

    if ((fdata = malloc(allocation_size + 1)) == NULL) {
        fprintf(stderr, "Out of memory.");
    }

    while (1) {
        total_read += fread(fdata + total_read, 1, allocation_size - total_read, f);
        if (ferror(f)) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "Read from %s failed: %s", path, strerror(errno));
            exit(1);
        }
        if (feof(f)) {
            break;
        }
        if (total_read == allocation_size) {
            if (allocation_size > SIZE_MAX / 2) {
                fprintf(stderr, "Out of memory.");
            }
            allocation_size *= 2;
            if ((realloc_result = realloc(fdata, allocation_size + 1)) == NULL) {
                free(fdata);
                fprintf(stderr, "Out of memory.");
                exit(1);
            }
            fdata = realloc_result;
        }
    }
    if (f != stdin) {
        fclose(f);
    }
    fdata[total_read] = '\0';
    *datap            = fdata;
    *lenp             = total_read;
}

int init_dialer_tls(nng_dialer d, const char *cacert, const char *cert, const char *key, const char *pass)
{
    nng_tls_config *cfg;
    int rv;

    if ((rv = nng_tls_config_alloc(&cfg, NNG_TLS_MODE_CLIENT)) != 0) {
        return (rv);
    }

    if (cert != NULL && key != NULL) {
        nng_tls_config_auth_mode(cfg, NNG_TLS_AUTH_MODE_REQUIRED);
        if ((rv = nng_tls_config_own_cert(cfg, cert, key, pass)) != 0) {
            goto out;
        }
    } else {
        nng_tls_config_auth_mode(cfg, NNG_TLS_AUTH_MODE_NONE);
    }

    if (cacert != NULL) {
        if ((rv = nng_tls_config_ca_chain(cfg, cacert, NULL)) != 0) {
            goto out;
        }
    }

    rv = nng_dialer_set_ptr(d, NNG_OPT_TLS_CONFIG, cfg);

out:
    nng_tls_config_free(cfg);
    return (rv);
}

int subscribe(nng_socket sock, const char *topic,uint8_t qos, const char *redis_key, bool verbose){
    int rv;
    nng_mqtt_topic_qos subscriptions[] = {
        {
            .qos = qos,
            .topic = {
                .buf = (uint8_t *)topic,
                .length = strlen(topic),
            },
        },
    };

    size_t topic_qos_count =
		    sizeof(subscriptions) / sizeof(nng_mqtt_topic_qos);

    nng_msg *msg;
    nng_mqtt_msg_alloc(&msg, 0);
    nng_mqtt_msg_set_packet_type(msg, NNG_MQTT_SUBSCRIBE);
    nng_mqtt_msg_set_subscribe_topics(
		    msg, subscriptions, topic_qos_count);
    // Send subscribe message
	nng_sendmsg(sock, msg, NNG_FLAG_NONBLOCK);

    // Get PUBLISH payload and topic from msg;
		uint32_t payload_len;
		uint8_t *payload =
		    nng_mqtt_msg_get_publish_payload(msg, &payload_len);
		uint32_t    topic_len;
		const char *recv_topic =
		    nng_mqtt_msg_get_publish_topic(msg, &topic_len);

		printf("RECV: '%.*s' FROM: '%.*s'\n", payload_len,
		    (char *) payload, topic_len, recv_topic);

		uint8_t *send_data = nng_alloc(payload_len);
		memcpy(send_data, payload, payload_len);


    return rv;

}



int tls_client(const char *url, uint8_t proto_ver, const char *ca, const char *cert, const char *key, const char *pass, const char *topic,const char *redis_key, uint8_t qos, bool verbose)
{
    nng_socket sock;
    nng_dialer dialer;
    int rv;

    if (proto_ver == MQTT_PROTOCOL_VERSION_v5) {
        if ((rv = nng_mqttv5_client_open(&sock)) != 0) {
            fatal("nng_socket", rv);
        }
    } else {
        if ((rv = nng_mqtt_client_open(&sock)) != 0) {
            fatal("nng_socket", rv);
        }
    }

    nng_msg *msg;
    nng_mqtt_msg_alloc(&msg, 0);
    nng_mqtt_msg_set_packet_type(msg, NNG_MQTT_CONNECT);
    nng_mqtt_msg_set_connect_keep_alive(msg, 60);
    nng_mqtt_msg_set_connect_clean_session(msg, true);
    nng_mqtt_msg_set_connect_proto_version(msg, proto_ver);
    nng_mqtt_msg_set_connect_user_name(msg, "emqx");
    nng_mqtt_msg_set_connect_password(msg, "emqx123");

    nng_mqtt_set_connect_cb(sock, connect_cb, &sock);
    reconnect_info info = { sock, dialer, url };
    nng_mqtt_set_disconnect_cb(sock, disconnect_cb, NULL);

    if ((rv = nng_dialer_create(&dialer, sock, url)) != 0) {
        fatal("nng_dialer_create", rv);
    }

    if ((rv = init_dialer_tls(dialer, ca, cert, key, pass)) != 0) {
        fatal("init_dialer_tls", rv);
    }

    nng_dialer_set_ptr(dialer, NNG_OPT_MQTT_CONNMSG, msg);
    if ((rv = nng_dialer_start(dialer, NNG_FLAG_ALLOC)) != 0){
        fatal("nng_dialer_start", rv);
    }
    
    
    subscribe(sock, topic,qos,redis_key, verbose);
    
    
    //nng_close(sock);
    
}

void usage()
{
    fprintf(stderr, "Usage: ./tls [-u URL] [-a CAFILE] [-c CERT] [-k KEY] [-p PASS] [-v VERSION]\n");
    exit(1);
}

void usage_definition(){
    fprintf(stderr, "Usage: tls [-u URL] [-c CAFILE] [-t CERT] [-k KEY] [-p PASS] [-v VERSION]\n");
    fprintf(stderr, "  example: tls -u tls+mqtt-tcp://broker.emqx.io:8883 \n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -u URL      URL of the MQTT broker. Use the port 8883 and tls+mqtt-tcp to url protocol\n");
    fprintf(stderr, "  -a CAFILE   CA certificate file\n");
    fprintf(stderr, "  -c CERT     client certificate file\n");
    fprintf(stderr, "  -k KEY      client private key file\n");
    fprintf(stderr, "  -p PASS     client private key password\n");
    fprintf(stderr, "  -v VERSION  MQTT protocol version (default: 4)\n");
    fprintf(stderr, "  -h          Show this help\n");
    fprintf(stderr, "  -t TOPIC    Topic to publish\n");
    fprintf(stderr, "  -t TOPIC    Topic to publish\n");
    fprintf(stderr, "  -q QOS      QoS level (default: 0)\n");
    fprintf(stderr, "  -i INTERVAL Interval between packets in milliseconds (default: 1000)\n");
    fprintf(stderr, "  -n NUM      Number of packets to send (default: 1)\n");
    exit(1);
}

int main(int argc, char const *argv[])
{
    char *path = NULL;
    size_t file_len;
    char *url = NULL;
    char *cafile = NULL;
    char *cert = NULL;
    char *key = NULL;
    char *key_psw = NULL;
    uint8_t proto_ver = MQTT_PROTOCOL_VERSION_v311;//default version is 4
    int opt;

    // Elements for latency test
    const char *topic;
    topic = "teste_davi";
    uint8_t qos = 0; //default qos is 0
    bool verbose = false;
    const char *redis_key = "valores"; //chave padrão para o redis


    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);

    while ((opt = getopt(argc, (char *const *)argv, "a:c:k:u:p:v:t:q:r:")) != -1) {
        switch (opt) {
        case '?':
		case 'h':
			usage_definition();
			exit(0);
        case 'a':
            cafile = optarg;
            break;
        case 'c':
            cert = optarg;
            break;
        case 'k':
            key = optarg;
            break;
        case 'u':
            url = optarg;
            break;
        case 'p':
            key_psw = optarg;
            break;
        case 'v':
            proto_ver = atoi(optarg);
            break;
             case 't':
            topic = optarg;
            break;
        case 'q':
            qos = atoi(optarg);
            break;
        case 'r':
            redis_key = optarg;
            break;
        default:
            fprintf(stderr, "invalid argument: '%c'\n", opt);
            usage();
            exit(1);
        }
    }

    if (url == NULL) {
        usage();
    }

    if (cafile != NULL) {
        loadfile(cafile, (void **)&cafile, &file_len);
    }
    if (cert != NULL) {
        loadfile(cert, (void **)&cert, &file_len);
    }
    if (key != NULL) {
        loadfile(key, (void **)&key, &file_len);
    }

    if (tls_client(url, proto_ver, cafile, cert, key, key_psw, topic,redis_key, qos, verbose) != 0) {
        fprintf(stderr, "Error: tls_client\n");
        return 1;
    }

    return 0;
}
