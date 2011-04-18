/*
 * managerui - user interface
 * Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
 * Dedicated to the Public Domain
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

/*
 * Keygrabber
 * The keygrabber functions open a device interface to receive keyboard or
 * other events. They use the linux event interface (evdev) and return the
 * key events.
 *
 * ev_open: Opens a device for reading.
 * ev_read: Reads a single event from a device and returns it.
 * ev_close: Closes a previously opened device.
 */

/*
 * Open Keygrabber
 * This opens a linux character device (/dev/input/event*) and returns
 * a valid file descriptor.
 * Returns a negative integer on failure:
 */
static signed int ev_open(const char *device)
{
	return open(device, O_RDONLY);
}

/*
 * Read Event from Keygrabber
 * Tries to read an event from a previously opened linux character device. It returns
 * the key number on success or an error.
 *
 * The returned integer contains the 16bit unsigned integer key code in the lower 16bit and
 * several flags in the higher 16bits.
 * Flags:
 *  EV_ERROR: If set an error occured and the keycode has to be interpreted as error code.
 *  EV_UP: If set the key is released.
 *  EV_HOLD: If set the key is hold.
 *
 * If a key is pressed, neither of *_UP and *_HOLD are set. If the key is then hold, all
 * further key repeat events have the *_HOLD flag set. The time between the *_HOLD events is
 * configured by the system wide key-repeat settings. The last event has the *_UP flag
 * set.
 * Some keys send only *_UP events.
 *
 * Macros:
 *  EV_HASFLAG(code, flag) returns 0 if the given flag is not set in the given code, otherwise
 *                             a positive integer is returned.
 *  EV_GETCODE(code) extracts the keycode as 16bit integer from the given code.
 *  EV_GETERROR(code) same as EV_GETCODE().
 *  EV_FAIL(code) same as EV_HASFLAG(code, EV_ERROR)
 */
#define EV_ERROR 0x00010000
#define EV_UP    0x00020000
#define EV_HOLD  0x00040000

#define EV_EBLOCK 0x0001
#define EV_EFILE  0x0002

#define EV_HASFLAG(code, flag) ((code) & (flag))
#define EV_GETCODE(code) ((code) & 0x0000ffff)
#define EV_GETERROR(code) EV_GETCODE(code)
#define EV_FAIL(code) EV_HASFLAG((code), EV_ERROR)

static uint32_t ev_read(signed int fd)
{
	struct input_event ev;

	if (read(fd, &ev, sizeof(ev)) < 1) {
		if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
			return EV_EBLOCK | EV_ERROR;
		}
		else {
			close(fd);
			return EV_EFILE | EV_ERROR;
		}
	}

	if (ev.type == EV_KEY) {
		if (ev.value == 1)
			return ev.code;
		else if (ev.value == 2)
			return ev.code | EV_HOLD;
		else
			return ev.code | EV_UP;
	}

	return EV_EBLOCK | EV_ERROR;
}

/*
 * Close Keygrabber
 * This closes a previously opened linux character device.
 */
static void ev_close(signed int fd)
{
    close(fd);
}

int main(int argc, char **argv)
{
	signed int keygrabber;
	uint32_t ev;

	if (argc < 2) {
		fprintf(stderr, "usage: managerui /dev/input/eventX\n");
		return EXIT_FAILURE;
	}

	keygrabber = ev_open(argv[1]);
	if (keygrabber < 0) {
		fprintf(stderr, "Cannot open event file\n");
		return EXIT_FAILURE;
	}

	if (daemon(0, 0) != 0) {
		fprintf(stderr, "Cannot fork into background\n");
		return EXIT_FAILURE;
	}

	while (1) {
		ev = ev_read(keygrabber);
		if (EV_FAIL(ev)) {
			if (EV_GETERROR(ev) == EV_EBLOCK)
				continue;
			else
				break;
		}
		switch (EV_GETCODE(ev)) {
		case 225:
			if (EV_HASFLAG(ev, EV_UP))
				system("setbacklight %+10");
			break;
		case 224:
			if (EV_HASFLAG(ev, EV_UP))
				system("setbacklight %-10");
			break;
		default:
			break;
		}
	}

	ev_close(keygrabber);
	return EXIT_SUCCESS;
}
