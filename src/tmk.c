#include <config.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <termios.h>
#include <signal.h>
#include <alsa/asoundlib.h>

static snd_seq_t *handle;
static int tmk_client;
static int tmk_port = 0;
static int dest_client;
static int dest_port;
static snd_seq_event_t *ev;
static int queue;
static int output_ret;
static unsigned char octave = 5;

static struct termios orig_term;
static struct termios raw;
static char * cur_kb_mode;

static void send_note_on(unsigned char note) {
	ev->type = SND_SEQ_EVENT_NOTEON;
	ev->data.note.note = note;
	ev->data.note.velocity = 127;
	snd_seq_ev_set_subs(ev);
	snd_seq_event_output_direct(handle, ev);
}

static void send_note_off(unsigned char note) {
	ev->type = SND_SEQ_EVENT_NOTEOFF;
	ev->data.note.note = note;
	ev->data.note.velocity = 0;
	snd_seq_ev_set_subs(ev);
	snd_seq_event_output_direct(handle, ev);
}

static void seq_init() {
	snd_seq_open(&handle, "default", SND_SEQ_OPEN_OUTPUT, 0);
	if (handle == NULL) {
		printf("Could not allocate sequencer.\r\n");
		exit(1);
	}
	snd_seq_set_client_name(handle, "tmk");
	snd_seq_create_simple_port(handle, "Output",
			SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
			SND_SEQ_PORT_TYPE_MIDI_GENERIC);
}

static void terminal_setup() {
	tcgetattr(STDIN_FILENO, &orig_term);

	raw = orig_term;
	raw.c_lflag &= ~(ECHO | ICANON);
	raw.c_iflag = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	cur_kb_mode = (char *) K_RAW;
	ioctl(STDIN_FILENO, KDSKBMODE, cur_kb_mode);
}

static void terminal_teardown() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
        ioctl(STDIN_FILENO, KDSKBMODE, K_XLATE);
}

static void do_sigcont() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	ioctl(0, KDSKBMODE, cur_kb_mode);
}

static void do_exit() {
	terminal_teardown();
	_exit(0);
}

static void usage(char *cmd) {
	printf("TTY MIDI Keyboard\r\n");
	printf("Usage:\r\n");
	printf("\t%s [dest_client:dest_port]\r\n", cmd);
	printf("\t%s (-h | --help)\r\n", cmd);
	fflush(stdout);
}

int main(int argc, char *argv[])
{
        unsigned char in_ch;
	terminal_setup();
	signal(SIGINT, do_exit);
	signal(SIGTERM, do_exit);
	signal(SIGSTOP, terminal_teardown);
	signal(SIGCONT, do_sigcont);
	seq_init();
	if (argc > 2) {
		printf("Too many arguments.\r\n");
		usage(argv[0]);
		do_exit();
	}
	if (argc == 2) {
		if (sscanf(argv[1],"%d:%d",&dest_client,&dest_port) == 2) {
			tmk_client = snd_seq_client_id(handle);
			snd_seq_addr_t tmk, dest;
			snd_seq_port_subscribe_t *sub;
			tmk.client = tmk_client;
			tmk.port = tmk_port;
			dest.client = dest_client;
			dest.port = dest_port;
			snd_seq_port_subscribe_alloca(&sub);
			snd_seq_port_subscribe_set_sender(sub, &tmk);
			snd_seq_port_subscribe_set_dest(sub, &dest);
			if (snd_seq_subscribe_port(handle, sub) < 0) {
				printf("Error connecting to midi client: %s\r\n", argv[1]);
				fflush(stdout);
				do_exit();
			}
		}
		else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
			usage(argv[0]);
			do_exit();
		}
		else {
			printf("Unknown argument: %s\r\n", argv[1]);
			usage(argv[0]);
			do_exit();
		}
	}
	ev = malloc(sizeof(snd_seq_event_t));
	if (ev == NULL) {
		printf("Could not allocate midi event.\r\n");
		fflush(stdout);
		do_exit();
	}
	snd_seq_ev_set_direct(ev);
	snd_seq_ev_set_source(ev, tmk_port);
	ev->data.note.channel = 1;
	ev->data.note.velocity = 127;
        while(1) {
        	read(STDIN_FILENO, &in_ch, 1);
		switch(in_ch) {
		case 0xac: //octave down (z release)
			if (octave == 0)
				break;
			octave--;
			break;
		case 0xad: //octave up (x release)
			if (octave == 10)
				break;
			octave++;
			break;
		case 0x1e: // C
			send_note_on(12 * octave);
			break;
		case 0x9e:
			send_note_off(12 * octave);
			break;
		case 0x11:
			send_note_on(12 * octave + 1);
			break;
		case 0x91:
			send_note_off(12 * octave + 1);
			break;
		case 0x1f:
			send_note_on(12 * octave + 2);
			break;
		case 0x9f:
			send_note_off(12 * octave + 2);
			break;
		case 0x12: // Eb (e key)
			send_note_on(12 * octave + 3);
			break;
		case 0x92:
			send_note_off(12 * octave + 3);
			break;
		case 0x20: // E (d key)
			send_note_on(12 * octave + 4);
			break;
		case 0xa0:
			send_note_off(12 * octave + 4);
			break;
		case 0x21:
			send_note_on(12 * octave + 5);
			break;
		case 0xa1:
			send_note_off(12 * octave + 5);
			break;
		case 0x14:
			send_note_on(12 * octave + 6);
			break;
		case 0x94:
			send_note_off(12 * octave + 6);
			break;
		case 0x22:
			send_note_on(12 * octave + 7);
			break;
		case 0xa2:
			send_note_off(12 * octave + 7);
			break;
		case 0x15: // G# (y key)
			send_note_on(12 * octave + 8);
			break;
		case 0x95:
			send_note_off(12 * octave + 8);
			break;
		case 0x23:
			send_note_on(12 * octave + 9);
			break;
		case 0xa3:
			send_note_off(12 * octave + 9);
			break;
		case 0x16:
			send_note_on(12 * octave + 10);
			break;
		case 0x96:
			send_note_off(12 * octave + 10);
			break;
		case 0x24:
			send_note_on(12 * octave + 11);
			break;
		case 0xa4:
			send_note_off(12 * octave + 11);
			break;
		case 0x25:
			send_note_on(12 * octave + 12);
			break;
		case 0xa5:
			send_note_off(12 * octave + 12);
			break;
		case 0x18:
			send_note_on(12 * octave + 13);
			break;
		case 0x98:
			send_note_off(12 * octave + 13);
			break;
		case 0x26:
			send_note_on(12 * octave + 14);
			break;
		case 0xa6:
			send_note_off(12 * octave + 14);
			break;
		case 0x19:
			send_note_on(12 * octave + 15);
			break;
		case 0x99:
			send_note_off(12 * octave + 15);
			break;
		case 0x27:
			send_note_on(12 * octave + 16);
			break;
		case 0xa7:
			send_note_off(12 * octave + 16);
			break;
		case 0x28:
			send_note_on(12 * octave + 17);
			break;
		case 0xa8:
			send_note_off(12 * octave + 17);
			break;
		case 0x90:
			printf("Exiting\r\n");
			fflush(stdout);
			do_exit();
			break;
		case 0x0f:
			printf("Pausing\r\n");
			cur_kb_mode = (char *) K_XLATE;
			ioctl(STDIN_FILENO, KDSKBMODE, K_XLATE);
			int done = 0;
			while(1) {
				read(STDIN_FILENO, &in_ch , 1);
				switch(in_ch) {
				case '\t':
			 		done = 1;
					break;
				case 'q':
					printf("Exiting\r\n");
					fflush(stdout);
					do_exit();
					break;
				default:
					break;
				}
				if (done) {
					break;
				}
			}
			printf("Resuming\r\n");
			cur_kb_mode = (char *) K_RAW;
			ioctl(STDIN_FILENO, KDSKBMODE, K_RAW);
			break;
		defualt:
			break;
		}
	}
}
