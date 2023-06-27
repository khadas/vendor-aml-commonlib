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

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "aml_mixer_alsa.h"

typedef struct anti_clk_drift_s
{
	int audio_locker_cnt;
	int src_alsa_device;
	int sink_alsa_device;
	bool is_init;
} anti_clk_drift_t;

int soft_lock_in = -1;
int soft_lock_out = -1;
int soft_lock_enable = -1;

#define CLK_DRIFT_THRESHOLD_MS  2
#define CLK_SETTING_OFFSET      1000000
#define CLK_appreciation_multiple 100
#define CLK_CHANGE_THRESHOLD 10000

static anti_clk_drift_t _anti_clk_drift = {
	.audio_locker_cnt = 0,
	.src_alsa_device = -1,
	.sink_alsa_device = -1,
	.is_init = false,
};

int aml_set_locker_in_src(int alsa_device)
{
	aml_mixer_ctrl_set_int(AML_MIXER_ID_LOCKER_IN_SRC, alsa_device);
	_anti_clk_drift.src_alsa_device = alsa_device;

	return 0;
}

int aml_set_locker_out_sink(int alsa_device)
{
	aml_mixer_ctrl_set_int(AML_MIXER_ID_LOCKER_OUT_SINK, alsa_device);
	_anti_clk_drift.sink_alsa_device = alsa_device;

	return 0;
}

int aml_set_locker_enable(int en)
{
	if (_anti_clk_drift.src_alsa_device == -1) {
		printf("[%s:%d] locker in src not set\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (_anti_clk_drift.sink_alsa_device == -1) {
		printf("[%s:%d] locker out sink not set\n", __FUNCTION__, __LINE__);
		return -1;
	}

	aml_mixer_ctrl_set_int(AML_MIXER_ID_LOCKER_EN, en);
	_anti_clk_drift.audio_locker_cnt = 0;
	_anti_clk_drift.is_init = true;
}

int aml_get_soft_locker_diff()
{
	if (!_anti_clk_drift.is_init) {
		printf("[%s:%d] soft locker is not init\n",  __FUNCTION__, __LINE__);
		return -1;
	}

	return aml_mixer_ctrl_get_int(AML_MIXER_ID_LOCKER_DIFF);
}

int aml_set_mclk(int val)
{
	aml_mixer_ctrl_set_int(AML_MIXER_ID_MCLK_FINE_SETTING, val);

	return 0;
}

int aml_get_parameter(int argc,char *argv[]) {
	int i;
	for (i = 1;i < argc;++i) {
		switch (argv[i][1])
		{
			case 'i':
				soft_lock_in = argv[i][3] - '0';
				break;
			case 'o':
				soft_lock_out = argv[i][3] - '0';
				break;
			case 'e':
				soft_lock_enable = argv[i][3] - '0';
			   break;
			case 'h':
				printf("-i : =soft_lock_in\n \
						-o : =soft_lock_out\n \
						-e : =soft_lock_enable\n");
				break;
			default:
				break;
		}
	}
}

void aml_process_ended() {
  aml_close_mixer();
  exit(1);
}

int main(int argc,const char *argv[])
{
	int diff_ms = 0;
	int previous_diff = 0;
	int tune_val;
	int mclk_appreciation = 0;
	int diff_difference = 0;

	aml_get_parameter(argc,argv);

	//Set soft lock related devices and enable soft lock.
	aml_set_locker_in_src(soft_lock_in);
	aml_set_locker_out_sink(soft_lock_out);
	aml_set_locker_enable(soft_lock_enable);
	signal(SIGTERM,aml_process_ended);

	if (!_anti_clk_drift.is_init) {
		printf("[%s:%d] clk anti drift is not init yet\n", __FUNCTION__, __LINE__);
		return 0;
	}
	//Get soft locker in out difference.
	previous_diff = aml_get_soft_locker_diff();
	while (1) {
		diff_ms = aml_get_soft_locker_diff();

		//If the diff value is greater than the threshold and in an upward trend, change CLK
		if (abs(diff_ms) > CLK_DRIFT_THRESHOLD_MS && abs(diff_ms) >= abs(previous_diff)) {
			diff_difference = diff_ms - previous_diff;
			mclk_appreciation = (diff_difference * abs(diff_difference) + diff_ms / 5) * CLK_appreciation_multiple;
			if (abs(mclk_appreciation) > CLK_CHANGE_THRESHOLD)
				mclk_appreciation = CLK_CHANGE_THRESHOLD * (mclk_appreciation/abs(mclk_appreciation));
			tune_val = CLK_SETTING_OFFSET - mclk_appreciation;
			aml_set_mclk(tune_val);
		}
		previous_diff = diff_ms;
		sleep(2);
	}
	aml_close_mixer();

	return 0;

}
