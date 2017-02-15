#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>

int testread(int fd, uint32_t addr, uint32_t *value)
{
	int rc;

	lseek(fd, addr, SEEK_SET);
	rc = read(fd, value, 4);
	if (rc >= 0)
		rc = 0;

	return rc;
}

int testwrite(int fd, uint32_t addr, uint32_t value)
{
	int rc;

	lseek(fd, addr, SEEK_SET);
	rc = write(fd, &value, 4);
	if (rc >= 0)
		rc = 0;

	return rc;
}

#define NUM_WRITES	1
#define NUM_READS	9
#define NUM_HUB_READS	1

int main(int argc, char **argv)
{
	int rc = 0;
	int i, fd, option;
	int skip_write = 0;
	int verbose = 0;
	int hub = 0;
	char path[64] = "/sys/bus/platform/devices/fsi-master/slave@00:00/raw";
	uint32_t value;
	const uint32_t write_addresses[NUM_WRITES] = {
		0x818,
	};
	const uint32_t addresses[NUM_READS] = {
		0x800,	/* smode */
		0x818,	/* si1m */
		0x81C,	/* si1s */
		0x101C,	/* scom status */
		0x1028,	/* scom chipid */
		0x1808,	/* i2c mode */
		0x181C,	/* i2c status */
		0x3400,	/* hub mmode */
		0x35D0,	/* hub status */
	};
	const uint32_t hub_addresses[NUM_HUB_READS] = {
		0x100800,	/* smode */
	};

	while ((option = getopt(argc, argv, "vsb:x")) != -1) {
		switch (option) {
		case 'v':
			verbose = 1;
			break;
		case 's':
			skip_write = 1;
			break;
		case 'b':
			strncpy(path, optarg, 64);
			break;
		case 'x':
			hub = 1;
			break;
		default:
			printf("unknown option\n");
		}
	}

	fd = open(path, O_RDWR);
	if (fd < 0) {
		printf("FAILED - failed to open %s\n", path);
		return -ENODEV;
	}

	for (i = 0; i < NUM_READS; ++i) {
		rc = testread(fd, addresses[i], &value);
		if (rc) {
			printf("FAILED - failed to read %08x\n", addresses[i]);
			goto exit;
		}

		if (verbose)
			printf("read %08x: %08x\n", addresses[i], value);
	}

	if (!skip_write) {
		for (i = 0; i < NUM_WRITES; ++i) {
			value = 0;
			rc = testread(fd, write_addresses[i], &value);
			if (rc) {
				printf("FAILED - failed to read %08x\n",
				       write_addresses[i]);
				goto exit;
			}

			rc = testwrite(fd, write_addresses[i], value);
			if (rc) {
				printf("FAILED - failed to write %08x\n",
				       write_addresses[i]);
				goto exit;
			}

			if (verbose)
				printf("wrote %08x: %08x\n",
				       write_addresses[i], value);
		}
	}
	else if (verbose)
		printf("skipping write\n");

	if (hub) {
		for (i = 0; i < NUM_HUB_READS; ++i) {
			rc = testread(fd, hub_addresses[i], &value);
			if (rc) {
				printf("FAILED - failed to read hub %08x\n",
				       hub_addresses[i]);
				goto exit;
			}

			if (verbose)
				printf("read %08x: %08x\n", hub_addresses[i],
				       value);
		}
	}

	printf("SUCCESS\n");

exit:
	close(fd);

	return rc;
}
