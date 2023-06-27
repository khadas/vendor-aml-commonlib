/* mixer.c
**
** Copyright 2011, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
*/

#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "mixer.h"

void mixer_close(struct mixer *mixer)
{
	unsigned int n,m;

	if (!mixer)
		return;

	if (mixer->fd >= 0)
		close(mixer->fd);

	if (mixer->ctl) {
		for (n = 0; n < mixer->count; n++) {
			if (mixer->ctl[n].ename) {
				unsigned int max = mixer->ctl[n].info->value.enumerated.items;
				for (m = 0; m < max; m++)
					free(mixer->ctl[n].ename[m]);
				free(mixer->ctl[n].ename);
			}
		}
		free(mixer->ctl);
	}

	if (mixer->elem_info)
		free(mixer->elem_info);

	free(mixer);
}

struct mixer *mixer_open(unsigned int card)
{
	struct snd_ctl_elem_list elist;
	struct snd_ctl_elem_info tmp;
	struct snd_ctl_elem_id *eid = NULL;
	struct mixer *mixer = NULL;
	unsigned int n, m;
	int fd;
	char fn[256];

	snprintf(fn, sizeof(fn), "/dev/snd/controlC%u", card);
	fd = open(fn, O_RDWR);
	if (fd < 0)
		return 0;

	memset(&elist, 0, sizeof(elist));
	if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_LIST, &elist) < 0)
		goto fail;

	mixer = (struct mixer *)calloc(1, sizeof(*mixer));
	if (!mixer)
		goto fail;

	mixer->ctl = (struct mixer_ctl *)calloc(elist.count, sizeof(struct mixer_ctl));
	mixer->elem_info = (struct snd_ctl_elem_info*)calloc(elist.count, sizeof(struct snd_ctl_elem_info));
	if (!mixer->ctl || !mixer->elem_info)
		goto fail;

	if (ioctl(fd, SNDRV_CTL_IOCTL_CARD_INFO, &mixer->card_info) < 0)
		goto fail;

	eid = (struct snd_ctl_elem_id *)calloc(elist.count, sizeof(struct snd_ctl_elem_id));
	if (!eid)
		goto fail;

	mixer->count = elist.count;
	mixer->fd = fd;
	elist.space = mixer->count;
	elist.pids = eid;
	if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_LIST, &elist) < 0)
		goto fail;

	for (n = 0; n < mixer->count; n++) {
		struct snd_ctl_elem_info *ei = mixer->elem_info + n;
		ei->id.numid = eid[n].numid;
		if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_INFO, ei) < 0)
			goto fail;
		mixer->ctl[n].info = ei;
		mixer->ctl[n].mixer = mixer;
		if (ei->type == SNDRV_CTL_ELEM_TYPE_ENUMERATED) {
			char **enames = (char **)calloc(ei->value.enumerated.items, sizeof(char*));
			if (!enames)
				goto fail;
			mixer->ctl[n].ename = enames;
			for (m = 0; m < ei->value.enumerated.items; m++) {
				memset(&tmp, 0, sizeof(tmp));
				tmp.id.numid = ei->id.numid;
				tmp.value.enumerated.item = m;
				if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_INFO, &tmp) < 0)
					goto fail;
				enames[m] = strdup(tmp.value.enumerated.name);
				if (!enames[m])
					goto fail;
			}
		}
	}

	free(eid);
	return mixer;

fail:
	if (eid)
		free(eid);
	if (mixer)
		mixer_close(mixer);
	else if (fd >= 0)
		close(fd);
	return NULL;
}

struct mixer_ctl *mixer_get_ctl_by_name(struct mixer *mixer, const char *name)
{
	unsigned int n;

	if (!mixer)
		return NULL;

	for (n = 0; n < mixer->count; n++)
		if (!strcmp(name, (char*) mixer->elem_info[n].id.name))
			return mixer->ctl + n;

	return NULL;
}

int mixer_ctl_get_value(struct mixer_ctl *ctl, unsigned int id)
{
	struct snd_ctl_elem_value ev;
	int ret;

	if (!ctl || (id >= ctl->info->count))
		return -EINVAL;

	memset(&ev, 0, sizeof(ev));
	ev.id.numid = ctl->info->id.numid;
	ret = ioctl(ctl->mixer->fd, SNDRV_CTL_IOCTL_ELEM_READ, &ev);
	if (ret < 0)
		return ret;

	switch (ctl->info->type) {
	case SNDRV_CTL_ELEM_TYPE_BOOLEAN:
		return !!ev.value.integer.value[id];

	case SNDRV_CTL_ELEM_TYPE_INTEGER:
		return ev.value.integer.value[id];

	case SNDRV_CTL_ELEM_TYPE_ENUMERATED:
		return ev.value.enumerated.item[id];

	case SNDRV_CTL_ELEM_TYPE_BYTES:
		return ev.value.bytes.data[id];

	default:
		return -EINVAL;
	}

	return 0;
}

int mixer_ctl_set_value(struct mixer_ctl *ctl, unsigned int id, int value)
{
	struct snd_ctl_elem_value ev;
	int ret;

	if (!ctl || (id >= ctl->info->count))
		return -EINVAL;

	memset(&ev, 0, sizeof(ev));
	ev.id.numid = ctl->info->id.numid;
	ret = ioctl(ctl->mixer->fd, SNDRV_CTL_IOCTL_ELEM_READ, &ev);
	if (ret < 0)
		return ret;

	switch (ctl->info->type) {
	case SNDRV_CTL_ELEM_TYPE_BOOLEAN:
		ev.value.integer.value[id] = !!value;
		break;

	case SNDRV_CTL_ELEM_TYPE_INTEGER:
		ev.value.integer.value[id] = value;
		break;

	case SNDRV_CTL_ELEM_TYPE_ENUMERATED:
		ev.value.enumerated.item[id] = value;
		break;

	case SNDRV_CTL_ELEM_TYPE_BYTES:
		ev.value.bytes.data[id] = value;
		break;

	default:
		return -EINVAL;
	}

	return ioctl(ctl->mixer->fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &ev);
}
