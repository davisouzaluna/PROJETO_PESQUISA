#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include <nng/mqtt/mqtt_client.h>
#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>


#define MAX_STR_LEN 30
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#define BILLION 1000000000

const char *g_topic;
uint8_t g_qos;
bool g_verbose;
uint32_t interval_ms;
uint32_t num_packets;


#ifdef NNG_SUPP_TLS
#include <nng/supplemental/tls/tls.h>

static void loadfile(const char *path, void **datap, size_t *lenp);
static int  init_dialer_tls(nng_dialer d, const char *cacert, const char *cert,
     const char *key, const char *pass);
#endif

static size_t nwork = 32;

struct work {
	enum { INIT, RECV, WAIT, SEND } state;
	nng_aio *aio;
	nng_msg *msg;
	nng_ctx  ctx;
};

#define SUB_TOPIC1 "/nanomq/msg/1"
#define SUB_TOPIC2 "/nanomq/msg/2"
#define SUB_TOPIC3 "/nanomq/msg/3"

void
fatal(const char *msg, int rv)
{
	fprintf(stderr, "%s: %s\n", msg, nng_strerror(rv));
}

long long tempo_atual_nanossegundos() {
    struct timespec tempo_atual;
    clock_gettime(CLOCK_REALTIME, &tempo_atual);

    // Converter segundos para nanossegundos e adicionar nanossegundos
    return tempo_atual.tv_sec * BILLION + tempo_atual.tv_nsec;
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

void
client_cb(void *arg)
{
	struct work *work = arg;
	nng_msg *    msg;
	int          rv;

	switch (work->state) {

	case INIT:
		work->state = RECV;
		nng_ctx_recv(work->ctx, work->aio);
		break;

	case RECV:
		if ((rv = nng_aio_result(work->aio)) != 0) {
			fatal("nng_recv_aio", rv);
			work->state = RECV;
			nng_ctx_recv(work->ctx, work->aio);
			break;
		}

		work->msg   = nng_aio_get_msg(work->aio);
		work->state = WAIT;
		nng_sleep_aio(0, work->aio);
		break;

	case WAIT:
		msg = work->msg;

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

		nng_msg_header_clear(work->msg);
		nng_msg_clear(work->msg);

		// Send payload to topic "/nanomq/msg/transfer"
		char *topic = "/nanomq/msg/transfer";
		nng_mqtt_msg_set_packet_type(work->msg, NNG_MQTT_PUBLISH);
		nng_mqtt_msg_set_publish_topic(work->msg, topic);
		nng_mqtt_msg_set_publish_payload(
		    work->msg, send_data, payload_len);

		printf("SEND: '%.*s' TO:   '%s'\n", payload_len,
		    (char *) send_data, topic);

		nng_free(send_data, payload_len);
		nng_aio_set_msg(work->aio, work->msg);
		work->msg   = NULL;
		work->state = SEND;
		nng_ctx_send(work->ctx, work->aio);
		break;

	case SEND:
		if ((rv = nng_aio_result(work->aio)) != 0) {
			nng_msg_free(work->msg);
			fatal("nng_send_aio", rv);
		}
		work->state = RECV;
		nng_ctx_recv(work->ctx, work->aio);
		break;

	default:
		fatal("bad state!", NNG_ESTATE);
		break;
	}
}

struct work *
alloc_work(nng_socket sock)
{
	struct work *w;
	int          rv;

	if ((w = nng_alloc(sizeof(*w))) == NULL) {
		fatal("nng_alloc", NNG_ENOMEM);
	}
	if ((rv = nng_aio_alloc(&w->aio, client_cb, w)) != 0) {
		fatal("nng_aio_alloc", rv);
	}
	if ((rv = nng_ctx_open(&w->ctx, sock)) != 0) {
		fatal("nng_ctx_open", rv);
	}
	w->state = INIT;
	return (w);
}

void
connect_cb(nng_pipe p, nng_pipe_ev ev, void *arg)
{
	int reason = 0;
	// get connect reason
	nng_pipe_get_int(p, NNG_OPT_MQTT_CONNECT_REASON, &reason);
	// get property for MQTT V5
	// property *prop;
	// nng_pipe_get_ptr(p, NNG_OPT_MQTT_CONNECT_PROPERTY, &prop);
	printf("%s: connected[%d]!\n", __FUNCTION__, reason);

	if (reason == 0) {
		nng_socket *sock = arg;
		nng_mqtt_topic_qos topic_qos[] = {
			{ .qos     = 0,
			    .topic = { .buf = (uint8_t *) SUB_TOPIC1,
			        .length     = strlen(SUB_TOPIC1) } },
			{ .qos     = 1,
			    .topic = { .buf = (uint8_t *) SUB_TOPIC2,
			        .length     = strlen(SUB_TOPIC2) } },
			{ .qos     = 2,
			    .topic = { .buf = (uint8_t *) SUB_TOPIC3,
			        .length     = strlen(SUB_TOPIC3) } }
		};

		size_t topic_qos_count =
		    sizeof(topic_qos) / sizeof(nng_mqtt_topic_qos);

		// Connected succeed
		nng_msg *submsg;
		nng_mqtt_msg_alloc(&submsg, 0);
		nng_mqtt_msg_set_packet_type(submsg, NNG_MQTT_SUBSCRIBE);
		nng_mqtt_msg_set_subscribe_topics(
		    submsg, topic_qos, topic_qos_count);

		// Send subscribe message
		nng_sendmsg(*sock, submsg, NNG_FLAG_NONBLOCK);
	}
}

void
disconnect_cb(nng_pipe p, nng_pipe_ev ev, void *arg)
{
	printf("%s: disconnected!\n", __FUNCTION__);
}

static void
send_message_interval(void *arg)
{
	nng_socket *sock = arg;
	nng_msg *   pub_msg;
	nng_mqtt_msg_alloc(&pub_msg, 0);

	nng_mqtt_msg_set_packet_type(pub_msg, NNG_MQTT_PUBLISH);
	nng_mqtt_msg_set_publish_topic(pub_msg, SUB_TOPIC1);
	nng_mqtt_msg_set_publish_payload(
	    pub_msg, (uint8_t *) "offline message", strlen("offline message"));

	for (;;) {
		nng_msleep(2000);
		nng_msg *dup_msg;
		nng_msg_dup(&dup_msg, pub_msg);
		nng_sendmsg(*sock, dup_msg, NNG_FLAG_ALLOC);
		printf("sending message\n");
	}
}

#if defined(NNG_SUPP_SQLITE)
static int
sqlite_config(
    nng_socket *sock, nng_mqtt_sqlite_option *sqlite, uint8_t proto_ver)
{
	int rv;

	// set sqlite option
	nng_mqtt_set_sqlite_enable(sqlite, true);
	nng_mqtt_set_sqlite_flush_threshold(sqlite, 10);
	nng_mqtt_set_sqlite_max_rows(sqlite, 20);
	nng_mqtt_set_sqlite_db_dir(sqlite, "/tmp/nanomq");

	// init sqlite db
	nng_mqtt_sqlite_db_init(sqlite, "mqtt_client.db", proto_ver);

	// set sqlite option pointer to socket
	return nng_socket_set_ptr(*sock, NNG_OPT_MQTT_SQLITE, sqlite);

	return (0);
}
#endif

int
client(const char *url, uint8_t proto_ver)
{
	nng_socket   sock;
	nng_dialer   dialer;
	struct work *works[nwork];
	int          i;
	int          rv;
	
	if (proto_ver == MQTT_PROTOCOL_VERSION_v5) {
		if ((rv = nng_mqttv5_client_open(&sock)) != 0) {
			fatal("nng_socket", rv);
		}
	} else {
		if ((rv = nng_mqtt_client_open(&sock)) != 0) {
			fatal("nng_socket", rv);
		}
	}

#if defined(NNG_SUPP_SQLITE)
	nng_mqtt_sqlite_option *sqlite;
	if ((rv = nng_mqtt_alloc_sqlite_opt(&sqlite)) != 0) {
		fatal("nng_mqtt_alloc_sqlite_opt", rv);
	}
	sqlite_config(&sock, sqlite, proto_ver);

#endif

	for (i = 0; i < nwork; i++) {
		works[i] = alloc_work(sock);
	}

	// Mqtt connect message
	nng_msg *msg;
	nng_mqtt_msg_alloc(&msg, 0);
	nng_mqtt_msg_set_packet_type(msg, NNG_MQTT_CONNECT);
	nng_mqtt_msg_set_connect_keep_alive(msg, 60);
	nng_mqtt_msg_set_connect_proto_version(
	    msg, proto_ver);
	nng_mqtt_msg_set_connect_clean_session(msg, true);

	nng_mqtt_set_connect_cb(sock, connect_cb, &sock);
	nng_mqtt_set_disconnect_cb(sock, disconnect_cb, NULL);

	if ((rv = nng_dialer_create(&dialer, sock, url)) != 0) {
		fatal("nng_dialer_create", rv);
	}

	nng_dialer_set_ptr(dialer, NNG_OPT_MQTT_CONNMSG, msg);
	nng_dialer_start(dialer, NNG_FLAG_NONBLOCK);

#if defined(NNG_SUPP_SQLITE)
	nng_thread *thread;
	nng_thread_create(&thread, send_message_interval, &sock);
#endif

	for (i = 0; i < nwork; i++) {
		client_cb(works[i]);
	}

	for (;;) {
		nng_msleep(3600000); // neither pause() nor sleep() portable
	}
#if defined(NNG_SUPP_SQLITE)
	nng_mqtt_free_sqlite_opt(sqlite);
#endif
	return 0;
}

#ifdef NNG_SUPP_TLS
// This reads a file into memory.  Care is taken to ensure that
// the buffer is one byte larger and contains a terminating
// NUL. (Useful for key files and such.)
static void
loadfile(const char *path, void **datap, size_t *lenp)
{
	FILE * f;
	size_t total_read      = 0;
	size_t allocation_size = BUFSIZ;
	char * fdata;
	char * realloc_result;

	if (strcmp(path, "-") == 0) {
		f = stdin;
	} else {
		if ((f = fopen(path, "rb")) == NULL) {
			fprintf(stderr, "Cannot open file %s: %s", path,
			    strerror(errno));
			exit(1);
		}
	}

	if ((fdata = malloc(allocation_size + 1)) == NULL) {
		fprintf(stderr, "Out of memory.");
	}

	while (1) {
		total_read += fread(
		    fdata + total_read, 1, allocation_size - total_read, f);
		if (ferror(f)) {
			if (errno == EINTR) {
				continue;
			}
			fprintf(stderr, "Read from %s failed: %s", path,
			    strerror(errno));
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
			if ((realloc_result = realloc(
			         fdata, allocation_size + 1)) == NULL) {
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

static int
init_dialer_tls(nng_dialer d, const char *cacert, const char *cert,
    const char *key, const char *pass)
{
	nng_tls_config *cfg;
	int             rv;

	if ((rv = nng_tls_config_alloc(&cfg, NNG_TLS_MODE_CLIENT)) != 0) {
		return (rv);
	}

	if (cert != NULL && key != NULL) {
		nng_tls_config_auth_mode(cfg, NNG_TLS_AUTH_MODE_REQUIRED);
		if ((rv = nng_tls_config_own_cert(cfg, cert, key, pass)) !=
		    0) {
			goto out;
		}
	} else {
		nng_tls_config_auth_mode(cfg, NNG_TLS_AUTH_MODE_OPTIONAL);
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

int
tls_client(const char *url, uint8_t proto_ver, const char *ca,
    const char *cert, const char *key, const char *pass)
{
	nng_socket   sock;
	nng_dialer   dialer;
	struct work *works[nwork];
	int          i;
	int          rv;

	if (proto_ver == MQTT_PROTOCOL_VERSION_v5) {
		if ((rv = nng_mqttv5_client_open(&sock)) != 0) {
			fatal("nng_socket", rv);
		}
	} else {
		if ((rv = nng_mqtt_client_open(&sock)) != 0) {
			fatal("nng_socket", rv);
		}
	}

	for (i = 0; i < nwork; i++) {
		works[i] = alloc_work(sock);
	}
	char *verbose_env = getenv("VERBOSE");
    bool verbose = (verbose_env != NULL && strcmp(verbose_env, "1") == 0);
	//CONNECT Message
	// Mqtt connect message
	nng_msg *msg;
	nng_mqtt_msg_alloc(&msg, 0);
	nng_mqtt_msg_set_packet_type(msg, NNG_MQTT_CONNECT);
	nng_mqtt_msg_set_connect_keep_alive(msg, 60);
	nng_mqtt_msg_set_connect_clean_session(msg, true);
	nng_mqtt_msg_set_connect_proto_version(msg, proto_ver);
	nng_mqtt_msg_set_connect_user_name(msg, "emqx");
	nng_mqtt_msg_set_connect_password(msg, "emqx123");

	nng_mqtt_set_connect_cb(sock, connect_cb, &sock);
	nng_mqtt_set_disconnect_cb(sock, disconnect_cb, NULL);

	if ((rv = nng_dialer_create(&dialer, sock, url)) != 0) {
		fatal("nng_dialer_create", rv);
	}

	if ((rv = init_dialer_tls(dialer, ca, cert, key, pass)) != 0) {
		fatal("init_dialer_tls", rv);
	}

	
	if ((rv = nng_dialer_start(dialer, NNG_FLAG_ALLOC)) != 0){
		fatal("nng_dialer_start", rv);
	}

	

	for (i = 0; i < nwork; i++) {
		client_cb(works[i]);
	}
	// Setar a flag para tratamento de interrupção de saída com Ctrl+C
	signal(SIGINT, intHandler);

	for (uint32_t i = 0; i < num_packets && keepRunning; i++) {
        client_publish(sock, g_topic, g_qos, verbose);

        if (interval_ms > 0) {
            nng_msleep(interval_ms); // Espera o intervalo especificado antes de publicar novamente
        }
    }


	for (;;) {
		nng_msleep(3600000); // neither pause() nor sleep() portable
	}
}
#endif

void
usage(void)
{
	printf("mqtt_async: \n");
	printf("	-u <url> \n");
	printf("	-n <number of works> (default: 32)\n");
	printf("	-V <version> The MQTT version used by the client "
	       "(default: 4)\n");
#ifdef NNG_SUPP_TLS
	printf("	-s enable ssl/tls mode (default: disable)\n");
	printf("	-a <cafile path>\n");
	printf("	-c <cert file path>\n");
	printf("	-k <key file path>\n");
	printf("	-p <key password>\n");
#endif
}

int
main(int argc, char **argv)
{
	int    rc;
	char * path;
	size_t file_len;

	bool    enable_ssl = false;
	char *  url        = NULL;
	char *  cafile     = NULL;
	char *  cert       = NULL;
	char *  key        = NULL;
	char *  key_psw    = NULL;
	uint8_t proto_ver  = MQTT_PROTOCOL_VERSION_v311;

	g_topic = "teste_davi";
	g_qos = 0;
	g_verbose = false;
	num_packets = 20;
	interval_ms = 1000;

	int   opt;
	int   digit_optind  = 0;
	int   option_index  = 0;
	char *short_options = "u:V:n:sa:c:k:p:W;";

	static struct option long_options[] = {
		{ "url", required_argument, NULL, 0 },
		{ "version", required_argument, NULL, 0 },
		{ "nwork", no_argument, NULL, 'n' },
		{ "ssl", no_argument, NULL, false },
		{ "cafile", required_argument, NULL, 0 },
		{ "cert", required_argument, NULL, 0 },
		{ "key", required_argument, NULL, 0 },
		{ "psw", required_argument, NULL, 0 },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 },
	};

	while ((opt = getopt_long(argc, argv, short_options, long_options,
	            &option_index)) != -1) {
		switch (opt) {
		case 0:
			// TODO
			break;
		case '?':
		case 'h':
			usage();
			exit(0);
		case 'u':
			url = argv[optind - 1];
			break;
		case 'V':
			proto_ver = atoi(argv[optind - 1]);
			break;
		case 'n':
			nwork = atoi(argv[optind - 1]);
			break;
		case 's':
			enable_ssl = true;
			break;
#ifdef NNG_SUPP_TLS
		case 'a':
			path = argv[optind - 1];
			loadfile(path, (void **) &cafile, &file_len);
			break;
		case 'c':
			path = argv[optind - 1];
			loadfile(path, (void **) &cert, &file_len);
			break;
		case 'k':
			path = argv[optind - 1];
			loadfile(path, (void **) &key, &file_len);
			break;
		case 'p':
			key_psw = argv[optind - 1];
			break;
#endif
		default:
			fprintf(stderr, "invalid argument: '%c'\n", opt);
			usage();
			exit(1);
		}
	}

	if (url == NULL) {
		url = "mqtt-tcp://broker.emqx.io:1883";
		printf("set default url: '%s'\n", url);
	}

	if (enable_ssl) {
#ifdef NNG_SUPP_TLS
		tls_client(url, proto_ver, cafile, cert, key, key_psw);
#else
		fprintf(stderr, "tls client: Not supported \n");
#endif

	} else {
		client(url, proto_ver);
	}

	return 0;
}