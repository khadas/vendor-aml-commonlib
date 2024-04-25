/*
 * Copyright (C) 2018 Amlogic Corporation.
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
#define LOG_TAG "AudioALSADeviceParser"

#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
//#include <cutils/log.h>
#include <string.h>

#include "alsa_device_parser.h"
#include "aml_malloc_debug.h"

#define READ_BUFFER_SIZE     (512)
#define NAME_LEN             (128)

/* patch */
#define ALSASOUND_CARD_PATH  "/proc/asound/cards"
#define ALSASOUND_PCM_PATH   "/proc/asound/pcm"

/* sound card name */
#define CARD_NAME_AUGE       "AMLAUGESOUND"
#define CARD_NAME_MESON      "AMLMESONAUDIO"
#define CARD_NAME_M8         "AMLM8AUDIO"
#define CARD_NAME_TV         "AMLTVAUDIO"
#define CARD_AML_KEYWORD     "AML"

/* for parse device port
 * for i2s, i2s_playback, i2s_capture,  when select device, firstly to check i2s_playback,
 * i2s_capture, then i2s
 */
#define ALSAPORT_PCM              "alsaPORT-pcm"         /* usually for bt pcm */
#define ALSAPORT_I2S              "alsaPORT-i2s"         /* i2s0, Playback,Capture */
#define ALSAPORT_I2SPLAYPLAYBACK  "alsaPORT-i2s1"        /* i2s1 */
#define ALSAPORT_I2SCAPTURE       "alsaPORT-i2s2"        /* i2s2 */
#define ALSAPORT_TDM              "alsaPORT-tdm"
#define ALSAPORT_PDM              "alsaPORT-pdm"
#define ALSAPORT_SPDIF            "alsaPORT-spdif"
#define ALSAPORT_SPDIFB           "alsaPORT-spdifb"
#define ALSAPORT_I2S2HDMI         "alsaPORT-i2s2hdmi"    /* virtual link */
#define ALSAPORT_TV               "alsaPORT-tv"          /* Now for TV input */
#define ALSAPORT_LPBK             "alsaPORT-loopback"
#define ALSAPORT_BUILTINMIC       "builtinmic"
#define ALSAPORT_EARC             "alsaPORT-earc"
#define ALSAPORT_I2S4HDMIRX       "i2s4hdmirx"
#define ALSAPORT_I2S4PARSER       "alsaPORT-i2s4parser"


#define ALOGI(fmt, ...)
#define ALOGD(fmt, ...)
#define ALOGE(fmt, ...)
#define ALOGW(fmt, ...)
#define ALOGV(fmt, ...)


struct AudioDeviceDescriptor {
	char name[NAME_LEN];

	int mCardindex;
	int mPcmIndex;

	int mPlayback;
	int mCapture;
};

struct alsa_info {
	int alsa_card_index;
	/* check whether auge sound card */
	int is_auge;
	/* whether auge pcm devices are checked */
	int deviced_checked;

	struct AudioDeviceDescriptor *pcm_descrpt;
	struct AudioDeviceDescriptor *i2s_descrpt;
	struct AudioDeviceDescriptor *i2s1_descrpt;
	struct AudioDeviceDescriptor *i2s2_descrpt;
	struct AudioDeviceDescriptor *tdm_descrpt;
	struct AudioDeviceDescriptor *pdm_descrpt;
	struct AudioDeviceDescriptor *spdif_descrpt;
	struct AudioDeviceDescriptor *spdifb_descrpt;
	struct AudioDeviceDescriptor *i2s2hdmi_descrpt;
	struct AudioDeviceDescriptor *tvin_descrpt;
	struct AudioDeviceDescriptor *lpbk_descrpt;
	struct AudioDeviceDescriptor *builtinmic_descrpt;
	struct AudioDeviceDescriptor *earc_descrpt;
	struct AudioDeviceDescriptor *i2s4hdmirx_descrpt;
	struct AudioDeviceDescriptor *i2s4parser_descrpt;
};

static struct alsa_info *p_aml_alsa_info;

static struct alsa_info *alsa_device_get_info()
{
	return p_aml_alsa_info;
}

bool alsa_device_is_auge(void)
{
	struct alsa_info *p_info = alsa_device_get_info();

	if (p_info && p_info->is_auge)
		return true;

	return false;
}
/** TODO: only for aloop test
 *    Suppose that aloop card id =0,
 *    if not, need way to parse.
 */
int alsa_device_get_card_index_by_name(void *name)
{
	FILE *mCardFile = NULL;
	int mCardIndex = -1;

	if (!name)
		return -1;

	mCardFile = fopen(ALSASOUND_CARD_PATH, "r");
	if (mCardFile) {
		char tempbuffer[READ_BUFFER_SIZE];

		while (!feof(mCardFile)) {
			fgets(tempbuffer, READ_BUFFER_SIZE, mCardFile);

			/* this line contain '[' character */
			if (strchr(tempbuffer, '[')) {
				char *Rch = strtok(tempbuffer, "[");
				int id = atoi(Rch);
				ALOGD("\tcurrent card id = %d, Rch = %s", id, Rch);
				Rch = strtok(NULL, " ]");
				ALOGD("\tcurrent sound card name = %s", Rch);
				if (strcmp(Rch, name) == 0) {
					ALOGD("\t sound cardIndex found = %d", id);
					mCardIndex = id;
					break;
				}
			}
			memset((void *)tempbuffer, 0, READ_BUFFER_SIZE);
		}
		fclose(mCardFile);
	}
	return mCardIndex;
}

/*
 * cat /proc/asound/cards
 *	 0 [AMLAUGESOUND   ]: AML-AUGESOUND - AML-AUGESOUND
                  AML-AUGESOUND
 * mCardIndex = 0;
 */
int alsa_device_get_card_index()
{
	struct alsa_info *p_info = alsa_device_get_info();
	FILE *mCardFile = NULL;
	bool isCardIndexFound = false;
	int mCardIndex = -1;

	if (p_info)
		return p_info->alsa_card_index;

	mCardFile = fopen(ALSASOUND_CARD_PATH, "r");
	if (mCardFile) {
		char tempbuffer[READ_BUFFER_SIZE];

		ALOGD("card open success");
		if (!p_aml_alsa_info) {
			p_aml_alsa_info = aml_audio_calloc(1, sizeof(struct alsa_info));
			if (!p_aml_alsa_info) {
				ALOGE ("NOMEM for alsa info\n");
				fclose(mCardFile);
				return -1;
			}
		}

		while (!feof(mCardFile)) {
			fgets(tempbuffer, READ_BUFFER_SIZE, mCardFile);

			/* this line contain '[' character */
			if (strchr(tempbuffer, '[') && strstr(tempbuffer, CARD_AML_KEYWORD)) {
				char *Rch = strtok(tempbuffer, "[");
				mCardIndex = atoi(Rch);
				ALOGD("\tcurrent mCardIndex = %d, Rch = %s", mCardIndex, Rch);
				Rch = strtok(NULL, " ]");
				ALOGD("\tcurrent sound card name = %s", Rch);
				if (strcmp(Rch, CARD_NAME_AUGE) == 0) {
					ALOGD("\t auge sound cardIndex found = %d", mCardIndex);
					isCardIndexFound = true;
					p_aml_alsa_info->is_auge = 1;
					break;
				} else if (strcmp(Rch, CARD_NAME_MESON) == 0) {
					ALOGD("\t meson sound cardIndex found = %d", mCardIndex);
					isCardIndexFound = true;
					p_aml_alsa_info->is_auge = 0;
					break;
				}
			}

			memset((void *)tempbuffer, 0, READ_BUFFER_SIZE);
		}
		ALOGD("reach EOF");
		fclose(mCardFile);
	} else
		ALOGE("Pcm open fail errno %d", errno);

	if (isCardIndexFound == true) {
		p_aml_alsa_info->alsa_card_index = mCardIndex;
	} else
		ALOGE("ParseCardIndex doesn't found card index");

	ALOGD("%s() parse card index:%d\n", __FUNCTION__, mCardIndex);

	return mCardIndex;
}

void alsa_device_parser_pcm_string(struct alsa_info *p_info, char *InputBuffer)
{
	char *Rch;
	char mStreamName[256];
	char *PortName = NULL;

	ALOGD("AddPcmString p_info:%p, InputBuffer = %s", p_info, InputBuffer);
	if (!p_info) {
		ALOGE("alsa device info is NULL\n");
		return;
	}

	Rch = strtok(InputBuffer, "-");
	/* parse for stream name */
	if (Rch  != NULL) {
		struct AudioDeviceDescriptor *mAudioDeviceDescriptor =
			aml_audio_calloc(1, sizeof(struct AudioDeviceDescriptor));
		if (!mAudioDeviceDescriptor) {
			ALOGE("%s no memory for device descriptor\n", __FUNCTION__);
			return;
		}

		mAudioDeviceDescriptor->mCardindex = atoi(Rch);
		Rch = strtok(NULL, ":");
		mAudioDeviceDescriptor->mPcmIndex = atoi(Rch);
		Rch = strtok(NULL, ":");
		if (Rch) {
			memcpy(mStreamName, Rch, 256);
			PortName = strstr(mStreamName, "alsaPORT-");
			if (PortName) {
				memcpy(mAudioDeviceDescriptor->name, PortName, sizeof(mAudioDeviceDescriptor->name));

				if (!strncmp(PortName, ALSAPORT_PCM, strlen(ALSAPORT_PCM)) &&
					p_info->pcm_descrpt == NULL) {
					p_info->pcm_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_I2S2HDMI, strlen(ALSAPORT_I2S2HDMI)) &&
					p_info->i2s2hdmi_descrpt == NULL) {
					p_info->i2s2hdmi_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_I2SPLAYPLAYBACK, strlen(ALSAPORT_I2SPLAYPLAYBACK)) &&
					p_info->i2s1_descrpt == NULL) {
					p_info->i2s1_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_I2SCAPTURE, strlen(ALSAPORT_I2SCAPTURE)) &&
					p_info->i2s2_descrpt == NULL) {
					p_info->i2s2_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_I2S4PARSER, strlen(ALSAPORT_I2S4PARSER)) &&
					p_info->i2s4parser_descrpt == NULL) {
					p_info->i2s4parser_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_I2S, strlen(ALSAPORT_I2S)) &&
					p_info->i2s_descrpt == NULL) {
					p_info->i2s_descrpt = mAudioDeviceDescriptor;
					if (strstr(PortName, ALSAPORT_I2S4HDMIRX))
						p_info->i2s4hdmirx_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_TDM, strlen(ALSAPORT_TDM)) &&
					p_info->tdm_descrpt == NULL) {
					p_info->tdm_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_PDM, strlen(ALSAPORT_PDM)) &&
					p_info->pdm_descrpt == NULL) {
					p_info->pdm_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_SPDIFB, strlen(ALSAPORT_SPDIFB)) &&
					p_info->spdifb_descrpt == NULL) {
					p_info->spdifb_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_SPDIF, strlen(ALSAPORT_SPDIF)) &&
					p_info->spdif_descrpt == NULL) {
					p_info->spdif_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_TV, strlen(ALSAPORT_TV)) &&
					p_info->tvin_descrpt == NULL) {
					p_info->tvin_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_LPBK, strlen(ALSAPORT_LPBK)) &&
					p_info->lpbk_descrpt == NULL) {
					p_info->lpbk_descrpt = mAudioDeviceDescriptor;
				} else if (!strncmp(PortName, ALSAPORT_EARC, strlen(ALSAPORT_EARC)) &&
					p_info->earc_descrpt == NULL) {
					p_info->earc_descrpt = mAudioDeviceDescriptor;
				} else {
					ALOGD("%s(), port:%s, not used for any desc", __func__, PortName);
					aml_audio_free(mAudioDeviceDescriptor);
					mAudioDeviceDescriptor = NULL;
					return;
				}

				if (strstr(PortName, ALSAPORT_BUILTINMIC) != NULL)
					p_info->builtinmic_descrpt = mAudioDeviceDescriptor;
			} else
				ALOGD("\tstream no alsaPORT prefix name, StreamName:%s\n", mStreamName);
		}
		ALOGD("%s Desc:%p mCardindex:%d, mPcmindex:%d, PortName:%s\n", __FUNCTION__, mAudioDeviceDescriptor,
		        mAudioDeviceDescriptor->mCardindex, mAudioDeviceDescriptor->mPcmIndex, PortName);
		strtok(NULL, ": ");
	}
}

static void dump_alsa_device_desc(struct alsa_info *p_info)
{
	ALOGD("%s %s: %p", __func__, ALSAPORT_PCM, p_info->pcm_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_I2S, p_info->i2s_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_I2SPLAYPLAYBACK, p_info->i2s1_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_I2SCAPTURE, p_info->i2s2_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_TDM, p_info->tdm_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_PDM, p_info->pdm_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_SPDIF, p_info->spdif_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_SPDIFB, p_info->spdifb_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_I2S2HDMI, p_info->i2s2hdmi_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_TV, p_info->tvin_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_LPBK, p_info->lpbk_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_BUILTINMIC, p_info->builtinmic_descrpt);
	ALOGD("%s %s: %p", __func__, ALSAPORT_EARC, p_info->earc_descrpt);
}

/*
 * alsaPORT is kept same with meson sound card
 * cat /proc/asound/pcm
 * 00-00: TDM-A-dummy-alsaPORT-pcm multicodec-0 :  : playback 1 : capture 1
 * 00-01: TDM-B-dummy-alsaPORT-i2s multicodec-1 :  : playback 1 : capture 1
 * 00-02: TDM-C-dummy multicodec-2 :  : playback 1 : capture 1
 * 00-03: PDM-dummy-alsaPORT-pdm dummy-3 :	: capture 1
 * 00-04: SPDIF-dummy-alsaPORT-spdif dummy-4 :	: playback 1 : capture 1
 * 00-05: SPDIF-B-dummy-alsaPORT-hdmi dummy-5 :  : playback 1
 */
int alsa_device_update_pcm_index(int alsaPORT, int stream)
{
	struct alsa_info *p_info = alsa_device_get_info();
	int new_port = -1;
	struct AudioDeviceDescriptor *pADD = NULL;

	if (!p_info || !p_info->is_auge) {
		/* fix to spdif port */
		if (alsaPORT >= 2)
			alsaPORT = 1;
		if (stream && !alsa_device_is_auge())
			alsaPORT = 0;

		return alsaPORT;
	}

	if (!p_info->deviced_checked) {
		FILE *mPcmFile = NULL;
		char tempbuffer[READ_BUFFER_SIZE];

		mPcmFile = fopen(ALSASOUND_PCM_PATH, "r");
		if (mPcmFile) {
			ALOGD("Pcm open success");
			while (!feof(mPcmFile)) {
				fgets(tempbuffer, READ_BUFFER_SIZE, mPcmFile);
				alsa_device_parser_pcm_string(p_info, tempbuffer);
				memset((void *)tempbuffer, 0, READ_BUFFER_SIZE);
			}
			ALOGD("reach EOF");
			fclose(mPcmFile);
			if (0)
				dump_alsa_device_desc(p_info);
			p_info->deviced_checked = 1;
		} else
			ALOGD("Pcm open fail");
	}
	switch (alsaPORT) {
	case PORT_I2S:
		if (stream == PLAYBACK) {
			/*if (p_info->i2s1_descrpt)
				pADD = p_info->i2s1_descrpt;
			else*/
				pADD = p_info->i2s_descrpt;
		} else if (stream == CAPTURE) {
			/*if (p_info->i2s2_descrpt)
				pADD = p_info->i2s2_descrpt;
			else*/
				pADD = p_info->i2s_descrpt;
		} else
			pADD = p_info->i2s_descrpt;
		break;
	case PORT_SPDIF:
		pADD = p_info->spdif_descrpt;
		break;
	case PORT_PCM:
		pADD = p_info->pcm_descrpt;
		break;
	case PROT_TDM:
		pADD = p_info->tdm_descrpt;
		break;
	case PROT_PDM:
		pADD = p_info->pdm_descrpt;
		break;
	case PORT_SPDIFB:
		pADD = p_info->spdifb_descrpt;
		break;
	case PORT_I2S2HDMI:
		pADD = p_info->i2s2hdmi_descrpt;
		break;
	case PORT_TV:
		pADD = p_info->tvin_descrpt;
		break;
	case PORT_I2S1:
		pADD = p_info->i2s1_descrpt;
		break;
	case PORT_I2S2:
		pADD = p_info->i2s2_descrpt;
		break;
	case PORT_LOOPBACK:
	case PORT_ECHO_REFERENCE:
		pADD = p_info->lpbk_descrpt;
		break;
	case PORT_BUILTINMIC:
		pADD = p_info->builtinmic_descrpt;
		if (!pADD) {
			pADD = p_info->i2s_descrpt;
		}
		break;
	case PORT_EARC:
		pADD = p_info->earc_descrpt;
		break;
	case PORT_I2S4HDMIRX:
		pADD = p_info->i2s4hdmirx_descrpt;
		break;
	case PORT_I2S4PARSER:
		pADD = p_info->i2s4parser_descrpt;
		break;
	default:
		pADD = p_info->i2s_descrpt;
		ALOGD("Default port is I2s\n");
		break;
	}

	if (pADD)
		new_port = pADD->mPcmIndex;

	ALOGD("auge sound card, pAdd=%p fix alsaPORT:%d to :%d\n",pADD, alsaPORT, new_port);
	/* dump_alsa_device_desc(p_info); */

	return new_port;
}

bool is_earc_descrpt(void)
{
	return p_aml_alsa_info->earc_descrpt ? 1 : 0;
}
