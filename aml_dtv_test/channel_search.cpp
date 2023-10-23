#include "channel_search.h"
#include <am_dmx.h>
#include <am_fend.h>
#include <am_debug.h>
#include <am_mw/am_scan.h>
#include <sys/wait.h>
#include "util.h"
#include "program_analysis.h"
#include "play_video.h"

#define FEND_DEV_NO 0
#define DMX_DEV_NO 0
sqlite3 *hdb;
pthread_mutex_t mutex_x = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_y = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
extern pthread_mutex_t mutex_play;
extern pthread_cond_t cond_play;

fe_modulation getDVBCModulationToInt(const char *modulation) {
	if (contains(modulation, "16QAM")) {
		return QAM_16;
	} else if (contains(modulation, "32QAM")) {
		return QAM_32;
	} else if (contains(modulation, "64QAM")) {
		return QAM_64;
	} else if (contains(modulation, "128QAM")) {
		return QAM_128;
	}  else if (contains(modulation, "256QAM")) {
		return QAM_256;
	} else {
		return QAM_AUTO;//随便给一个吧
	}
}

void dtv_scan_store(AM_SCAN_Result_t *result)
{
	int vid = -1 , vfmt = -1 , pid = -1, fmt = -1;
	AM_SCAN_TS_t *ts;
	//SCAN_ServiceInfo_t *service;
	service_list_t service_list;
	fprintf(stderr, "Storing tses ...\n");
	AM_SI_LIST_BEGIN(result->tses, ts) {
		service_list_t slist;
		scan_store_dvb_ts(result, ts, slist);
		service_list.merge(slist);
		slist.clear();
	}
	AM_SI_LIST_END()
	//遍历打印一波
	if(!service_list.empty() && service_list.size() > 0){
		//随便取出一个播
		SCAN_ServiceInfo_t * paly_info = service_list.front();
		if((paly_info->aud_info).audio_count > 0){
			vid = paly_info->vid; vfmt = paly_info->vfmt;
			pid = paly_info->aud_info.audios[0].pid; fmt = paly_info->aud_info.audios[0].fmt;
			fprintf(stderr, "paly info vid = %d , vfmt = %d , pid = %d , fmt = %d \n"
			, paly_info->vid, paly_info->vfmt, paly_info->aud_info.audios[0].pid
			, paly_info->aud_info.audios[0].fmt);
		}
		//for (service_list_t::iterator p = service_list.begin(); p != service_list.end(); p++)
		//{
			//fprintf(stderr, "service vid = %d  vfmt = %d audio_count = %d...\n",(*p)->vid
				//, (*p)->vfmt, ((*p)->aud_info).audio_count);
		//}
		//删除下需要
		for(SCAN_ServiceInfo_t* child : service_list) {
			fprintf(stderr, "rm vid = %d  ...\n", child->vid);
			delete child;
		}
		service_list.clear();
	}
	if(vid != -1 && vfmt!= -1 && pid != -1 && fmt != -1){
		AmTsPlayer(vid , vfmt, pid, fmt);
	}
}

static int print_result_callback(void *param, int col, char **values, char **names)
{
	int i, j, tc, tc1 = 0, tc2 = 0;
	for (i=0; i<col; i++)
	{
		fprintf(stderr, "%s ", names[i]);
		if (names[i])
			tc1 = strlen(names[i]);
		if (values[i])
			tc2 = strlen(values[i]);
		tc = tc2 - tc1;
		for (j=0; j<tc; j++)
			fprintf(stderr, " ");
	}
	fprintf(stderr, "\n");
	for (i=0; i<col; i++)
	{
		fprintf(stderr, "%s ", values[i]);
		if (names[i])
			tc1 = strlen(names[i]);
		if (values[i])
			tc2 = strlen(values[i]);
		tc = tc1 - tc2;
		for (j=0; j<tc; j++)
			fprintf(stderr, " ");
	}
	fprintf(stderr, "\n\n");
	return 0;
}

static void init_dtv_fe_paras(AM_FENDCTRL_DVBFrontendParameters_t *fpara, int cnt)
{
	int i;
	for (i=0; i<cnt; i++,fpara++)
	{
#if 0
		fpara->m_type = FE_OFDM;
		memset(&fpara->terrestrial.para, 0, sizeof(fpara->terrestrial.para));
		fpara->terrestrial.para.frequency = 474000000 + i*8000000;
		fpara->terrestrial.para.inversion = INVERSION_AUTO;
		fpara->terrestrial.para.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
		fpara->terrestrial.para.u.ofdm.code_rate_HP = FEC_AUTO;
		fpara->terrestrial.para.u.ofdm.code_rate_LP = FEC_AUTO;
		fpara->terrestrial.para.u.ofdm.constellation = QAM_AUTO;
		fpara->terrestrial.para.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
		fpara->terrestrial.para.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
		fpara->terrestrial.para.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
#else
		fpara->m_type = FE_QAM;
		memset(&fpara->cable.para, 0, sizeof(fpara->cable.para));
		fpara->cable.para.frequency = 474000000 + i*8000000;
		fpara->cable.para.u.qam.symbol_rate = 6875000;
		fpara->cable.para.u.qam.fec_inner = FEC_AUTO;
		fpara->cable.para.u.qam.modulation = QAM_64;
#endif
	}
}

static void init_atv_fe_paras(AM_FENDCTRL_DVBFrontendParameters_t *fpara, int cnt)
{
	int i;

	for (i=0; i<cnt; i++,fpara++)
	{
		fpara->m_type = FE_ANALOG;
		memset(&fpara->analog.para, 0, sizeof(fpara->analog.para));
		//fpara->analog.para.frequency = 200000000 + i*8000000;
		if (i == 0)
			fpara->analog.para.frequency = 40000000;
		else if (i==1)
			fpara->analog.para.frequency = 100000000;
		else if (i==2)
			fpara->analog.para.frequency = 40000000;
		else
			fpara->analog.para.frequency = 200000000 + i*8000000;
	}
}
static void progress_evt_callback(long dev_no, int event_type, void *param, void *user_data)
{
	char *errmsg = NULL;
	switch (event_type)
	{
		case AM_SCAN_EVT_PROGRESS:
		{
			AM_SCAN_Progress_t *prog = (AM_SCAN_Progress_t*)param;
			if (!prog)
				return;
			switch (prog->evt)
			{
				case AM_SCAN_PROGRESS_SCAN_BEGIN:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_SCAN_BEGIN EVT");
					break;
				case AM_SCAN_PROGRESS_SCAN_END:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_SCAN_END EVT");
					//搜索结束,退出搜索线程,wait那边退出,解锁,
					//这里和预期不符合,貌似已经开始销毁,但未结束,回调就过来了,播放
					//预期是先结束搜索再播放,但目前观察无影响,只需要下次来的搜索前结束播放和搜索即可
					pthread_cond_signal(&cond);
					break;
				case AM_SCAN_PROGRESS_NIT_BEGIN:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_NIT_BEGIN EVT");
					break;
				case AM_SCAN_PROGRESS_NIT_END:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_NIT_END EVT");
					break;
				case AM_SCAN_PROGRESS_TS_BEGIN:
					{
						struct dvb_frontend_parameters *p = (struct dvb_frontend_parameters*)prog->data;
						AM_DEBUG(1, "AM_SCAN_PROGRESS_TS_BEGIN EVT, frequency %u", p->frequency);
						break;
					}
				case AM_SCAN_PROGRESS_TS_END:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_TS_END EVT");
					break;
				case AM_SCAN_PROGRESS_PAT_DONE:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_PAT_DONE EVT");
					break;
				case AM_SCAN_PROGRESS_PMT_DONE:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_PMT_DONE EVT");
					break;
				case AM_SCAN_PROGRESS_CAT_DONE:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_CAT_DONE EVT");
					break;
				case AM_SCAN_PROGRESS_SDT_DONE:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_SDT_DONE EVT");
					{
						dvbpsi_sdt_t *sdts, *sdt;
						dvbpsi_sdt_service_t *srv;
						dvbpsi_descriptor_t *descr;
						char name[64];
						sdts = (dvbpsi_sdt_t *)prog->data;
						//依次取出节目名称并显

						AM_SI_LIST_BEGIN(sdts, sdt)
						AM_SI_LIST_BEGIN(sdt->p_first_service, srv)
						AM_SI_LIST_BEGIN(srv->p_first_descriptor, descr)
							if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SERVICE)
							{
								dvbpsi_service_dr_t *psd = (dvbpsi_service_dr_t*)descr->p_decoded;
								//取节目名称
								if (psd->i_service_name_length > 0)
								{
									//AM_EPG_ConvertCode((char*)psd->i_service_name, psd->i_service_name_length,\
												name, sizeof(name));
								    AM_SI_ConvertDVBTextCode((char*)psd->i_service_name, psd->i_service_name_length,\
												name, sizeof(name));
									AM_DEBUG(1,"Service [%s]", name);
								}
							}
						AM_SI_LIST_END()
						AM_SI_LIST_END()
						AM_SI_LIST_END()
					}
					break;
				case AM_SCAN_PROGRESS_STORE_BEGIN:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_STORE_BEGIN EVT");
					break;
				case AM_SCAN_PROGRESS_STORE_END:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_STORE_END EVT");
					//print the result select from dbase
					fprintf(stderr, "====================net_table=====================\n");
					sqlite3_exec(hdb, "select * from net_table", print_result_callback, NULL, &errmsg);
					fprintf(stderr, "====================ts_table=====================\n");
					sqlite3_exec(hdb, "select * from ts_table", print_result_callback, NULL, &errmsg);
					fprintf(stderr, "====================srv_table=====================\n");
					sqlite3_exec(hdb, "select * from srv_table order by chan_num", print_result_callback, NULL, &errmsg);
					break;
				case AM_SCAN_PROGRESS_NEW_PROGRAM:
					AM_DEBUG(1, "AM_SCAN_PROGRESS_NEW_PROGRAM");
					/* Notify the new searched programs */
					//AM_SCAN_ProgramProgress_t *pp = (AM_SCAN_ProgramProgress_t *)prog->data;
					//if (pp != NULL) {
					//	pT->mCurEv.mprogramType = pp->service_type;
					//	snprintf(pT->mCurEv.mProgramName, sizeof(pT->mCurEv.mProgramName), "%s", pp->name);
					//	pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
					//	pT->sendEvent(pT->mCurEv);
					//}
					break;
				default:
					AM_DEBUG(1, "Unkonwn EVT");
					break;
			}
		}
			break;
		default:
			break;
	}
}

static int start_scan_test(AM_FENDCTRL_DVBFrontendParameters_t *dtv_fes)
{
	AM_DMX_OpenPara_t open_para;
	AM_FEND_OpenPara_t fpara;
	memset(&fpara, 0, sizeof(fpara));
	fpara.mode = FE_ANALOG;
	AM_TRY(AM_FEND_Open(FEND_DEV_NO, &fpara));
	memset(&open_para, 0, sizeof(open_para));
	AM_TRY(AM_DMX_Open(DMX_DEV_NO, &open_para));
	AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_TS2);

	AM_SCAN_Handle_t hscan = NULL;//void 的一个fun
	int i, mode;
	AM_FENDCTRL_DVBFrontendParameters_t atv_fes[10];
	AM_Bool_t go = AM_TRUE, new_scan = AM_FALSE;
	AM_DB_Init(&hdb);//这TM,还得整个数据库？
	init_atv_fe_paras(atv_fes, AM_ARRAY_SIZE(atv_fes));
	//"auto"
	// mode = AM_SCAN_DTVMODE_AUTO|AM_SCAN_DTVMODE_SEARCHBAT;
	//"manual"
	mode = AM_SCAN_DTVMODE_MANUAL;
	//"allband"
	//mode = AM_SCAN_DTVMODE_ALLBAND;
	new_scan = AM_TRUE;
	//需要把前一次停了,感觉不需要,只要再最后销毁下就行吧
	//if (hscan)
	//{
	//	AM_EVT_Unsubscribe((long)hscan, AM_SCAN_EVT_PROGRESS, progress_evt_callback, NULL);
	//	AM_SCAN_Destroy(hscan, AM_TRUE);
	//	hscan == 0;
	//}
	AM_SCAN_CreatePara_t para;
	para.fend_dev_id = FEND_DEV_NO;
	para.hdb = hdb;
	para.store_cb = dtv_scan_store;
	para.mode = AM_SCAN_MODE_DTV_ATV;
	para.dtv_para.dmx_dev_id = DMX_DEV_NO;
	para.dtv_para.source = FE_QAM;
	para.dtv_para.fe_cnt = 1;//AM_ARRAY_SIZE(dtv_fes);
	para.dtv_para.fe_paras = dtv_fes;
	if(dtv_fes->m_type == FE_QPSK)
	{
		para.dtv_para.source = FE_QPSK;
		AM_SEC_DVBSatelliteEquipmentControl_t sec;
		memset(&sec, 0, sizeof(AM_SEC_DVBSatelliteEquipmentControl_t));
		sec.m_lnbs.m_lof_hi = 10750000;
		sec.m_lnbs.m_lof_lo = 9750000;
		sec.m_lnbs.m_lof_threshold = 10250000;
		sec.m_lnbs.SatCR_idx = -1;
		para.dtv_para.sat_para.sec = sec;
	}
	para.dtv_para.mode = mode;
	para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
	para.dtv_para.resort_all = AM_FALSE;
	para.dtv_para.standard = AM_SCAN_DTV_STD_DVB;
	//para.atv_para.fe_cnt = 0;
	//para.atv_para.fe_paras = atv_fes;
	//在am_scan.c的AM_SCAN_Create,atv_para.mode观察
	//猜测应该赋值AM_SCAN_ATVMODE_NONE就不会继续ATV搜索
	para.atv_para.mode = AM_SCAN_ATVMODE_NONE;
	//para.atv_para.direction = 1;
	//para.atv_para.afe_dev_id = 0;
	//para.atv_para.default_aud_std = 0;
	//para.atv_para.default_vid_std = 0;
	//para.atv_para.afc_range = 2000000;
	//para.atv_para.afc_unlocked_step = 1000000;
	//para.atv_para.cvbs_unlocked_step = 1500000;
	//para.atv_para.cvbs_locked_step = 6000000;
	para.dtv_para.clear_source = AM_FALSE;
	AM_SCAN_Create(&para, &hscan);
	//注册搜索进度通知事件
	AM_EVT_Subscribe((long)hscan, AM_SCAN_EVT_PROGRESS, progress_evt_callback, NULL);
	AM_SCAN_Start(hscan);
	fprintf(stderr, "start_into wait\n");
	//第一次进入,注册回调后,会释放这个mutex_y锁，然后wait这里等信号
	//下次进入,会先发信号,这边就拿到了,先进行销毁上一次的注册,退出结束后才释放mutex_x,第二次的线程才能进入搜台
	pthread_cond_wait(&cond, &mutex_y);
	AM_EVT_Unsubscribe((long)hscan, AM_SCAN_EVT_PROGRESS, progress_evt_callback, NULL);
	AM_SCAN_Destroy(hscan, AM_TRUE);//这里别存,存了由于数据库没路径销毁的时候会报错
	hscan == 0;
	AM_TRY(AM_DB_Quit(hdb));
	fprintf(stderr, "start_scan_test EXIT!!!\n");
	AM_DMX_Close(DMX_DEV_NO);
	AM_FEND_Close(FEND_DEV_NO);
	return 0;
}

void DVBC_Mode(const char * message){
	pthread_cond_signal(&cond);
	pthread_cond_signal(&cond_play);
	pthread_mutex_lock(&mutex_x);
	pthread_mutex_lock(&mutex_y);
	fprintf(stderr, "into DVBC_Mode!!!\n");
	//不同模式之间的区别,在于解析参数后,dtv参数的不同
	//DVBC这么个玩意cmd:SearchChannel_manual,mode:DVBC,frequency:113MHZ,dvbc_symbolRate:6.952,dvbc_modulation:64QAM
	//%[^:],读取不是:字符、%*[^:],读取不是:字符,但不取.%*[^0-9]%d%*[^:]:%f%*[^:]:%s
	int frequency; float symbolrate;
	char modulation[10] = { 0 };
	int result = sscanf(message, "%*[^0-9]%d%*[^:]:%f%*[^:]:%s", &frequency, &symbolrate, modulation);
	if(result < 0)
	{
		fprintf(stderr, "sscanf failed!!!\n");
	} else {
		fprintf(stderr, "frequency = %d , symbolrate = %f , modulation = %s !!!\n", frequency, symbolrate , modulation);
	}
	AM_FENDCTRL_DVBFrontendParameters_t * dtv_fes= new AM_FENDCTRL_DVBFrontendParameters_t();
	dtv_fes->m_type = FE_QAM;
	memset(&dtv_fes->cable.para, 0, sizeof(dtv_fes->cable.para));
	dtv_fes->cable.para.frequency = frequency * 1000000;
	dtv_fes->cable.para.u.qam.symbol_rate = symbolrate * 1000000;
	dtv_fes->cable.para.u.qam.fec_inner = FEC_AUTO;
	dtv_fes->cable.para.u.qam.modulation = getDVBCModulationToInt(modulation);
	start_scan_test(dtv_fes);
	delete dtv_fes;
	pthread_mutex_unlock(&mutex_x);
	pthread_mutex_unlock(&mutex_y);
	fprintf(stderr, "DVBC_Mode exit !!!\n");
}

void J83B_Mode(const char * message){
	pthread_cond_signal(&cond);
	pthread_cond_signal(&cond_play);
	pthread_mutex_lock(&mutex_x);
	pthread_mutex_lock(&mutex_y);
	//cmd:SearchChannel_manual,mode:J83B,frequency:63MHZ
	int frequency;
	int result = sscanf(message, "%*[^:]:%*[^:]:%*[^:]:%d%*s", &frequency);
	if(result < 0)
	{
		fprintf(stderr, "sscanf failed!!!\n");
	} else {
		fprintf(stderr, "frequency = %d !!!\n", frequency);
	}
	AM_FENDCTRL_DVBFrontendParameters_t * dtv_fes= new AM_FENDCTRL_DVBFrontendParameters_t();
	dtv_fes->m_type = FE_ATSC;
	memset(&dtv_fes->atsc.para, 0, sizeof(dtv_fes->atsc.para));
	dtv_fes->atsc.para.frequency = frequency * 1000000;
	dtv_fes->atsc.para.inversion = INVERSION_AUTO;
	dtv_fes->atsc.para.u.vsb.modulation = QAM_AUTO;
	//应该还差一个参数,需要设fpp_demod_local.deli_sys == SYS_DVBC_ANNEX_B?没找到例子
	start_scan_test(dtv_fes);
	delete dtv_fes;
	pthread_mutex_unlock(&mutex_x);
	pthread_mutex_unlock(&mutex_y);
}

void DVBS_Mode(const char * message){
	pthread_cond_signal(&cond);
	pthread_cond_signal(&cond_play);
	pthread_mutex_lock(&mutex_x);
	pthread_mutex_lock(&mutex_y);
	//cmd:SearchChannel_manual,mode:DVBS2,frequency:1000MHZ,dvbs_symbolRate:24500000
	int frequency, symbolrate;
	int result = sscanf(message, "%*[^:]:%*[^:]:%*[^:]:%d%*[^:]:%d", &frequency, &symbolrate);
	if(result < 0)
	{
		fprintf(stderr, "sscanf failed!!!\n");
	} else {
		fprintf(stderr, "frequency = %d , symbolrate = %d !!!\n", frequency * 1000, symbolrate);
	}
	AM_FENDCTRL_DVBFrontendParameters_t * dtv_fes= new AM_FENDCTRL_DVBFrontendParameters_t();
	dtv_fes->m_type = FE_QPSK;
	memset(&dtv_fes->sat.para, 0, sizeof(dtv_fes->sat.para));
	dtv_fes->sat.para.frequency = 10750000 + frequency * 1000;
	dtv_fes->sat.para.u.qpsk.symbol_rate = symbolrate;
	dtv_fes->sat.para.inversion = INVERSION_AUTO;
	dtv_fes->sat.para.u.qpsk.fec_inner = FEC_AUTO;
	//应该还需要设置一个东西,没太理解设的啥,暂时不需要？
//#ifdef USE_dvb_fend
//			AM_FEND_SetVoltage(fpp_demod_local.dev_no, voltage);
//#else
//			AML_HAL_FE_LnbVoltage(fpp_demod_local.dev_no, voltage);
//#endif
	start_scan_test(dtv_fes);
	delete dtv_fes;
	pthread_mutex_unlock(&mutex_x);
	pthread_mutex_unlock(&mutex_y);
}
