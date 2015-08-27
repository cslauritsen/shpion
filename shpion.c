#define _GNU_SOURCE
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
 
#define INTERVAL_MASK	0xffffffffffffffc0L

#define QOS         1
#define TIMEOUT     10000L

static MQTTClient client;
int debug;

static char * gitemail() {
	FILE *file = NULL;
	char buf[1024];
	file = fopen("/u01/pnp/.gitconfig", "r");
	while(fgets(buf, sizeof(buf), file)) {
		char* newlinepos = index(buf, '\n');
		if (newlinepos) {
			*newlinepos = '\0';
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

static char * sha1_to_str(unsigned char* inbuf, int inlen) {
	char *outbuf = NULL;
	unsigned char hash[20];
	int j=0;
	int k=0;
	SHA1(inbuf, inlen, hash);
	outbuf = (char*) malloc(sizeof(hash)*2 + 1); // holds hex representation of bytes
	memset(outbuf, 0, sizeof(hash)*2 + 1);
	for (j=0; j < sizeof(hash); j++) {
		sprintf(outbuf+k, "%02x", hash[j]);
		k+=2;
	}
	return outbuf;
}

void handler (int sig)
{
  MQTTClient_disconnect(client, 10000);
  MQTTClient_destroy(&client);
  printf ("\nexiting...(%d)\n", sig);
  exit (0);
}
 
void perror_exit (char *error)
{
  perror (error);
  handler (9);
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
	struct input_event ev[64];
	int fd, rd, size = sizeof (struct input_event);
	char name[256] = "Unknown";
	char *device = NULL;
	long last_interval = -1;
	long event_count = 0;

	signal(SIGKILL, handler);
	signal(SIGTERM, handler);
 
	debug = 0;
	//Setup check
	//
	if (argc < 3) {
		printf("Usage: %s device mqttserver\n", argv[0]);
		exit(2);
	}
	if (argv[1] == NULL){
		printf("Please specify (on the command line) the path to the dev event interface devicen");
		exit (0);
	}
 
	const char *address = argv[2];

	if ((getuid ()) != 0)
		printf ("You are not root! This may not work...n");
 
	if (argc > 1)
		device = argv[1];
 
	//Open Device
	if ((fd = open (device, O_RDONLY)) == -1)
		printf ("%s is not a vaild device.n", device);
 
	//Print Device Name
	ioctl (fd, EVIOCGNAME (sizeof (name)), name);
	printf ("Reading From : %s (%s)n", device, name);

    	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    	MQTTClient_message pubmsg = MQTTClient_message_initializer;
    	MQTTClient_deliveryToken token;

    	int rc;

	char *email = gitemail();
	char *emailHash = sha1_to_str((unsigned char*) email, strlen(email));
	char *topicStr = NULL;
	asprintf(&topicStr, "shpion/%s", emailHash);
	printf("publishing to %s\n", topicStr);


    	MQTTClient_create(&client, address, (const char*) email, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    	conn_opts.keepAliveInterval = 20; 
	conn_opts.cleansession = 1;

	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        	printf("Failed to connect, return code %d\n", rc);
        	exit(-1);       
	}
	else {
		char  msgBuf[100];
		time_t now;
		time(&now);
		ctime_r(&now, msgBuf);
		pubmsg.payload = msgBuf;
		pubmsg.payloadlen = strlen(msgBuf);
		pubmsg.qos = QOS;
		pubmsg.retained = 0;
		MQTTClient_publishMessage(client, "shpion/started", &pubmsg, &token);
	}

	while (1) {
		int i=0;
		if ((rd = read (fd, ev, size * 64)) < size) {
			perror_exit ("read()");      
		}

		for (i=0; i < rd/size; i++) {
			if (0x4 == ev[i].type) {
				long this_interval = ev[i].time.tv_sec & INTERVAL_MASK;
				if (-1 == last_interval || this_interval != last_interval) {
					if (debug) {
						printf("\n========================================%ld >> %ld\n"
							, this_interval, event_count);
					}
					char * msgBuf = NULL;
					asprintf(&msgBuf, "%ld: %ld", this_interval, event_count);
					if (msgBuf) {
						pubmsg.payload 		= msgBuf;
						pubmsg.payloadlen 	= strlen(msgBuf);
						pubmsg.qos 		= QOS;
						pubmsg.retained 	= 0;
						MQTTClient_publishMessage(client, (const char*) topicStr, &pubmsg, &token);
						free(msgBuf);
					}
					
					event_count = 0;
				}
				if (debug) printf ("%ld %ld.%ld sz=%d rd=%d Code=%d val=%02x typ=%x\n"
					, this_interval
					, ev[i].time.tv_sec
					, ev[i].time.tv_usec
					, size, rd, (ev[i].code), ev[i].value, ev[i].type);
				last_interval = this_interval;
				event_count++;
			}
		}
	}

	if (email) {
		free(email);
		email = NULL;
	}

	if (emailHash) {
		free(emailHash);
		emailHash = NULL;
	}

	if (topicStr) {
		free(topicStr);
		topicStr = NULL;
	}
 
	return 0;
}
