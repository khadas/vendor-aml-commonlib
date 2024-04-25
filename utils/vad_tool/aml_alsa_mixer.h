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

#ifndef _AML_ALSA_MIXER_H_
#define _AML_ALSA_MIXER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Value of the Spdif Protection Mode
 **/
typedef enum SPDIF_PROTECTION__MODE {
    SPDIF_PROTECTION__MODE_NONE  = 0,
    SPDIF_PROTECTION__MODE_NEVER = 1,
    SPDIF_PROTECTION__MODE_ONCE  = 2,
} eSpdifProtectionMode;

#define SPDIF_PROTECTION_ENABLE 65537
#define SPDIF_PROTECTION_DISABLE 1

/*
 *  Value of the Alsa Mixer Control Point
 **/
/* Audio i2s mute */
typedef enum MIXER_AUDIO_I2S_MUTE {
    I2S_MUTE_OFF = 0,
    I2S_MUTE_ON  = 1,
    I2S_MUTE_MAX,
} eMixerAudioI2sMute;

/* Audio spdif mute */
typedef enum MIXER_SPDIF_MUTE {
    SPDIF_MUTE_OFF = 0,
    SPDIF_MUTE_ON  = 1,
    SPDIF_MUTE_MAX,
} eMixerSpdifMute;

/* Audio In Source */
typedef enum MIXER_AUDIO_IN_SOURCE {
    AUDIOIN_SRC_LINEIN  = 0,
    AUDIOIN_SRC_ATV     = 1,
    AUDIOIN_SRC_HDMI    = 2,
    AUDIOIN_SRC_SPDIFIN = 3,
    AUDIOIN_SRC_MAX,
} eMixerAudioInSrc;

/* Audio I2SIN Audio Type */
typedef enum MIXER_I2SIN_AUDIO_TYPE {
    I2SIN_AUDIO_TYPE_LPCM      = 0,
    I2SIN_AUDIO_TYPE_NONE_LPCM = 1,
    I2SIN_AUDIO_TYPE_UN_KNOWN  = 2,
    I2SIN_AUDIO_TYPE_MAX,
} eMixerI2sInAudioType;

/* Audio SPDIFIN Audio Type */
typedef enum MIXER_SPDIFIN_AUDIO_TYPE {
    SPDIFIN_AUDIO_TYPE_LPCM   = 0,
    SPDIFIN_AUDIO_TYPE_AC3    = 1,
    SPDIFIN_AUDIO_TYPE_EAC3   = 2,
    SPDIFIN_AUDIO_TYPE_DTS    = 3,
    SPDIFIN_AUDIO_TYPE_DTSHD  = 4,
    SPDIFIN_AUDIO_TYPE_TRUEHD = 5,
    SPDIFIN_AUDIO_TYPE_PAUSE  = 6,
    SPDIFIN_AUDIO_TYPE_MAX,
} eMixerApdifinAudioType;

/* Hardware resample enable */
typedef enum MIXER_HW_RESAMPLE_ENABLE {
    HW_RESAMPLE_DISABLE = 0,
    HW_RESAMPLE_32K,
    HW_RESAMPLE_44K,
    HW_RESAMPLE_48K,
    HW_RESAMPLE_88K,
    HW_RESAMPLE_96K,
    HW_RESAMPLE_176K,
    HW_RESAMPLE_192K,
    HW_RESAMPLE_MAX,
} eMixerHwResample;

/* Output Swap */
typedef enum MIXER_OUTPUT_SWAP {
    OUTPUT_SWAP_LR = 0,
    OUTPUT_SWAP_LL = 1,
    OUTPUT_SWAP_RR = 2,
    OUTPUT_SWAP_RL = 3,
    OUTPUT_SWAP_MAX,
} eMixerOutputSwap;

/* spdifin/arcin */
typedef enum AUDIOIN_SWITCH {
    SPDIF_IN = 0,
    ARC_IN = 1,
} eMixerAudioinSwitch;

struct aml_mixer_ctrl {
    int  ctrl_idx;
    char ctrl_name[50];
};

/* the same as toddr source*/
typedef enum ResampleSource {
    RESAMPLE_FROM_TDMIN_A     = 0,
    RESAMPLE_FROM_TDMIN_B     = 1,
    RESAMPLE_FROM_TDMIN_C     = 2,
    RESAMPLE_FROM_SPDIFIN     = 3,
    RESAMPLE_FROM_PDMIN       = 4,
    RESAMPLE_FROM_FRATV       = 5,
    RESAMPLE_FROM_TDBIN_LB    = 6,
    RESAMPLE_FROM_LOOPBACK_A  = 7,
    RESAMPLE_FROM_FRHDMIRX    = 8,
    RESAMPLE_FROM_LOOPBACK_B  = 9,
    RESAMPLE_FROM_SPDIFIN_LB  = 10,
    RESAMPLE_FROM_EARCRX_DMAC = 11,
    RESAMPLE_FROM_RESERVED_0  = 12,
    RESAMPLE_FROM_RESERVED_1  = 13,
    RESAMPLE_FROM_RESERVED_2  = 14,
    RESAMPLE_FROM_VAD         = 15,
} eMixerAudioResampleSource;

/*
 *  Alsa Mixer Control Command List
 **/
typedef enum AML_MIXER_CTRL_ID {
    AML_MIXER_ID_I2S_MUTE = 0,
    AML_MIXER_ID_SPDIF_MUTE,
    AML_MIXER_ID_SPDIF_B_MUTE,
    AML_MIXER_ID_HDMI_OUT_AUDIO_MUTE,
    AML_MIXER_ID_HDMI_ARC_AUDIO_ENABLE,
    AML_MIXER_ID_HDMI_EARC_AUDIO_ENABLE,
    AML_MIXER_ID_AUDIO_IN_SRC,
    AML_MIXER_ID_I2SIN_AUDIO_TYPE,
    AML_MIXER_ID_SPDIFIN_AUDIO_TYPE,
    AML_MIXER_ID_HW_RESAMPLE_ENABLE,
    AML_MIXER_ID_OUTPUT_SWAP,
    AML_MIXER_ID_HDMI_IN_AUDIO_STABLE,
    AML_MIXER_ID_HDMI_IN_SAMPLERATE,
    AML_MIXER_ID_HDMI_IN_CHANNELS,
    AML_MIXER_ID_HDMI_IN_FORMATS,
    AML_MIXER_ID_HDMI_ATMOS_EDID,
    AML_MIXER_ID_ATV_IN_AUDIO_STABLE,
    AML_MIXER_ID_SPDIF_FORMAT,
    AML_MIXER_ID_SPDIF_B_FORMAT,
    AML_MIXER_ID_AUDIO_SRC_TO_HDMI,
    AML_MIXER_ID_AV_IN_AUDIO_STABLE,
    AML_MIXER_ID_EQ_MASTER_VOLUME,
    AML_MIXER_ID_SPDIFIN_ARCIN_SWITCH,
    AML_MIXER_ID_SPDIFIN_PAO,
    AML_MIXER_ID_HDMIIN_AUDIO_TYPE,
    AML_MIXER_ID_SPDIFIN_SRC,
    AML_MIXER_ID_HDMIIN_AUDIO_PACKET,
    AML_MIXER_ID_CHANGE_SPDIF_PLL,
    AML_MIXER_ID_CHANGE_SPDIF_PLL_PPM,
    AML_MIXER_ID_CHANGE_SPDIFB_PLL,
    AML_MIXER_ID_CHANGE_SPDIFB_PLL_PPM,
    AML_MIXER_ID_CHANGE_I2S_PLL,
    AML_MIXER_ID_CHANGE_I2S_PLL_PPM,
    AML_MIXER_ID_CHANGE_EARC_PLL,
    AML_MIXER_ID_CHANGE_EARC_PLL_PPM,
    AML_MIXER_ID_SPDIF_IN_SAMPLERATE,
    AML_MIXER_ID_HW_RESAMPLE_SOURCE,
    AML_MIXER_ID_EARCRX_AUDIO_CODING_TYPE,
    AML_MIXER_ID_AUDIO_HAL_FORMAT,
    AML_MIXER_ID_HDMIIN_AUDIO_EDID,
    AML_MIXER_ID_EARC_TX_ATTENDED_TYPE,
    AML_MIXER_ID_EARC_TX_AUDIO_TYPE,
    AML_MIXER_ID_EARC_TX_EARC_MODE,
    AML_MIXER_ID_ARC_EARC_RX_ENABLE,
    AML_MIXER_ID_ARC_EARC_TX_ENABLE,
    AML_MIXER_ID_EARCTX_CDS,
    AML_MIXER_ID_EARC_TX_LATENCY,
    AML_MIXER_ID_ARC_EARC_SPDIFOUT_REG_MUTE,
    AML_MIXER_ID_EARC_TX_CA,
    AML_MIXER_ID_AML_CHIP_ID,
    AML_MIXER_ID_TVIN_VIDEO_DELAY,
    AML_MIXER_ID_TVIN_VIDEO_MIN_DELAY,
    AML_MIXER_ID_TVIN_VIDEO_MAX_DELAY,
    AML_MIXER_ID_SPDIF_B_OUT_CHANNEL_STATUS,
    AML_MIXER_ID_MEDIA_VIDEO_DELAY,
    AML_MIXER_ID_HDMIIN_AUDIO_MODE,
    AML_MIXER_ID_VAD_ENABLE,
    AML_MIXER_ID_VAD_SOURCE_SEL,
    AML_MIXER_ID_VAD_SWITCH,
    AML_MIXER_ID_DTV_CLK_TUNING,
    AML_MIXER_ID_I2S2HDMI_FORMAT,
    AML_MIXER_ID_SPDIF_OUT_CHANNEL_STATUS,
    AML_MIXER_ID_HDMIIN_NONAUDIO,
    AML_MIXER_ID_AED_MASTER_VOLUME,
    AML_MIXER_ID_AED_LCH_VOLUME,
    AML_MIXER_ID_AED_RCH_VOLUME,
    AML_MIXER_ID_DAC_PLAYBACK_VOLUME,
    AML_MIXER_ID_AI_SOUND_MODE,
    AML_MIXER_ID_AUDIO_PCPD_MONITOR_ENABLE,
    AML_MIXER_ID_AUDIO_PCPD_MONITOR_DATA_TYPE,
    AML_MIXER_ID_MAX,
} eMixerCtrlID;

/*
 *tinymix "Audio spdif format" list
 */
typedef enum AML_SPDIF_FORMAT {
    AML_STEREO_PCM = 0,
    AML_DTS_RAW_MODE = 1,
    AML_DOLBY_DIGITAL = 2,
    AML_DTS = 3,
    AML_DOLBY_DIGITAL_PLUS = 4,
    AML_DTS_HD = 5,
    AML_MULTI_CH_LPCM = 6,
    AML_TRUE_HD = 7,
    AML_DTS_HD_MA = 8,
    AML_HIGH_SR_STEREO_LPCM = 9,
} eMixerSpdif_Format;

/*
 *tinymix "arc/earc format" list
 *use new enum for eARC, it is same
 *with sound/soc/amlogic/common/iec_info.h
 */
typedef enum aml_arc_coding_types {
    AML_AUDIO_CODING_TYPE_UNDEFINED          = 0,

    /* LINEAR PCM */
    AML_AUDIO_CODING_TYPE_STEREO_LPCM        = 1,
    AML_AUDIO_CODING_TYPE_MULTICH_2CH_LPCM   = 2,
    AML_AUDIO_CODING_TYPE_MULTICH_8CH_LPCM   = 3,
    AML_AUDIO_CODING_TYPE_MULTICH_16CH_LPCM  = 4,
    AML_AUDIO_CODING_TYPE_MULTICH_32CH_LPCM  = 5,
    /* High bit rate */
    AML_AUDIO_CODING_TYPE_HBR_LPCM           = 6,

    /*
     * NON-LINEAR PCM
     * IEC61937-2, Burst-info, Data type
     */
    /* Dolby */
    /* AC3 Layout A */
    AML_AUDIO_CODING_TYPE_AC3                = 7,
    /* AC3 Layout B */
    AML_AUDIO_CODING_TYPE_AC3_LAYOUT_B       = 8,
    AML_AUDIO_CODING_TYPE_EAC3               = 9,
    AML_AUDIO_CODING_TYPE_MLP                = 10,
    /* DTS */
    AML_AUDIO_CODING_TYPE_DTS                = 11,
    AML_AUDIO_CODING_TYPE_DTS_HD             = 12,
    AML_AUDIO_CODING_TYPE_DTS_HD_MA          = 13,

    /* Super Audio CD, DSD (One Bit Audio) */
    AML_AUDIO_CODING_TYPE_SACD_6CH           = 14,
    AML_AUDIO_CODING_TYPE_SACD_12CH          = 15,

    /* Pause */
    AML_AUDIO_CODING_TYPE_PAUSE              = 16,
}eMixerARC_Format;

/*
 *tinymix "eARC_TX Channel Allocation" list
 *this control the channel allocation for multi channel pcm case,
 *please refer tp CTA-861-G-Final, Table 35 Audio InfoFrame Data Byte 4
 */
typedef enum aml_earc_channel_allocation {
    AML_EARC_CHANNEL_FL_FR                        = 0x0,   // normal 2.0 channel
    AML_EARC_CHANNEL_FL_FR_LFE1                   = 0x1,   // normal 2.1 channel
    AML_EARC_CHANNEL_FL_FR_FC                     = 0x2,
    AML_EARC_CHANNEL_FL_FR_LFE1_FC                = 0x3,
    AML_EARC_CHANNEL_FL_FR_BC                     = 0x4,
    AML_EARC_CHANNEL_FL_FR_LFE1_BC                = 0x5,
    AML_EARC_CHANNEL_FL_FR_FC_BC                  = 0x6,
    AML_EARC_CHANNEL_FL_FR_LFE1_FC_BC             = 0x7,
    AML_EARC_CHANNEL_FL_FR_LS_RS                  = 0x8,
    AML_EARC_CHANNEL_FL_FR_LFE1_LS_RS             = 0x9,
    AML_EARC_CHANNEL_FL_FR_FC_LS_RS               = 0xa,
    AML_EARC_CHANNEL_FL_FR_LFE1_FC_LS_RS          = 0xb,   // normal 5.1 channel
    AML_EARC_CHANNEL_FL_FR_LS_RS_BC               = 0xc,
    AML_EARC_CHANNEL_FL_FR_LFE1_LS_RS_BC          = 0xd,
    AML_EARC_CHANNEL_FL_FR_FC_LS_RS_BC            = 0xe,
    AML_EARC_CHANNEL_FL_FR_LFE1_FC_LS_RS_BC       = 0xf,
    AML_EARC_CHANNEL_FL_FR_LFE1_FC_LS_RS_RLC_RRC  = 0x13,  // normal 7.1 channel
    AML_EARC_CHANNEL_REFER_TO_SPEAKER_MASK        = 0xfe,  // Channels delivered according to the Speaker Mask (see section 6.6.3)
    AML_EARC_CHANNEL_REFER_TO_CHANNEL_INDEX       = 0xff,  // Channels delivered according to Channel Index (see section 6.6.4)
} eMixerEARC_Channel_Allocation;


/*
 *tinymix "Spdif to HDMITX Select" list
 */
enum AML_SRC_TO_HDMITX {
    AML_NONE_TO_HDMITX = -1,
    AML_SPDIF_A_TO_HDMITX,
    AML_SPDIF_B_TO_HDMITX,
    AML_TDM_A_TO_HDMITX,
    AML_TDM_B_TO_HDMITX,
    AML_TDM_C_TO_HDMITX,
    AML_INVALID_TO_HDMITX,
};


struct aml_mixer_list {
    int  id;
    char mixer_name[50];
};

struct aml_mixer_handle {
    struct mixer *pMixer;
    pthread_mutex_t lock;
};

int open_mixer_handle(struct aml_mixer_handle *mixer_handle);
int close_mixer_handle(struct aml_mixer_handle *mixer_handle);

/*
 * get interface
 **/
int aml_mixer_ctrl_get_array(struct aml_mixer_handle *mixer_handle, int mixer_id, void *array, int count);
int aml_mixer_ctrl_get_int(struct aml_mixer_handle *mixer_handle, int mixer_id);
int aml_mixer_ctrl_get_enum_str_to_int(struct aml_mixer_handle *mixer_handle, int mixer_id, int *ret);

// or
#if 0
int aml_mixer_get_audioin_src(int mixer_id);
int aml_mixer_get_i2sin_type(int mixer_id);
int aml_mixer_get_spdifin_type(int mixer_id);
#endif

/*
 * set interface
 **/
int aml_mixer_ctrl_set_array(struct aml_mixer_handle *mixer_handle, int mixer_id, void *array, int count);
int aml_mixer_ctrl_set_int(struct aml_mixer_handle *mixer_handle, int mixer_id, int value);
int aml_mixer_ctrl_set_str(struct aml_mixer_handle *mixer_handle, int mixer_id, char *value);

#ifdef __cplusplus
}
#endif

#endif
