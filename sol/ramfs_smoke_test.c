#include "string.h"
#include "stdio.h"
#include "error.h"
#include "vfs.h"

#define RAMFS_ROOT       "ramfs_smoke_test_root"
#define RAMFS_ROOT_PATH  "/" RAMFS_ROOT
#define RAMFS_FILE       "file"
#define RAMFS_FILE_PATH  RAMFS_ROOT_PATH "/" RAMFS_FILE
#define RAMFS_DIRN       10


static void test_readdir(struct fs_file *dir)
{
	struct dirent entries[RAMFS_DIRN];
	int rc = vfs_readdir(dir, entries, RAMFS_DIRN);

	if (rc >= 0) {
		DBG_INFO("vfs_readdir returned %d entries", rc);
		for (int i = 0; i != rc; ++i)
			DBG_INFO("entry: %s", entries[i].name);
	} else {
		DBG_ERR("vfs_readdir failed with error: %s", errstr(rc));
	}
}

static void test_create(void)
{
	struct fs_file file;
	const char *file_path = RAMFS_FILE_PATH;
	int rc = vfs_create(file_path, &file);

	if (!rc) {
		DBG_INFO("vfs_create(%s) succeeded", file_path);
		rc = vfs_release(&file);
		if (!rc)
			DBG_INFO("vfs_release succeeded");
		else
			DBG_ERR("vfs_release failed with error: %s",
				errstr(rc));
	} else {
		DBG_ERR("vfs_create(%s) failed with error: %s",
			file_path, errstr(rc));
	}
}

static void test_open(void)
{
	struct fs_file file;
	const char *file_path = RAMFS_FILE_PATH;
	int rc = vfs_open(file_path, &file);

	if (!rc) {
		DBG_INFO("vfs_open(%s) succeeded", file_path);
		rc = vfs_release(&file);
		if (!rc)
			DBG_INFO("vfs_release succeeded");
		else
			DBG_ERR("vfs_release failed with error: %s",
				errstr(rc));
	} else {
		DBG_ERR("vfs_open(%s) failed with error: %s",
			file_path, errstr(rc));
	}
}

static void test_unlink(void)
{
	const char *file_path = RAMFS_FILE_PATH;
	int rc = vfs_unlink(file_path);

	if (!rc)
		DBG_INFO("vfs_unlink(%s) succeeded", file_path);
	else
		DBG_ERR("vfs_unlink(%s) failed with error: %s",
			file_path, errstr(rc));
}

static void test_root(void)
{
	struct fs_file root_dir;
	const char *root_path = RAMFS_ROOT_PATH;
	int rc = vfs_open(root_path, &root_dir);

	if (!rc) {
		DBG_INFO("vfs_open(%s) succeeded", root_path);
		test_readdir(&root_dir);
		rc = vfs_release(&root_dir);
		if (rc)
			DBG_ERR("vfs_release failed with error: %s",
				errstr(rc));
		else
			DBG_INFO("vfs_release succeeded");
	} else {
		DBG_ERR("vfs_open(%s) failed with error: %s",
			root_path, errstr(rc));
	}
}

static void test_read_write(void)
{
	const char *file_path = RAMFS_FILE_PATH;
	const char *test_string = "ramfs test string";
	struct fs_file file;
	int rc = vfs_open(file_path, &file);
	char buffer[256];
	
	if (!rc) {
		DBG_INFO("vfs_open(%s) succeeded", file_path);

		rc = vfs_write(&file, test_string, strlen(test_string) + 1);
		if (rc < 0)
			DBG_ERR("vfs_write failed with error: %s",
				errstr(rc));
		else
			DBG_INFO("vfs_write wrote %d bytes", rc);

		rc = vfs_seek(&file, 0, FSS_END);
		if (rc < 0) {
			DBG_ERR("vfs_seek failed with error: %s",
				errstr(rc));
		} else {
			DBG_INFO("vfs_seek reports file size %d", rc);
			if ((size_t)rc != strlen(test_string) + 1)
				DBG_ERR("vfs_seek returned wrong file size");
		}

		rc = vfs_seek(&file, 0, FSS_SET);
		if (rc < 0)
			DBG_ERR("vfs_seek failed with error: %s",
				errstr(rc));
		else
			DBG_INFO("vfs_seek set offset to %d", rc);
	
		memset(buffer, 0, sizeof(buffer));
		rc = vfs_read(&file, buffer, strlen(test_string) + 1);
		if (rc < 0)
			DBG_ERR("vfs_read failed with error: %s",
				errstr(rc));
		else
			DBG_INFO("vfs_read read %d bytes", rc);

		if (strcmp(test_string, buffer)) {
			DBG_ERR("read data doesn't match written data");
			DBG_ERR("written: %s", test_string);
			DBG_ERR("read: %s", buffer);
		} else {
			DBG_INFO("read data matches written data");
		}

		rc = vfs_release(&file);
		if (!rc)
			DBG_INFO("vfs_release succeeded");
		else
			DBG_ERR("vfs_release failed with error: %s",
				errstr(rc));
	} else {
		DBG_ERR("vfs_open(%s) failed with error: %s",
			file_path, errstr(rc));
	}
}

static void test_read_beyond_the_end(void)
{
	const char *file_path = RAMFS_FILE_PATH;
	struct fs_file file;
	int rc = vfs_open(file_path, &file);
	char buffer[256];
	
	if (!rc) {
		DBG_INFO("vfs_open(%s) succeeded", file_path);

		rc = vfs_seek(&file, 0, FSS_END);
		if (rc < 0)
			DBG_ERR("vfs_seek failed with error: %s",
				errstr(rc));
		else
			DBG_INFO("vfs_seek reports file size %d", rc);

		rc = vfs_read(&file, buffer, sizeof(buffer));
		if (rc < 0)
			DBG_ERR("vfs_read failed with error: %s",
				errstr(rc));
		else if (rc)
			DBG_ERR("vfs_read read %d bytes beyond the end", rc);
		else
			DBG_INFO("vfs_read read %d bytes beyond the end", rc);

		rc = vfs_release(&file);
		if (!rc)
			DBG_INFO("vfs_release succeeded");
		else
			DBG_ERR("vfs_release failed with error: %s",
				errstr(rc));
	} else {
		DBG_ERR("vfs_open(%s) failed with error: %s",
			file_path, errstr(rc));
	}
}

static void ramfs_do_test(void)
{
	test_create();
	test_open();
	test_root();
	test_read_write();
	test_read_beyond_the_end();
	test_unlink();
	test_root();
}

void ramfs_test(void)
{
	int rc = vfs_mount("ramfs", RAMFS_ROOT, 0, 0);

	if (!rc) {
		DBG_INFO("successfully mounted");
		ramfs_do_test();
		rc = vfs_umount(RAMFS_ROOT);
		if (rc)
			DBG_ERR("vfs_umount failed with %d", rc);
		else
			DBG_INFO("successfully unmounted");
	} else {
		DBG_ERR("vfs_mount failed with %d", rc);
	}
}
