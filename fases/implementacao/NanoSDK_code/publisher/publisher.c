// Inclua os headers necessários
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

static nng_socket * g_sock;
#define CONN 1
#define SUB 2
#define PUB 3

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



int
client(int type, const char *url, const char *qos, const char *topic, const char *data, const char *numero_pub)
{
	nng_socket  sock;
	int         rv, sz, q, qpub, i;
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
	    0 != nng_mqtt_quic_set_msg_send_cb(&sock, msg_send_cb, (void *)arg)) {
		printf("error in quic client cb set.\n");
	}
	g_sock = &sock;
	// MQTT Connect...
	msg = mqtt_msg_compose(CONN, 0, NULL, NULL);
	nng_sendmsg(sock, msg, NNG_FLAG_ALLOC);


	if (qos) {
		q = atoi(qos);
		if (q < 0 || q > 2) {
			printf("Qos should be in range(0~2).\n");
			q = 0;
		}
	}

	if (numero_pub){

		qpub = atoi(numero_pub);
		for(i=0;i<qpub;i++){
		msg = mqtt_msg_compose(PUB, q, (char *)topic, (char *)data);
		nng_sendmsg(*g_sock, msg, NNG_FLAG_ALLOC);
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

        	
	nng_msleep(10);
	nng_close(sock);
	fprintf(stderr, "Done.\n");

	return (0);

}

static void
printf_helper(char *exec)
{
	fprintf(stderr, "Usage: %s <url> <qos> <topic> <data>\n", exec);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        goto error;
    }
    client(PUB, argv[1], argv[2], argv[3], argv[4], argv[5]);
    
    return 0;


error:

	printf_helper(argv[0]);
	return 0;
}
