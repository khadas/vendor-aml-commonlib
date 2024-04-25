/*
 * Copyright (C) 2017 Amlogic Corporation.
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

/**
 ** aml_alsa_mixer.c
 **
 ** This program is APIs for read/write mixers of alsa.
 ** author: shen pengru
 **
 */
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
//#include <cutils/log.h>
#include <fcntl.h>
#include <pthread.h>
#include "asoundlib.h"
#include "aml_alsa_mixer.h"
#include "alsa_device_parser.h"

#undef  LOG_TAG
#define LOG_TAG "audio_alsa_mixer"

static struct aml_mixer_list gAmlMixerList[] = {
    /* for i2s out status */
    {AML_MIXER_ID_I2S_MUTE,             "Audio i2s mute"},
    /* for spdif out status */
    {AML_MIXER_ID_SPDIF_MUTE,           "Audio spdif mute"},
    {AML_MIXER_ID_SPDIF_B_MUTE,         "Audio spdif_b mute"},
    /* for HDMI TX status */
    {AML_MIXER_ID_HDMI_OUT_AUDIO_MUTE,  "Audio hdmi-out mute"},
    /* for HDMI ARC status */
    {AML_MIXER_ID_HDMI_ARC_AUDIO_ENABLE, "HDMI ARC Switch"},
    {AML_MIXER_ID_HDMI_EARC_AUDIO_ENABLE, "eARC_RX attended type"},
    {AML_MIXER_ID_AUDIO_IN_SRC,         "Audio In Source"},
    {AML_MIXER_ID_I2SIN_AUDIO_TYPE,     "I2SIN Audio Type"},
    {AML_MIXER_ID_SPDIFIN_AUDIO_TYPE,   "SPDIFIN Audio Type"},
    {AML_MIXER_ID_HW_RESAMPLE_ENABLE,   "Hardware resample enable"},
    {AML_MIXER_ID_OUTPUT_SWAP,          "Output Swap"},
    /* for HDMI RX status */
    {AML_MIXER_ID_HDMI_IN_AUDIO_STABLE, "HDMIIN audio stable"},
    {AML_MIXER_ID_HDMI_IN_SAMPLERATE,   "HDMIIN audio samplerate"},
    {AML_MIXER_ID_HDMI_IN_CHANNELS,     "HDMIIN audio channels"},
    {AML_MIXER_ID_HDMI_IN_FORMATS,      "HDMIIN audio format"},
    {AML_MIXER_ID_HDMI_ATMOS_EDID,      "HDMI ATMOS EDID Switch"},
    /* for ATV status */
    {AML_MIXER_ID_ATV_IN_AUDIO_STABLE,  "ATV audio stable"},
    {AML_MIXER_ID_SPDIF_FORMAT,         "Audio spdif format"},
    {AML_MIXER_ID_SPDIF_B_FORMAT,       "Audio spdif_b format"},
    {AML_MIXER_ID_AUDIO_SRC_TO_HDMI,    "HDMITX Audio Source Select"},

    /* for AV status */
    {AML_MIXER_ID_AV_IN_AUDIO_STABLE,   "AV audio stable"},
    /* for Speaker master volume */
    {AML_MIXER_ID_EQ_MASTER_VOLUME,     "EQ master volume"},
    /* ARCIN and SPDIFIN switch*/
    {AML_MIXER_ID_SPDIFIN_ARCIN_SWITCH, "AudioIn Switch"},
    {AML_MIXER_ID_SPDIFIN_PAO,          "SPDIFIN PAO"},
    /* HDMI IN audio format */
    {AML_MIXER_ID_HDMIIN_AUDIO_TYPE,    "HDMIIN Audio Type"},
    /* SPDIF IN audio SRC select */
    {AML_MIXER_ID_SPDIFIN_SRC,          "Audio spdifin source"},
    {AML_MIXER_ID_HDMIIN_AUDIO_PACKET,  "HDMIIN Audio Packet"},
    {AML_MIXER_ID_CHANGE_SPDIF_PLL,     "SPDIF CLK Fine Setting"},
    {AML_MIXER_ID_CHANGE_SPDIF_PLL_PPM,     "SPDIF CLK Fine PPM Tuning"},
    {AML_MIXER_ID_CHANGE_SPDIFB_PLL,    "SPDIF_B CLK Fine Setting"},
    {AML_MIXER_ID_CHANGE_SPDIFB_PLL_PPM,    "SPDIF_B CLK Fine PPM Tuning"},
    {AML_MIXER_ID_CHANGE_I2S_PLL,       "TDM MCLK Fine Setting"},
    {AML_MIXER_ID_CHANGE_I2S_PLL_PPM,       "TDM MCLK Fine PPM Tuning"},
    {AML_MIXER_ID_CHANGE_EARC_PLL,      "eARC_TX CLK Fine Setting"},
    {AML_MIXER_ID_CHANGE_EARC_PLL_PPM,      "eARC_TX CLK Fine PPM Tuning"},
    {AML_MIXER_ID_SPDIF_IN_SAMPLERATE,  "SPDIFIN audio samplerate"},
    {AML_MIXER_ID_HW_RESAMPLE_SOURCE,   "Hw resample module"},
    {AML_MIXER_ID_EARCRX_AUDIO_CODING_TYPE,   "eARC_RX Audio Coding Type"},
    {AML_MIXER_ID_AUDIO_HAL_FORMAT,     "Audio HAL Format"},
    {AML_MIXER_ID_HDMIIN_AUDIO_EDID,    "HDMIIN AUDIO EDID"},
    {AML_MIXER_ID_EARC_TX_ATTENDED_TYPE, "eARC_TX attended type"},
    {AML_MIXER_ID_EARC_TX_AUDIO_TYPE,   "eARC_TX Audio Coding Type"},
    {AML_MIXER_ID_EARC_TX_EARC_MODE,    "eARC_TX eARC Mode"},
    {AML_MIXER_ID_ARC_EARC_RX_ENABLE,   "ARC eARC RX enable"},
    {AML_MIXER_ID_ARC_EARC_TX_ENABLE,   "ARC eARC TX enable"},
    {AML_MIXER_ID_EARCTX_CDS,           "eARC_TX CDS"},
    {AML_MIXER_ID_EARC_TX_LATENCY,      "eARC_TX Latency"},
    {AML_MIXER_ID_ARC_EARC_SPDIFOUT_REG_MUTE,      "ARC eARC Spdifout Reg Mute"},
    {AML_MIXER_ID_EARC_TX_CA,           "eARC_TX Channel Allocation"},
    {AML_MIXER_ID_AML_CHIP_ID,          "AML chip id"},
    {AML_MIXER_ID_TVIN_VIDEO_DELAY,     "TVIN VIDEO DELAY"},
    {AML_MIXER_ID_TVIN_VIDEO_MIN_DELAY, "TVIN VIDEO MIN DELAY"},
    {AML_MIXER_ID_TVIN_VIDEO_MAX_DELAY, "TVIN VIDEO MAX DELAY"},
    {AML_MIXER_ID_SPDIF_B_OUT_CHANNEL_STATUS, "spdif_b out channel status"},
    {AML_MIXER_ID_MEDIA_VIDEO_DELAY,    "Media Video Delay"},
    {AML_MIXER_ID_HDMIIN_AUDIO_MODE,    "HDMIIN Audio output mode"},
    {AML_MIXER_ID_VAD_ENABLE,           "VAD enable"},
    {AML_MIXER_ID_VAD_SOURCE_SEL,       "VAD Source sel"},
    {AML_MIXER_ID_VAD_SWITCH,           "VAD Switch"},
    {AML_MIXER_ID_DTV_CLK_TUNING,       "DTV clk force MPLL"},
    {AML_MIXER_ID_I2S2HDMI_FORMAT,      "Audio I2S to HDMITX Format"},
    {AML_MIXER_ID_SPDIF_OUT_CHANNEL_STATUS, "spdif out channel status"},
    {AML_MIXER_ID_HDMIIN_NONAUDIO,      "HDMIIN NONAUDIO"},
    {AML_MIXER_ID_AED_MASTER_VOLUME, "AED master volume"},
    {AML_MIXER_ID_AED_LCH_VOLUME, "AED Lch volume"},
    {AML_MIXER_ID_AED_RCH_VOLUME, "AED Rch volume"},
    {AML_MIXER_ID_DAC_PLAYBACK_VOLUME,  "DAC Digital Playback Volume"},
    {AML_MIXER_ID_AI_SOUND_MODE,         "AI Sort Result"},
    {AML_MIXER_ID_AUDIO_PCPD_MONITOR_ENABLE,  "Pc_Pd_Monitor_A Detect enable"},
    {AML_MIXER_ID_AUDIO_PCPD_MONITOR_DATA_TYPE,  "Pc_Pd_Monitor_A Audio Type"},
};

static char *get_mixer_name_by_id(int mixer_id)
{
    int i;
    int cnt_mixer = sizeof(gAmlMixerList) / sizeof(struct aml_mixer_list);

    for (i = 0; i < cnt_mixer; i++) {
        if (gAmlMixerList[i].id == mixer_id) {
            return gAmlMixerList[i].mixer_name;
        }
    }

    return NULL;
}

int open_mixer_handle(struct aml_mixer_handle *mixer_handle)
{
    int card = 0;
    struct mixer *pmixer = NULL;

    card = alsa_device_get_card_index();
    if (card < 0) {
        printf("[%s:%d] Failed to get sound card\n", __FUNCTION__, __LINE__);
        return -1;
    }

    pmixer = mixer_open(card);
    if (NULL == pmixer) {
        printf("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
        return -1;
    }
    mixer_handle->pMixer = pmixer;
    pthread_mutex_init(&mixer_handle->lock, NULL);

    return 0;
}

int close_mixer_handle(struct aml_mixer_handle *mixer_handle)
{
    struct mixer *pMixer = mixer_handle->pMixer;

    if (NULL != pMixer) {
        mixer_close(pMixer);
    }

    return 0;
}

static struct mixer_ctl *get_mixer_ctl_handle(struct mixer *pmixer, int mixer_id)
{
    struct mixer_ctl *pCtrl = NULL;

    if (get_mixer_name_by_id(mixer_id) != NULL) {
        pCtrl = mixer_get_ctl_by_name(pmixer,
                                      get_mixer_name_by_id(mixer_id));
    }

    return pCtrl;
}

int aml_mixer_ctrl_get_array(struct aml_mixer_handle *mixer_handle, int mixer_id, void *array, int count)
{
    struct mixer *pMixer = mixer_handle->pMixer;
    struct mixer_ctl *pCtrl;

    if (pMixer == NULL) {
        printf("[%s:%d] pMixer is invalid!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    pthread_mutex_lock(&mixer_handle->lock);
    pCtrl = get_mixer_ctl_handle(pMixer, mixer_id);
    if (pCtrl == NULL) {
        printf("[%s:%d] Failed to open mixer %s\n", __FUNCTION__, __LINE__,
              get_mixer_name_by_id(mixer_id));
        pthread_mutex_unlock(&mixer_handle->lock);
        return -1;
    }
    mixer_ctl_get_array(pCtrl, array, count);
    pthread_mutex_unlock(&mixer_handle->lock);

    return 0;
}


int aml_mixer_ctrl_get_int(struct aml_mixer_handle *mixer_handle, int mixer_id)
{
    struct mixer *pMixer = mixer_handle->pMixer;
    struct mixer_ctl *pCtrl;
    int value = -1;

    if (pMixer == NULL) {
        printf("[%s:%d] pMixer is invalid!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    pthread_mutex_lock(&mixer_handle->lock);
    pCtrl = get_mixer_ctl_handle(pMixer, mixer_id);
    if (pCtrl == NULL) {
        printf("[%s:%d] Failed to open mixer %s\n", __FUNCTION__, __LINE__,
              get_mixer_name_by_id(mixer_id));
        pthread_mutex_unlock(&mixer_handle->lock);
        return -1;
    }

    value = mixer_ctl_get_value(pCtrl, 0);
    pthread_mutex_unlock(&mixer_handle->lock);

    return value;
}

int aml_mixer_ctrl_get_enum_str_to_int(struct aml_mixer_handle *mixer_handle, int mixer_id, int *ret)
{
    struct mixer *pMixer = mixer_handle->pMixer;
    struct mixer_ctl *pCtrl;
    const char *string = NULL;
    int value = -1;

    if (pMixer == NULL) {
        printf("[%s:%d] pMixer is invalid!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    pthread_mutex_lock(&mixer_handle->lock);
    pCtrl = get_mixer_ctl_handle(pMixer, mixer_id);
    if (pCtrl == NULL) {
        printf("[%s:%d] Failed to open mixer %s\n", __FUNCTION__, __LINE__,
              get_mixer_name_by_id(mixer_id));
        pthread_mutex_unlock(&mixer_handle->lock);
        return -1;
    }
    value = mixer_ctl_get_value(pCtrl, 0);
    string = mixer_ctl_get_enum_string(pCtrl, value);
    pthread_mutex_unlock(&mixer_handle->lock);

    if (string) {
        *ret = atoi(string);
        return 0;
    } else {
        return -1;
    }
}

int aml_mixer_ctrl_set_array(struct aml_mixer_handle *mixer_handle, int mixer_id, void *array, int count)
{
    struct mixer *pMixer = mixer_handle->pMixer;
    struct mixer_ctl *pCtrl;

    if (pMixer == NULL) {
        printf("[%s:%d] pMixer is invalid!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    pthread_mutex_lock(&mixer_handle->lock);
    pCtrl = get_mixer_ctl_handle(pMixer, mixer_id);
    if (pCtrl == NULL) {
        printf("[%s:%d] Failed to open mixer %s\n", __FUNCTION__, __LINE__,
              get_mixer_name_by_id(mixer_id));
        pthread_mutex_unlock(&mixer_handle->lock);
        return -1;
    }
    int ret = mixer_ctl_set_array(pCtrl, array, count);
    if (ret < 0)
        printf("[%s:%d] failed to set array, error: %d\n",__FUNCTION__, __LINE__, ret);
    pthread_mutex_unlock(&mixer_handle->lock);

    return 0;
}


int aml_mixer_ctrl_set_int(struct aml_mixer_handle *mixer_handle, int mixer_id, int value)
{
    struct mixer *pMixer = mixer_handle->pMixer;
    struct mixer_ctl *pCtrl;

    if (pMixer == NULL) {
        printf("[%s:%d] pMixer is invalid!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    pthread_mutex_lock(&mixer_handle->lock);
    pCtrl = get_mixer_ctl_handle(pMixer, mixer_id);
    if (pCtrl == NULL) {
        printf("[%s:%d] Failed to open mixer %s\n", __FUNCTION__, __LINE__,
              get_mixer_name_by_id(mixer_id));
        pthread_mutex_unlock(&mixer_handle->lock);
        return -1;
    }
    mixer_ctl_set_value(pCtrl, 0, value);
    pthread_mutex_unlock(&mixer_handle->lock);

    return 0;
}

int aml_mixer_ctrl_set_str(struct aml_mixer_handle *mixer_handle, int mixer_id, char *value)
{
    struct mixer *pMixer = mixer_handle->pMixer;
    struct mixer_ctl *pCtrl;

    if (pMixer == NULL) {
        printf("[%s:%d] pMixer is invalid!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    pthread_mutex_lock(&mixer_handle->lock);
    pCtrl = get_mixer_ctl_handle(pMixer, mixer_id);
    if (pCtrl == NULL) {
        printf("[%s:%d] Failed to open mixer %s\n", __FUNCTION__, __LINE__,
              get_mixer_name_by_id(mixer_id));
        pthread_mutex_unlock(&mixer_handle->lock);
        return -1;
    }
    mixer_ctl_set_enum_by_string(pCtrl, value);
    pthread_mutex_unlock(&mixer_handle->lock);

    return 0;
}


