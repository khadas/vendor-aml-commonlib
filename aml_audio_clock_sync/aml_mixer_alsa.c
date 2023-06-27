// Copyright (C) 2023 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "aml_mixer_alsa.h"
#include "aml_hw_file.h"
#include "mixer.h"

struct aml_mixer_handle {
	struct mixer *pMixer;
	pthread_mutex_t lock;
};

static struct aml_mixer_list gAmlMixerList[] = {
	{AML_MIXER_ID_LOCKER_IN_SRC, "locker in src"},
	{AML_MIXER_ID_LOCKER_OUT_SINK, "locker out sink"},
	{AML_MIXER_ID_LOCKER_EN, "soft locker enable"},
	{AML_MIXER_ID_LOCKER_DIFF, "soft locker diff"},
	{AML_MIXER_ID_MCLK_FINE_SETTING, "TDM MCLK Fine Setting"},
};

static struct aml_mixer_handle * mixer_handle = NULL;

static char *_get_mixer_name_by_id(int mixer_id)
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

static struct mixer_ctl *_get_mixer_ctl_handle(struct mixer *pmixer, int mixer_id)
{
	struct mixer_ctl *pCtrl = NULL;

	if (_get_mixer_name_by_id(mixer_id) != NULL) {
		pCtrl = mixer_get_ctl_by_name(pmixer,_get_mixer_name_by_id(mixer_id));
	}

	return pCtrl;
}

static struct mixer *_open_mixer_handle(void)
{
	int card = 0;
	struct mixer *pmixer = NULL;

	if (mixer_handle != NULL) {
		return mixer_handle->pMixer;
	}

	mixer_handle = (struct aml_mixer_handle *)calloc(1,sizeof(struct aml_mixer_handle));

	card = aml_get_sound_card_main();
	if (card < 0) {
		printf("[%s:%d] Failed to get sound card\n", __FUNCTION__, __LINE__);
		return NULL;
	}

	pmixer = mixer_open(card);
	if (NULL == pmixer) {
		printf("[%s:%d] Failed to open mixer \n", __FUNCTION__, __LINE__);
		return NULL;
	}

	mixer_handle->pMixer = pmixer;
	pthread_mutex_init(&mixer_handle->lock, NULL);

	return pmixer;
}

int aml_mixer_ctrl_get_int(int mixer_id)
{
	struct mixer     *pMixer;
	struct mixer_ctl *pCtrl;
	int value = -1;

	pMixer = _open_mixer_handle();
	if (pMixer == NULL) {
		printf("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pthread_mutex_lock (&mixer_handle->lock);

	pCtrl = _get_mixer_ctl_handle(pMixer, mixer_id);
	if (pCtrl == NULL) {
		printf("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
		pthread_mutex_unlock (&mixer_handle->lock);
		return -1;
	}

	value = mixer_ctl_get_value(pCtrl, 0);
	pthread_mutex_unlock (&mixer_handle->lock);
	return value;
}

int aml_mixer_ctrl_set_int(int mixer_id, int value)
{
	struct mixer     *pMixer;
	struct mixer_ctl *pCtrl;

	pMixer = _open_mixer_handle();
	if (pMixer == NULL) {
		printf("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pthread_mutex_lock (&mixer_handle->lock);

	pCtrl = _get_mixer_ctl_handle(pMixer, mixer_id);
	if (pCtrl == NULL) {
		printf("[%s:%d] Failed to open mixer %s\n", __FUNCTION__, __LINE__,_get_mixer_name_by_id(mixer_id));
		return -1;
	}

	mixer_ctl_set_value(pCtrl, 0, value);
	pthread_mutex_unlock (&mixer_handle->lock);

	return 0;
}

void aml_close_mixer() {
	if (mixer_handle->pMixer != NULL) {
		mixer_close(mixer_handle->pMixer);
		mixer_handle->pMixer = NULL;
	}

	if (mixer_handle != NULL) {
		free(mixer_handle);
		mixer_handle = NULL;
	}
}
