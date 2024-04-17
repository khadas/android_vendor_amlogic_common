/*
 * Copyright (c) 2024 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *
 */
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <libgen.h>

static const char *system_dlkm_modules_dir_const = "/system/lib/modules";
static char system_dlkm_modules_dir[200];
static int system_dlkm_modules_path_len;
static const char *modules_load_list = "/vendor/lib/modules/modules_load_list";

unsigned char dbg_flag = 0;
#define prdbg(fmt, ...)				\
{						\
	if (dbg_flag)				\
		printf(fmt, ##__VA_ARGS__);	\
}

void module_install(const char *filename)
{
	int fd;
	int ret;

	prdbg("module install: %s\n", filename);
	fd = open(filename, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	if (fd == -1) {
		 prdbg("Could not open module %s: %s\n", filename, strerror(errno));
		 return;
	}
	ret = syscall(__NR_finit_module, fd, " ", 0);
	if (ret != 0) {
		 prdbg("Could not insmod module %s: %s\n", filename, strerror(errno));
	}

	close(fd);
}

int get_system_dlkm_modules_dir(void)
{
	DIR* dir;
	struct dirent* dnt;

	dir = opendir(system_dlkm_modules_dir_const);
	if (dir == NULL) {
		prdbg("opendir %s: %s\n", system_dlkm_modules_dir_const, strerror(errno));
		return -1;
	}

	while ((dnt = readdir(dir)) != NULL)
	{
		if (isdigit(dnt->d_name[0]) && strstr(dnt->d_name, "-android")) {
			system_dlkm_modules_path_len = snprintf(system_dlkm_modules_dir, 100, "%s/%s/", system_dlkm_modules_dir_const, dnt->d_name);
			break;
		}
	}

	closedir(dir);

	return 0;
}

int main(int argc, char *argv[])
{
	FILE *modules_load_file;
	char module[200];
	int len;

	if ((argc == 2) && (argv[1][0] == '-') && (argv[1][1] == 'd'))
		dbg_flag = 1;

	get_system_dlkm_modules_dir();

	modules_load_file = fopen(modules_load_list, "r");
	if (modules_load_file) {
		while (fgets(module, sizeof(module), modules_load_file)) {
			len = strlen(module);
			if (module[len-1] == '\n')
				module[len-1] = '\0';
			prdbg("module: %s\n", module);
			if (module[0] == '/') {
				module_install(module);
			} else {
				strcpy(&system_dlkm_modules_dir[system_dlkm_modules_path_len], module);
				module_install(system_dlkm_modules_dir);
			}
		}
		fclose(modules_load_file);
	}

	return 0;
}
