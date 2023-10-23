#ifndef _PROGRAM_ANALYSIS_
#define _PROGRAM_ANALYSIS_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<string.h>
#include <am_mw/am_scan.h>
#include <list>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    int vid, vfmt;
    AM_SI_AudioInfo_t aud_info;
} SCAN_ServiceInfo_t;

typedef std::list<SCAN_ServiceInfo_t*> service_list_t;

void scan_store_dvb_ts(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, service_list_t &slist);

#ifdef __cplusplus
}
#endif

#endif
