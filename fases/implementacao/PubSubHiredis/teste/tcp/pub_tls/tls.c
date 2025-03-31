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

#include <nng/mqtt/mqtt_client.h>
#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>
#include <nng/supplemental/tls/tls.h>

#define MAX_STR_LEN 30
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#define BILLION 1000000000
double time_connection;

void fatal(const char *msg, int rv)
{
    fprintf(stderr, "%s: %s\n", msg, nng_strerror(rv));
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

// Declare client_publish and tempo_para_varchar before their usage
int client_publish(nng_socket sock, const char *topic, uint8_t qos, bool verbose);
char *tempo_para_varchar(void);
int pub_time_packets(nng_socket sock, const char *topic, uint8_t qos, bool verbose, uint32_t interval, uint32_t num_packets);

int tls_client(const char *url, uint8_t proto_ver, const char *ca, const char *cert, const char *key, const char *pass, const char *topic, uint8_t qos, bool verbose, uint32_t interval, uint32_t num_packets)
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

    // Marcar o início do tempo
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_REALTIME, &start_time);
    
    nng_dialer_set_ptr(dialer, NNG_OPT_MQTT_CONNMSG, msg);
    if ((rv = nng_dialer_start(dialer, NNG_FLAG_ALLOC)) != 0){
        fatal("nng_dialer_start", rv);
    }

    clock_gettime(CLOCK_REALTIME, &end_time);

    // Calcular a diferença
    long seconds = end_time.tv_sec - start_time.tv_sec;
    long nanoseconds = end_time.tv_nsec - start_time.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    time_connection = elapsed;

    printf("Time taken to connect: %.9f seconds\n", elapsed);
    
    pub_time_packets(sock, topic, qos, verbose, interval, num_packets);
    
    nng_msg_free(msg);
    
    
}

//Latency test(publish packets in a given interval)
int pub_time_packets(nng_socket sock, const char *topic, uint8_t qos, bool verbose, uint32_t interval, uint32_t num_packets) {
    for (uint32_t i=0;i<num_packets && keepRunning;i++){ {
        client_publish(sock, topic, qos, verbose);
        
        if(interval>0){
            nng_msleep(interval);
            }  
    
        }
    }
    return 0;
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

void usage()
{
    fprintf(stderr, "Usage: ./tls [-u URL] [-a CAFILE] [-c CERT] [-k KEY] [-p PASS] [-v VERSION] [-t TOPIC] [-q QOS] [-i INTERVAL] [-n NUMBER_PACKETS]\n");
    exit(1);
}

void usage_definition(){
    fprintf(stderr, "Usage: tls [-u URL] [-c CAFILE] [-t CERT] [-k KEY] [-p PASS] [-v VERSION] [-t TOPIC] [-q QOS] [-i INTERVAL] [-n NUMBER_PACKETS]\n");
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
    uint32_t interval = 1000; //default interval is 1000 ms or 1 second
    uint32_t num_packets = 1; //default number of packets is 1
    bool verbose = false;
    


    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);

    while ((opt = getopt(argc, (char *const *)argv, "a:c:k:u:p:v:t:q:i:n:")) != -1) {
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
        case 'i':
            interval = atoi(optarg);
            break;  
        case 'n':
            num_packets = atoi(optarg);
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

    if (tls_client(url, proto_ver, cafile, cert, key, key_psw, topic, qos, verbose, interval, num_packets) != 0) {
        fprintf(stderr, "Error: tls_client\n");
        return 1;
    }else{
        printf("TLS client connected successfully\n");
        printf("Time taken to connect: %.9f seconds\n", time_connection);
    }

    return 0;
}