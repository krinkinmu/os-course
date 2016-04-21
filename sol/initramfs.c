#include "initramfs.h"
#include "paging.h"
#include "stdlib.h"
#include "string.h"
#include "error.h"
#include "misc.h"
#include "vfs.h"

#include <stdbool.h>
#include <stddef.h>


#define S_IFMT  0xF000
#define S_IFDIR 0x4000
#define S_IFREG 0x8000

#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)

struct cpio_header {
	char magic[6];
	char inode[8];
	char mode[8];
	char uid[8];
	char gid[8];
	char nlink[8];
	char mtime[8];
	char filesize[8];
	char major[8];
	char minor[8];
	char rmajor[8];
	char rminor[8];
	char namesize[8];
	char chksum[8];
} __attribute__((packed));


static unsigned long parse_hex(const char *data)
{
	static char buffer[9];

	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, data, sizeof(buffer) - 1);
	return strtoul(buffer, 0, 16);
}

static void create_dir(const char *name)
{
	const int rc = vfs_mkdir(name);

	if (rc)
		DBG_ERR("Failed to create %s with error %s",
					name, errstr(rc));
}

static int write_file(struct fs_file *file, const void *data, size_t size)
{
	const char *buf = data;
	size_t wr = 0;

	while (wr != size) {
		const int rc = vfs_write(file, buf + wr, size - wr);

		if (rc < 0)
			return rc;
		wr += rc;
	}

	return 0;
}

static void create_file(const char *name, const void *data, size_t size)
{
	struct fs_file file;
	int rc = vfs_create(name, &file);

	if (!rc) {
		rc = write_file(&file, data, size);
		if (rc)
			DBG_ERR("Failed to write file %s with error %s",
						name, errstr(rc));
		vfs_release(&file);
	} else {
		DBG_ERR("Failed to create file %s with error %s",
					name, errstr(rc));
	}
}

static void parse_cpio(const char *data, unsigned long size)
{
	unsigned long pos = 0;
	bool root = true;

	while (pos < size && size - pos >= sizeof(struct cpio_header)) {
		const struct cpio_header *hdr = (const void *)(data + pos);
		const unsigned long mode = parse_hex(hdr->mode);
		const unsigned long namesize = parse_hex(hdr->namesize);
		const unsigned long filesize = parse_hex(hdr->filesize);

		pos += sizeof(struct cpio_header);
		const char *filename = data + pos;
		pos = ALIGN(pos + namesize, 4);

		if (pos > size)
			break;

		if (strcmp(filename, "TRAILER!!!") == 0)
			break;

		const char *filedata = data + pos;
		pos += filesize;

		if (S_ISDIR(mode)) {
			if (!root)
				create_dir(filename);
			else
				root = false;
		} else if (S_ISREG(mode)) {
			if (pos <= size)
				create_file(filename, filedata, filesize);
		}
		pos = ALIGN(pos, 4);
	}
}

void setup_initramfs(void)
{
	const int rc = vfs_mount("ramfs", "initramfs", 0, 0);

	if (rc) {
		DBG_ERR("initramfs mount failed with error %s", errstr(rc));
		while (1);
	}

	parse_cpio(va((phys_t)initrd_begin), initrd_end - initrd_begin);
	memory_free_region(initrd_begin, initrd_end - initrd_begin);
}
