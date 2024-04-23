/*
 * Copyright (C) 2024 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pthread.h"
#include "stdio.h"
#include "aml_alsa_mixer.h"



#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include "stdbool.h"
#include "stdlib.h"
#include <string.h>
#include <semaphore.h>
#include "aml_alsa_mixer.h"
#include "aml_vad_wakeup.h"
#include "alsa_device_parser.h"

#define ANDROID_RB_PROPERTY "sys.powerctl"

#define PATH_CMD_LINE "/proc/cmdline"
#define PATH_REBOOT_REASON "/sys/devices/platform/reboot/reboot_reason"
#define PATH_FREEZE "/sys/power/state"

#define BUF_LEN_MAX    (4096)
#define BUF_LEN_NORMAL    (32)

void sem_post_vad_init(void);
void sem_wait_vad_init(void);
void sem_vad_init(void);

void writeSys(const char *path, const char *val) {
	int fd;

	if ((fd = open(path, O_RDWR)) < 0) {
		printf("writeSysFs, open fail:%s\n", path);
		return;
	}
	write(fd, val, strlen(val));
	close(fd);
}

int readSys(const char *path, char *buf, int count) {
	int fd, len = -1;

	if (NULL == buf) {
		printf("readSys, buf is null\n");
		return len;
	}

	if ((fd = open(path, O_RDONLY)) < 0) {
		printf("readSys, open fail:%s\n", path);
		return len;
	}

	len = read(fd, buf, count - 1);
	if (len < 0 || len > count - 1) {
		printf("readSys, read fail:%s\n", path);
		len = -1;
	} else {
		buf[len] = '\0';
	}
	close(fd);
	return len;
}

int processBuffer(char *t_buf, char split_ch, char *t_name, char *t_value) {
	char *tmp_ptr = NULL, *tmp_name = NULL, *tmp_value = NULL;

	tmp_ptr = t_buf;
	while (tmp_ptr && *tmp_ptr) {
		char *x = strchr(tmp_ptr, split_ch);
		if (x != NULL) {
			*x++ = '\0';
		}
		tmp_name = tmp_ptr;
		if (tmp_name[0] != '\0') {
			tmp_value = strchr(tmp_ptr, '=');
			if (tmp_value != NULL) {
				*tmp_value++ = '\0';
				if (strncmp(tmp_name, t_name, strlen(t_name)) == 0) {
					strncpy(t_value, tmp_value, BUF_LEN_NORMAL);
					return 0;
				}
			}
		}
		tmp_ptr = x;
	}
	return -1;
}

int processReadFile(char *file_path, int offset, char *pBuf, int len) {
	int tmp_cnt = 0;
	int dev_fd = open(file_path, O_RDONLY);
	if (dev_fd >= 0) {
		off_t ret = lseek(dev_fd, offset, SEEK_SET);
		if (ret == -1) {
			printf("processReadFile lseek fail\n");
			close(dev_fd);
			return 0;
		}
		tmp_cnt = read(dev_fd, pBuf, len);
		if (tmp_cnt < 0)
			tmp_cnt = 0;
		/* get rid of trailing newline, it happens */
		if (tmp_cnt > 0 && pBuf[tmp_cnt - 1] == '\n')
			tmp_cnt--;
		pBuf[tmp_cnt] = 0;
		close(dev_fd);
	} else {
		pBuf[0] = 0;
	}

	return tmp_cnt;
}

int processFile(char *t_fname, char split, char *t_name, char *t_value) {
	char line_buf[BUF_LEN_MAX];

	processReadFile(t_fname, 0, line_buf, BUF_LEN_MAX);
	return processBuffer(line_buf, split, t_name, t_value);
}

void vadReboot(void) {
	printf("ffv normal reboot\n");
	if (true) {
	}
}


bool isFFVFreezeMode() {
//	char buf[BUF_LEN_NORMAL] = { 0 };
//
//	printf("%s, line:%d\n", __func__, __LINE__);
//
//	memset(buf, 0, BUF_LEN_NORMAL);
//	if (processFile((char*)PATH_CMD_LINE, ' ', (char*)"ffv_freeze", buf) == 0) {
//		printf("%s, line:%d\n", __func__, __LINE__);
//		if (strncmp(buf, "on", 2) == 0){
//			printf("%s, line:%d\n", __func__, __LINE__);
//			return true;
//		}
//	}
//	return false;
	return true;
}

int32_t aml_vad_check_sound_card() {
	int card = -1;
	bool is_snd_dev_ok = false;
	int tmp_cnt = 0;
	while (1) {
		if (card == -1) {
			card = alsa_device_get_card_index();
		} else {
			if (is_snd_dev_ok == false) {
				char fn[256];
				snprintf(fn, sizeof(fn), "/dev/snd/controlC%u", card);
				if (access(fn, R_OK | W_OK) == 0) {
					is_snd_dev_ok = true;
					printf("%s, line:%d\n", __func__, __LINE__);
				} else {
					printf("aml_vad_check_sound_card dev:%s not found.\n", fn);
				}
			}
		}
		if (card != -1 && is_snd_dev_ok) {
			break;
		}
		printf("aml_vad_check_sound_card get card again ...\n");
		if (tmp_cnt++ >= 100) {
			printf("aml_vad_check_sound_card try get card timeout 10s ...\n");
			return -1;
		}
		usleep(100 * 1000);
	}
	printf("aml_vad_check_sound_card card id: %d\n", card);
	return 0;
}



int main(void)
{
	int32_t ret = 0;

	printf("VadService starting...\n");
	if (/*isCoolboot() && */isFFVFreezeMode()) {
		printf("VadService ffv freeze mode\n");
		ret = aml_vad_check_sound_card();
		if (ret != 0) {
			printf("VadService check sound card fail\n");
			return -1;
		}

		struct aml_mixer_handle alsa_mixer;
		memset(&alsa_mixer, 0, sizeof(struct aml_mixer_handle));
		open_mixer_handle(&alsa_mixer);
		sem_vad_init();
		aml_vad_suspend(&alsa_mixer);
		sem_wait_vad_init();
		printf("VadService enter the freeze mode.\n");
		writeSys(PATH_FREEZE, "freeze"); // block here

		printf("wake up !!!!!!!!!!!!!!\n");
		aml_vad_resume(&alsa_mixer);

		//	popen("reboot","r");
		//	vadReboot();
		return 0;
	}
}


