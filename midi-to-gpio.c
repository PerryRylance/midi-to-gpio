#include <alsa/asoundlib.h>
#include <wiringPi.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>

static snd_seq_t *seq_handle;
static int in_port;
extern char *optarg;

#define THRUPORTCLIENT 14
#define THRUPORTPORT 0

#define NUM_PINS 16

// NB: States are flipped because relays use LOW to switch on
#define PIN_STATE_ON	0
#define PIN_STATE_OFF	1

#define ENUM_DEVICES_BUFFER_SIZE 2048
#define MIDI_PLAYBACK_COMMAND_BUFFER_SIZE 4096

int pinMapping[NUM_PINS];

void init(int argc, char *argv[])
{
	int opt;
	int client	= 14;
	int port	= 0;

	printf("Enumerating devices\r\n");

	char *cmd = "aconnect -i";
	char buffer[ENUM_DEVICES_BUFFER_SIZE];
	FILE *fp;
	int scanned;

	if((fp = popen(cmd, "r")) == NULL)
		printf("Couldn't enumerate devices\r\n");
	else
	{
		while(fgets(buffer, ENUM_DEVICES_BUFFER_SIZE, fp) != NULL)
		{
			if(sscanf(buffer, "client %d", &scanned))
				client = scanned;
			else if(sscanf(buffer, "\t%d", &scanned))
				port = scanned;
		}

		pclose(fp);
	}

	printf("Parsing command line arguments\r\n");

	while ((opt = getopt(argc, argv, "c:p:")) != -1)
	{
		switch (opt)
		{
			case 'c':
				client = atoi(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			default:
				break;
		}
	}

	printf("Configuring pins\r\n");

	pinMapping[0] = 0;
	pinMapping[1] = 1;
	pinMapping[2] = 2;
	pinMapping[3] = 3;
	pinMapping[4] = 4;
	pinMapping[5] = 5;
	pinMapping[6] = 6;
	pinMapping[7] = 7;
	pinMapping[8] = 21;
	pinMapping[9] = 22;
	pinMapping[10] = 23;
	pinMapping[11] = 24;
	pinMapping[12] = 25;
	pinMapping[13] = 26;
	pinMapping[14] = 27;
	pinMapping[15] = 28;

	for(int i = 0; i < NUM_PINS; i++)
	{
		pinMode(pinMapping[i], OUTPUT);
		digitalWrite(pinMapping[i], PIN_STATE_OFF);
	}

	printf("Opening sound sequencer\r\n");
	snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);

	snd_seq_set_client_name(seq_handle, "AirHornPiano");
	in_port = snd_seq_create_simple_port(
		seq_handle,
		"listen:in",
		SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
		SND_SEQ_PORT_TYPE_APPLICATION);

	printf("Connecting to %d:%d\r\n", client, port);

	if (snd_seq_connect_from(seq_handle, in_port, client, port) == -1)
	{
		perror("Can't connect to port");
		exit(-1);
	}

	printf("Listening on %d:%d\r\n", client, port);

	if(client == 14 && port == 0)
	{
		// NB: Pretty awful way to give the device time to mount. Figure something better out please
		sleep(2);

		printf("Looking for MIDI files on /media/pi/**\r\n");

		cmd = "ls /media/pi/**/*.mid -R";

		if((fp = popen(cmd, "r")) == NULL)
			printf("Couldn't list \r\n");
		else
		{
			char *backslash = "/";
			char midiPlaybackCommand[MIDI_PLAYBACK_COMMAND_BUFFER_SIZE];

			while(fgets(buffer, ENUM_DEVICES_BUFFER_SIZE, fp) != NULL)
			{
				if(strncmp(buffer, backslash, strlen(backslash)) == 0)
				{
					printf("Playing %s", buffer);

					sprintf(midiPlaybackCommand, "aplaymidi -p 14:0 %s", buffer);
					popen(midiPlaybackCommand, "r");
				}
				else
				{
					printf("No MIDI files found\r\n");
				}

				break;
			}

			pclose(fp);
		}
	}
}

void shutdown()
{
	unlink("./pid");
}

void manage_process()
{
	printf("Checking for existing process\r\n");

	int pid;
	FILE *fh;

	if(access( "./pid", F_OK ) == 0) // NB: 0 is OK
	{
		fh = fopen("./pid", "r");
  		fscanf(fh, "%d", &pid); 
		fclose(fh);

		printf("Killing pid %d\r\n", pid);

		kill(pid, SIGTERM);
		sleep(1);
	}

	pid = getpid();

	printf("Writing pid %d\r\n", pid);

	fh = fopen("./pid", "w");
	fprintf(fh, "%d", pid);
	fclose(fh);
}

snd_seq_event_t *midi_read(void)
{
	snd_seq_event_t *ev = NULL;
	snd_seq_event_input(seq_handle, &ev);
	return ev;
}

void midi_process(snd_seq_event_t *ev)
{
	// printf("%d", ev->type);
	// printf("\n");

	if(!(ev->type == SND_SEQ_EVENT_NOTEON || ev->type == SND_SEQ_EVENT_NOTEOFF))
	{
		// printf("Ignoring event\n");

		snd_seq_free_event(ev);

		return; // NB: We only care about note on and off events
	}
	
	int midiPitch = ev->data.note.note % 24;

	while(midiPitch >= NUM_PINS)
		midiPitch -= 12;

	int pinIndex = pinMapping[midiPitch];
	
	// NB: According to the MIDI spec, note on with velocity 0 is to be treated as note off. Check this here.
	if(ev->type == SND_SEQ_EVENT_NOTEON && ev->data.note.velocity > 0)
	{
		// printf("Pin index %d ON\n", pinIndex);
		digitalWrite(pinIndex, PIN_STATE_ON);
	}
	else
	{
		// printf("Pin index %d OFF\n", pinIndex);
		digitalWrite(pinIndex, PIN_STATE_OFF);
	}
	
	snd_seq_free_event(ev);
}

int main(int argc, char *argv[])
{
	printf("Starting wiringPi\r\n");

	if(wiringPiSetup() == -1)
	{
		perror("Failed to start wiringPi");
		exit(1);
	}

	manage_process();
	init(argc, argv);

	printf("Starting MIDI processing loop\r\n");

	while(1)
	{
		midi_process(midi_read());
	}

	shutdown();

	return -1;
}