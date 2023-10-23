#include "program_analysis.h"
#include "am_si.h"
#define IS_DVBT2_TS(_para) (_para.m_type==FE_OFDM && _para.terrestrial.ofdm_mode == OFDM_DVBT2)

void scan_init_service_info(SCAN_ServiceInfo_t *srv_info)
{
	memset(srv_info, 0, sizeof(SCAN_ServiceInfo_t));
	srv_info->vid = 0x1fff;
	srv_info->vfmt = -1;
}

void scan_store_dvb_ts(AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, service_list_t &slist)
{
	fprintf(stderr, "into scan_store_dvb_ts \n");
	dvbpsi_pmt_t *pmt;
	dvbpsi_pmt_es_t *es;
	dvbpsi_descriptor_t *descr;
	int src = result->start_para->dtv_para.source;
	int mode = result->start_para->dtv_para.mode;
	AM_Bool_t store = AM_TRUE;
	uint8_t plp_id;
	SCAN_ServiceInfo_t srv_info;
	fprintf(stderr, "@@ TS: src %d @@\n", src);
	//am_scan.h AM_SCAN_TS_t 内  digital AM_FENDCTRL_DVBFrontendParameters_t类型fend_para;
	//AM_FENDCTRL_DVBFrontendParametersTerrestrial_t terrestrial;
	//struct dvb_frontend_parameters para; "frontend.h" dvb_ofdm_parameters ofdm;
	if (ts->digital.pmts || (IS_DVBT2_TS(ts->digital.fend_para) && ts->digital.dvbt2_data_plp_num > 0)) {
		int loop_count, lc;
		dvbpsi_sdt_t *sdt_list;
		dvbpsi_pmt_t *pmt_list;
		dvbpsi_pat_t *pat_list;
		//For DVB-T2, search for each PLP, else search in current TS
		loop_count = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plp_num : 1;
		fprintf(stderr, "plp num %d \n", loop_count);
		for (lc = 0; lc < loop_count; lc++) {
			pmt_list = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].pmts : ts->digital.pmts;
			AM_SI_LIST_BEGIN(pmt_list, pmt) {
				SCAN_ServiceInfo_t *psrv_info = new SCAN_ServiceInfo_t();
				scan_init_service_info(psrv_info);
				AM_SI_LIST_BEGIN(pmt->p_first_es, es) {
					AM_SI_ExtractAVFromES(es, &psrv_info->vid, &psrv_info->vfmt, &psrv_info->aud_info);
				}
				AM_SI_LIST_END()
			fprintf(stderr, "push_back before vid = %d , vfmt = %d , pid = %d , fmt = %d \n" 
			, psrv_info->vid, psrv_info->vfmt, psrv_info->aud_info.audios[0].pid
			, psrv_info->aud_info.audios[0].fmt);
				/*Store this service*/
				slist.push_back(psrv_info);
			}
			AM_SI_LIST_END()
		}
	}
}