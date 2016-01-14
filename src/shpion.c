#include <ctype.h>
#include <sys/poll.h>
#include <MQTTClient.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <termios.h>
#include <signal.h>
#include <openssl/sha.h>
 
#define REPORTING_INTERVAL_MASK		((unsigned long) 0xffffffffffffff80L)
#define REPORTING_INTERVAL_SECONDS 	(1 + ~(REPORTING_INTERVAL_MASK))
#define EVBUFSZ				64
#define SHA1SUMLEN 20

#define QOS         1
#define TIMEOUT     10000L


static volatile int exit_request = 0;

static char *email = NULL;
static char emailHash[2 * SHA1SUMLEN + 1];

static MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
int debug;
int event_count;

static char * gitemail() {
	FILE *file = NULL;
	char buf[1024];
	file = fopen("/u01/pnp/.gitconfig", "r");
	while(fgets(buf, sizeof(buf), file)) {
		char* newlinepos = index(buf, '\n');
		while(newlinepos && isspace(*newlinepos)) {
			*newlinepos-- = '\0';
		}
		char *c = strstr(buf, "email = ");
		if (c) {
			char *ret = NULL;
			asprintf(&ret, "%s", c+strlen("email = "));
			return ret;
		}
	}
	fclose(file);
	return NULL;
}

static char * sha1_to_str(char* outbuf, unsigned char* inbuf, int inlen) {
	unsigned char hash[20];
	int j=0;
	int k=0;
	SHA1(inbuf, inlen, hash);
	memset(outbuf, 0, sizeof(hash)*2 + 1);
	for (j=0; j < sizeof(hash); j++) {
		sprintf(outbuf+k, "%02x", hash[j]);
		k+=2;
	}
	return outbuf;
}

static void reportactivity(const char* topicStr, long this_interval) {
	if (!exit_request) {
		if (MQTTClient_isConnected(client) || (MQTTClient_connect(client, &conn_opts)) == MQTTCLIENT_SUCCESS) {
    			MQTTClient_deliveryToken token;
    			MQTTClient_message pubmsg = MQTTClient_message_initializer;
			char * msgBuf = NULL;
			asprintf(&msgBuf, "%ld,%d", this_interval, event_count);
			if (msgBuf) {
				pubmsg.payload 		= msgBuf;
				pubmsg.payloadlen 	= strlen(msgBuf);
				pubmsg.qos 		= QOS;
				pubmsg.retained 	= 0;
				MQTTClient_publishMessage(client, (const char*) topicStr, &pubmsg, &token);
				free(msgBuf);
			}
	
			if (debug) {
				printf("put msg event count: %d\n", event_count);
			}
			event_count = 0;
		}
		else {
			printf("WARNING: Connection failed to MQTT\n");
		}
	}
}

static void handler (int sig) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long this_interval = tv.tv_sec & REPORTING_INTERVAL_MASK;
    event_count = -1;
    char *t = NULL;
    asprintf(&t, "shpion/vmstop/%s", emailHash);
    reportactivity(t, this_interval);
    free(t);
    MQTTClient_disconnect(client, 2);
    MQTTClient_destroy(&client);

	if (email) {
		free(email);
		email = NULL;
	}
    exit_request=1;
    printf ("\nexiting...(%d)\n", sig);
}
 
volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int i;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

 
int main (int argc, char *argv[]) {
	struct input_event input_events[EVBUFSZ];
	int mfd, kfd, bytes_read;
	long last_interval = -1;

	sigset_t mysigs;
	struct sigaction act;
	struct timespec ts = {REPORTING_INTERVAL_SECONDS , 0}; // report every x seconds (where x matches the reporting mask)

	memset(emailHash, 0, sizeof(emailHash));

	memset(&act, 0, sizeof(act));
	act.sa_handler = handler;
	if (sigaction(SIGINT, &act, NULL)) {
		perror("sigaction");
		return 1;
	}
	if (sigaction(SIGTERM, &act, NULL)) {
		perror("sigaction");
		return 1;
	}

	sigemptyset(&mysigs);
	sigaddset(&mysigs, SIGINT);
	sigaddset(&mysigs, SIGTERM);

	debug = 1;
	if (argc < 2) {
		printf("Usage: %s mqttserver\n", argv[0]);
		exit(2);
	}
	if (argv[1] == NULL){
		printf("Please specify the mqttserver\n");
		exit (0);
	}

	const char *address = argv[1];

	if ((getuid ()) != 0)
		printf ("You are not root! This may not work...n");
 
 
	//Open Devices
	if ((kfd = open ("/dev/input/event2", O_RDONLY)) == -1) {
		perror("Failed to open device1");
		exit(2);
	}
	if ((mfd = open ("/dev/input/mouse0", O_RDONLY)) == -1) {
		perror("Failed to open device2");
		exit(2);
	}

 
    	int rc;

	email = gitemail();
    sha1_to_str(emailHash, (unsigned char*) email, strlen(email));
	char *topicStr = NULL;
	asprintf(&topicStr, "shpion/vmact/%s", emailHash);
	printf("publishing to %s\n", topicStr);

    MQTTClient_create(&client, address, (const char*) email, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

	conn_opts.keepAliveInterval = 20; 
	conn_opts.cleansession = 1;

	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        	printf("Failed to connect, return code %d\n", rc);
	}
	else {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		long this_interval = tv.tv_sec & REPORTING_INTERVAL_MASK;
		event_count = -1;
		char *t = NULL;
		asprintf(&t, "shpion/vmstart/%s", emailHash);
		reportactivity(t, this_interval);
		free(t);
	}

	#define POLL_FD_COUNT 2
	struct pollfd pfds[POLL_FD_COUNT];
	unsigned char buf[3*10];
	while (!exit_request) {

		pfds[0].fd = kfd;
		pfds[0].events = POLLIN;

		pfds[1].fd = mfd;
		pfds[1].events = POLLIN;

		int err = ppoll(pfds, POLL_FD_COUNT, &ts, &mysigs);

		if(EINTR != err) {
			bytes_read = 0;
			int kb_events_read = 0;
			int mouse_events_read = 0;
			// Keyboard events fill in struct input_event
			if (pfds[0].revents & POLLIN) {
				bytes_read += read(pfds[0].fd, input_events + kb_events_read, sizeof(struct input_event) * (EVBUFSZ-kb_events_read));
				kb_events_read += bytes_read / sizeof(struct input_event);
				event_count += kb_events_read;
			}

			// mouse events are 3-byte thingies
			if (pfds[1].revents & POLLIN) {
				bytes_read += read(pfds[1].fd, buf, sizeof(buf));
				mouse_events_read += bytes_read / 3;
				event_count += mouse_events_read;
			}

			if (0 == mouse_events_read && 0 == kb_events_read) {
				struct timeval tv;
				gettimeofday(&tv, NULL);
				long this_interval = tv.tv_sec & REPORTING_INTERVAL_MASK;
				reportactivity(topicStr, this_interval);
			}

			for (int i=0; i < kb_events_read; i++) {
				long this_interval = input_events[i].time.tv_sec & REPORTING_INTERVAL_MASK;
				if (debug) printf ("%ld %ld.%ld sz=%ld kbd_events_read=%d\n"
					, this_interval
					, input_events[i].time.tv_sec
					, input_events[i].time.tv_usec
					, sizeof(struct input_event)
					, kb_events_read
					);
				if (this_interval != last_interval) {
					reportactivity(topicStr, this_interval);
				}
				last_interval = this_interval;
			}
			for (int i=0; i < mouse_events_read; i++) {
				struct timeval tv;
				gettimeofday(&tv, NULL);
				long this_interval = tv.tv_sec & REPORTING_INTERVAL_MASK;
				if (debug) printf ("%ld %ld.%ld mouse_events_read=%d\n"
					, this_interval
					, tv.tv_sec
					, tv.tv_usec
					, mouse_events_read);
				if (this_interval != last_interval) {
					reportactivity(topicStr, this_interval);
				}
				last_interval = this_interval;
			}
		}
	}


	if (topicStr) {
		free(topicStr);
		topicStr = NULL;
	}
 
	puts("Finished.");
	return 0;
}
