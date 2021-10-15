#include <alsa/asoundlib.h>
#include <wiringPi.h>
#include <limits.h>
#include <unistd.h>

static snd_seq_t *seq_handle;
static int in_port;
extern char *optarg;

#define THRUPORTCLIENT 14
#define THRUPORTPORT 0

#define NUM_PINS 16

// NB: States are flipped because relays use LOW to switch on
#define PIN_STATE_ON	0
#define PIN_STATE_OFF	1

int pinMapping[NUM_PINS];

void init(int argc, char *argv[])
{
	int opt;
	int client	= 14;
	int port	= 0;

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

	snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);

	for(int i = 0; i < NUM_PINS; i++)
	{
		pinMode(pinMapping[i], OUTPUT);
		digitalWrite(pinMapping[i], PIN_STATE_OFF);
	}

	snd_seq_set_client_name(seq_handle, "LightOrgan");
	in_port = snd_seq_create_simple_port(
		seq_handle,
		"listen:in",
		SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
		SND_SEQ_PORT_TYPE_APPLICATION);

	if (snd_seq_connect_from(seq_handle, in_port, client, port) == -1)
	{
		perror("Can't connect to port");
		exit(-1);
	}
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
	/*if( daemon(0,0) != 0)
	{
		perror("Failed to start daemon");
		exit(1);
	}*/

	printf("Initializing wiringPi\n");

	if(wiringPiSetup() == -1)
	{
		perror("Failed to start wiringPi");
		exit(1);
	}

	printf("Initializing MIDI interface\n");

	init(argc, argv);

	printf("Listening for events\n");
	
	while(1)
	{
		midi_process(midi_read());
	}

	printf("Closing");

	return -1;
}