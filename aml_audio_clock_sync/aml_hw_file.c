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

#include "aml_hw_file.h"

#define SOUND_CARDS_PATH "/proc/asound/cards"
int aml_get_sound_card_main()
{
	int card = -1, err = 0;
	int fd = -1;
	unsigned fileSize = 512;
	char *read_buf = NULL, *pd = NULL;

	fd = open(SOUND_CARDS_PATH, O_RDONLY);
	if (fd < 0) {
		printf("[%s:%d] Failed to open config file %s\n", __FUNCTION__, __LINE__, SOUND_CARDS_PATH);
		close(fd);
		return -EINVAL;
	}

	read_buf = (char *) malloc(fileSize);
	if (!read_buf) {
		printf("[%s:%d] Failed to malloc read_buf\n", __FUNCTION__, __LINE__);
		close(fd);
		return -ENOMEM;
	}
	memset(read_buf, 0x0, fileSize);
	err = read(fd, read_buf, (fileSize-1));
	if (err < 0) {
		printf("[%s:%d] Failed to read config file %s\n", __FUNCTION__, __LINE__, SOUND_CARDS_PATH);
		close(fd);
		free(read_buf);
		return -EINVAL;
	}
	pd = strstr(read_buf, "AML");
	if ((pd != NULL) && (pd >= (read_buf+3))) {
		card = *(pd - 3) - '0';
	}

	free(read_buf);
	close(fd);
	return card;
}
