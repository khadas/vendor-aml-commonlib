// Copyright (C) 2024 Amlogic, Inc. All rights reserved.
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

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "porting.h"

struct options {
	const char    *opt;
	unsigned char val;
};

/*
 * Options val is determined by the report descriptor, refer
 * to jira SWPL-166429. If the report descriptor changes,
 * the corresponding val needs to be modified accordingly.
 * */
static struct options volume_code[] = {
	{.opt = "--VolumeUp",	.val = 0x01},
	{.opt = "--VolumeDown",	.val = 0x02},
	{.opt = "--Mute",	.val = 0x04},
	{.opt = "--Play",	.val = 0x08},
	{.opt = NULL}
};


int uac_volume_command_parse(char* report, const char* buf)
{
	char *tok = strtok(buf, " ");
	int key = 0;
	int i = 0;

	for (; tok != NULL; tok = strtok(NULL, " ")) {
		for (i = 0; volume_code[i].opt != NULL; i++) {
			if (strcmp(tok, volume_code[i].opt) == 0) {
				*report = volume_code[i].val;
				break;
			}
		}
		if (volume_code[i].opt != NULL)
			continue;

		if (!(tok[0] == '-' && tok[1] == '-')) {
			SLOGE("Bad options:'%s'\n", tok);
			return -1;
		}
	}

	return 0;
}

void help(void)
{
	int i = 0;

	SLOGE("Usage:\n");
	SLOGE("\t\t uac_hid dev/hidgx --option:\n");
	SLOGE("Options:\n");
	for (i = 0; volume_code[i].opt != NULL; i++) {
		SLOGE("\t\t%s\n", volume_code[i].opt);
	}
}

int main(int argc, const char *argv[])
{
	const char *filename = NULL;
	int fd = 0;
	char report = 0;
	int ret = 0;

	if (argc < 2 || (strcmp(argv[1], "--help")) == 0) {
		help();
		return -1;
	}

	filename = argv[1];
	if ((fd = open(filename, O_RDWR, 0666)) == -1) {
		perror(filename);
		return -1;
	}

	ret = uac_volume_command_parse(&report, argv[2]);
	if (ret != 0 || 0 == report) {
		perror(filename);
		goto end;
	}

	ret = write(fd, &report, 1);
	if (ret != 1) {
		perror(filename);
	}

	/* stop volume up or volume down*/
	report = 0;
	ret = write(fd, &report, 1);
	if (ret != 1) {
		perror(filename);
		goto end;
	}
end:
	close(fd);

	return ret;
}

