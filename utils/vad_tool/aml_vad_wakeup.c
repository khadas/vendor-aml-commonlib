/*
 * Copyright (C) 2021 Amlogic Corporation.
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

#define LOG_TAG "aml_vad_wakeup"
#include "stdio.h"
#include <pthread.h>
#include "asoundlib.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "alsa_device_parser.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "aml_vad_wakeup.h"
#include "aml_malloc_debug.h"
#include <semaphore.h>

#define VAD_DEVICE          (2)


typedef struct vad_wakeup_t {
    bool exit_run;
    pthread_t p_thread;
    bool is_suspend;
    struct pcm *pcm;
    struct aml_mixer_handle *mixer;
} VAD_WAKEUP_T;
static VAD_WAKEUP_T *g_pst_vad_wakeup = NULL;

#define ASR_IOC_TYPE                                    'C'
#define ASR_IOC_TYPE_GET_ASR_DATA_SIZE                  0
#define ASR_IOC_TYPE_RESET_ASR_DATA                     1

#define ASR_IOC_TYPE_CMD_GET_ASR_DATA_SIZE              \
    _IOR(ASR_IOC_TYPE, ASR_IOC_TYPE_GET_ASR_DATA_SIZE, unsigned int)
#define ASR_IOC_TYPE_CMD_RESET_ASR_DATA              \
    _IO(ASR_IOC_TYPE, ASR_IOC_TYPE_RESET_ASR_DATA)

static bool g_is_dumping_data = false;

static sem_t wait_vad_init;

void sem_post_vad_init(void)
{
	sem_post(&wait_vad_init);
}

void sem_wait_vad_init(void)
{
	sem_wait(&wait_vad_init);
}

void sem_vad_init(void)
{
	sem_init(&wait_vad_init, 0, 0);
}
int retval;

static void aml_vad_thread(VAD_WAKEUP_T* vad) {
	if (vad == NULL) {
		printf("vad is null");
		return;
	}
	struct pcm_config config;
	//    struct mixer *mixer;
	void *buffer;
	unsigned int size;
	int device = VAD_DEVICE;

	device = VAD_DEVICE;
	config.channels = 2;
	config.rate = 16000;
	memset(&config, 0, sizeof(config));
	config.channels = 2;
	config.rate = 16000;

	config.period_size = 128;
	config.period_count = 4;
	config.format = PCM_FORMAT_S32_LE;
	config.start_threshold = 0;
	config.stop_threshold = 0;
	config.silence_threshold = 0;

	if (vad->pcm == NULL) {
		vad->pcm = pcm_open(alsa_device_get_card_index(), device, PCM_IN, &config);
	}

	if (!vad->pcm || !pcm_is_ready(vad->pcm)) {
		printf("Unable to open PCM device (%s)", pcm_get_error(vad->pcm));
		return;
	}else{
		printf("\n\npcm open ok!!!!!!!!!\n\n");
	}

	size = pcm_frames_to_bytes(vad->pcm, pcm_get_buffer_size(vad->pcm));
	buffer = aml_audio_malloc(size);
	if (!buffer) {
		printf("Unable to allocate %u bytes", size);
		pcm_close(vad->pcm);
		return;
	}
	printf("Capturing ch:%d rate:%d bit:%d, read_size:%d", config.channels, config.rate,
			pcm_format_to_bits(config.format), size);

	sem_post_vad_init();

	while (true) {
		if (vad->exit_run) {
			break;
		}else{
			int ret = pcm_read(vad->pcm, buffer, size);
			if (ret != 0) {
				printf("pcm_read fail need:%d, ret:%d", size, ret);
			}
		}
	}
	pcm_close(vad->pcm);
	vad->pcm = NULL;
	printf("exit---");
	aml_audio_free(buffer);

	pthread_exit(&retval);

	return;
}

int32_t aml_vad_suspend(struct aml_mixer_handle *mixer) {
    int ret = 0;
    int source = 4;
    if (g_pst_vad_wakeup) {
        printf("already vadSuspend");
    } else {
        g_pst_vad_wakeup = (VAD_WAKEUP_T *)aml_audio_calloc(1, sizeof(VAD_WAKEUP_T));
        g_pst_vad_wakeup->exit_run = true;
    }

    if ((g_pst_vad_wakeup->p_thread > 0) || g_pst_vad_wakeup->exit_run == false) {
        printf("already vadThread running");
        return -1;
    }

    g_pst_vad_wakeup->exit_run = false;
    g_pst_vad_wakeup->mixer = mixer;
    aml_mixer_ctrl_set_int(mixer, AML_MIXER_ID_VAD_ENABLE, 1);
    source = 4; //PDMIN
    aml_mixer_ctrl_set_int(mixer, AML_MIXER_ID_VAD_SOURCE_SEL, source);

    pthread_create(&(g_pst_vad_wakeup->p_thread), NULL, aml_vad_thread, g_pst_vad_wakeup);

    if (ret) {
        printf("vad wake error creating thread: %s", strerror(ret));
        return false;
    }
    return true;
}

int32_t aml_vad_resume(struct aml_mixer_handle *mixer) {
    if (g_pst_vad_wakeup->exit_run) {
        printf("already vadThread running");
        return -1;
    }

    g_pst_vad_wakeup->exit_run = true;

    pthread_join(g_pst_vad_wakeup->p_thread, NULL);
    printf("\n pthread join exit 00\n");

    g_pst_vad_wakeup->p_thread = -1;
    aml_mixer_ctrl_set_int(mixer, AML_MIXER_ID_VAD_ENABLE, 0);
    aml_audio_free(g_pst_vad_wakeup);
    g_pst_vad_wakeup = NULL;
    return true;
}

void aml_vad_read_dump_data() {
    int vad_dump_dev_fd = -1;
    unsigned int dump_data_size = 0;
    unsigned int read_bytes = 0;
    FILE *file = NULL;

    vad_dump_dev_fd = open("/dev/asr_dump_data", O_RDWR);
    if (vad_dump_dev_fd < 0) {
        printf("open device file:/dev/asr_dump_data failed. err: %s", strerror(errno));
        goto exit;
    }
    printf("starting dump vad data, fd:%d", vad_dump_dev_fd);

    if (ioctl(vad_dump_dev_fd, ASR_IOC_TYPE_CMD_GET_ASR_DATA_SIZE, &dump_data_size) < 0) {
        printf("ioctl GET_ASR_DATA_SIZE failed. err:%s", strerror(errno));
        goto out;
    }
    printf("available vad data size:%d", dump_data_size);

    file = fopen("/data/audio/vad_dump_data", "w");
    if (file == NULL) {
        printf("open /data/audio/vad_dump_data fail. err:%s", strerror(errno));
        goto out;
    }

    while (read_bytes < dump_data_size) {
        char data[4096];
        printf("\n");
        ssize_t ret = read(vad_dump_dev_fd, data, sizeof(data));
        if (ret > 0) {
            fwrite(data, 1, ret, file);
            printf("read data ret:%zd, read_bytes:%d", ret, read_bytes);
            read_bytes += ret;
        } else if (ret == 0) {
            printf("read end, no data.");
            break;
        } else {
            printf("read fail ret:%zd err:%s", ret, strerror(errno));
            break;
        }
    }

    fclose(file);
out:
    if (ioctl(vad_dump_dev_fd, ASR_IOC_TYPE_CMD_RESET_ASR_DATA) < 0) {
        printf("ioctl RESET_ASR_DATA failed. err:%s", strerror(errno));
    }
    close(vad_dump_dev_fd);

exit:
    g_is_dumping_data = false;
    printf("dump vad data end");
}

void aml_vad_dump(bool block) {
}
