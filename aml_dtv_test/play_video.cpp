#include "play_video.h"
#include <string>
#include "AmTsPlayer.h"
#include "amports/aformat.h"
#include "amports/vformat.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <semaphore.h>

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

const int kRwSize = 188*1024;
am_tsplayer_handle session;
extern pthread_mutex_t mutex_play;
extern pthread_cond_t cond_play;

static int set_osd_blank(int blank)
{
    /*const char *path1 = "/sys/class/graphics/fb0/blank";
    const char *path3 = "/sys/class/graphics/fb0/osd_display_debug";
    int fd;
    char cmd[128] = {0};
    fd = open(path3,O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(cmd,"%d",1);
        write (fd,cmd,strlen(cmd));
        close(fd);
    } else {
		fprintf(stderr, "debug path open failed!!\n");
	}
    fd = open(path1,O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(cmd,"%d",blank);
        write (fd,cmd,strlen(cmd));
        close(fd);
    } else {
		fprintf(stderr, "blank path open failed!!\n");
	}*/
	//换方式了
	const char *path = "/sys/kernel/debug/dri/0/vpu/blank";
	int fd;
    char cmd[128] = {0};
	fd = open(path ,O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(cmd,"%d",blank);
        write (fd,cmd,strlen(cmd));
        close(fd);
    } else {
		fprintf(stderr, "blank path open failed!!\n");
	}
    return 0;
}

void video_callback(void *user_data, am_tsplayer_event *event)
{
    if (!event) {
        return ;
    }
    UNUSED(user_data);
    fprintf(stderr, "video_callback type %d\n", event->type);
    switch (event->type) {
        case AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED:
        {
            fprintf(stderr, "[evt] AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED: %d x %d @%d [%d]\n",
                event->event.video_format.frame_width,
                event->event.video_format.frame_height,
                event->event.video_format.frame_rate,
                event->event.video_format.frame_aspectratio);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_USERDATA_AFD:
        case AM_TSPLAYER_EVENT_TYPE_USERDATA_CC:
        {
            uint8_t* pbuf = event->event.mpeg_user_data.data;
            uint32_t size = event->event.mpeg_user_data.len;
            fprintf(stderr, "[evt] USERDATA [%d] : %x-%x-%x-%x %x-%x-%x-%x ,size %d\n",
                event->type, pbuf[0], pbuf[1], pbuf[2], pbuf[3],
                pbuf[4], pbuf[5], pbuf[6], pbuf[7], size);
            UNUSED(pbuf);
            UNUSED(size);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME:
        {
            fprintf(stderr, "[evt] AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME\n");
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_VIDEO:
        {
            fprintf(stderr, "[evt] AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_VIDEO\n");
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_AUDIO:
        {
            fprintf(stderr, "[evt] AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_AUDIO\n");
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_AV_SYNC_DONE:
        {
            fprintf(stderr, "[evt] AM_TSPLAYER_EVENT_TYPE_AV_SYNC_DONE\n");
            break;
        }
        default:
            break;
    }
}
int dev_fd = -1;
int lastFrameCount = 0;
int	lastDropCount = 0;
void AmTsPlayer(int vid , int vfmt, int pid, int fmt)
{
	//进入播放的话,需要锁住
	pthread_mutex_lock(&mutex_play);
	char *fn = "/dev/amstream_userdata";//This file should not affect other dev file
	dev_fd = open(fn, O_RDONLY);//必须播放前就打开,不然打不开了,播放结束后关一下
	lastFrameCount = 0;//每次播放前,数据清零
	lastDropCount = 0;
	setenv("LOW_MEM_PLATFORM", "1", 1);
	setenv("XDG_RUNTIME_DIR", "//run//", 1);
	fprintf(stderr, "AmTsPlayer into\n");
	//拷贝的一波,aml-comp\multimedia\mediahal-sdk\example\AmTsPlayerExample
	//基本不改,主要改四个参数
    //文件直接 注释,没文件,std::string inputTsName("/data/1.ts");
	//t 4种类型,默认的话是3,由于>=0 <=2之外赋值成了TS_MEMORY,如果是从DTV直接搜到播,直接赋值0即可
	//TS_DEMOD = 0,                          // TS Data input from demod
	//TS_MEMORY = 1,                         // TS Data input from memory
	//ES_MEMORY = 2,                         // ES Data input from memory
	//TS_USB_CAMCARD = 3,                    // TS Data input from usb camcard
    am_tsplayer_input_source_type tsType = TS_DEMOD;
	//这个参数没改动,
    am_tsplayer_input_buffer_type drmmode = TS_INPUT_BUFFER_TYPE_NORMAL;
	//-y目前默认
    am_tsplayer_avsync_mode avsyncMode = TS_SYNC_AMASTER;
	//-c vtrick不知道是啥,给默认
    am_tsplayer_video_trick_mode vTrickMode = AV_VIDEO_TRICK_MODE_NONE;
    //-v video的编码格式,传入的是如下
	//aml-comp\vendor\amlogic\dvb\android\ndk\include\amports\vformat.h
	//typedef enum {
	//    VFORMAT_UNKNOWN = -1,
	//    VFORMAT_MPEG12 = 0,
	//    VFORMAT_MPEG4,
	//    VFORMAT_H264,
	//    VFORMAT_MJPEG,
	//    VFORMAT_REAL,
	//    VFORMAT_JPEG,
	//    VFORMAT_VC1,
	//    VFORMAT_AVS,
	//    VFORMAT_SW,
	//    VFORMAT_H264MVC,
	//    VFORMAT_H264_4K2K,
	//    VFORMAT_HEVC,
	//    VFORMAT_H264_ENC,
	//    VFORMAT_JPEG_ENC,
	//    VFORMAT_VP9,
	//    VFORMAT_AVS2,
	//
	////add new here before
	//    VFORMAT_MAX,
	//    VFORMAT_UNSUPPORT = VFORMAT_MAX
	//} vformat_t;
	//根据传入的,vfmt,需要找
	///*Video decoder type*/
	//typedef enum {
	//    AV_VIDEO_CODEC_AUTO = 0,               // Unknown video type (Unsupport)
	//    AV_VIDEO_CODEC_MPEG1 = 1,              // MPEG1
	//    AV_VIDEO_CODEC_MPEG2 = 2,              // MPEG2
	//    AV_VIDEO_CODEC_H264 = 3,               // H264
	//    AV_VIDEO_CODEC_H265 = 4,               // H265
	//    AV_VIDEO_CODEC_VP9 = 5,                // VP9
	//    AV_VIDEO_CODEC_AVS = 6,                // AVS
	//    AV_VIDEO_CODEC_MPEG4 = 7,              // MPEG4
	//好在就8个, unknown:0, mpeg1:1, mpeg2:2, h264:3[default], h265:4, vp9:5 avs:6 mpeg4:7
	am_tsplayer_video_codec vCodec = AV_VIDEO_CODEC_H264;
	switch(vfmt){
		//mpeg12不知道给1还是给2,暂时给2
		case VFORMAT_MPEG12:
			vCodec = AV_VIDEO_CODEC_MPEG2;
			break;
		case VFORMAT_H264:
			vCodec = AV_VIDEO_CODEC_H264;
			break;
		case VFORMAT_HEVC:
			vCodec = AV_VIDEO_CODEC_H265;
			break;
		case VFORMAT_VP9:
			vCodec = AV_VIDEO_CODEC_VP9;
			break;
		case VFORMAT_AVS:
			vCodec = AV_VIDEO_CODEC_AVS;
			break;
		case VFORMAT_MPEG4:
			vCodec = AV_VIDEO_CODEC_MPEG4;
			break;
		default :
			vCodec = AV_VIDEO_CODEC_AUTO;
	}
	//	typedef enum {
	//    AFORMAT_UNKNOWN = -1,
	//    AFORMAT_MPEG   = 0,
	//    AFORMAT_PCM_S16LE = 1,
	//    AFORMAT_AAC   = 2,
	//    AFORMAT_AC3   = 3,
	//    AFORMAT_ALAW = 4,
	//    AFORMAT_MULAW = 5,
	//    AFORMAT_DTS = 6,
	//    AFORMAT_PCM_S16BE = 7,
	//    AFORMAT_FLAC = 8,
	//    AFORMAT_COOK = 9,
	//    AFORMAT_PCM_U8 = 10,
	//    AFORMAT_ADPCM = 11,
	//    AFORMAT_AMR  = 12,
	//    AFORMAT_RAAC  = 13,
	//    AFORMAT_WMA  = 14,
	//    AFORMAT_WMAPRO   = 15,
	//    AFORMAT_PCM_BLURAY  = 16,
	//    AFORMAT_ALAC  = 17,
	//    AFORMAT_VORBIS    = 18,
	//    AFORMAT_AAC_LATM   = 19,
	//    AFORMAT_APE   = 20,
	//    AFORMAT_EAC3   = 21,
	//    AFORMAT_PCM_WIFIDISPLAY = 22,
	//    AFORMAT_DRA    = 23,
	//    AFORMAT_SIPR   = 24,
	//    AFORMAT_TRUEHD = 25,
	//    AFORMAT_MPEG1  = 26, //AFORMAT_MPEG-->mp3,AFORMAT_MPEG1-->mp1,AFROMAT_MPEG2-->mp2
	//    AFORMAT_MPEG2  = 27,
	//    AFORMAT_WMAVOI = 28,
	//    AFORMAT_WMALOSSLESS =29,
	//    AFORMAT_OPUS = 30,
	//    AFORMAT_UNSUPPORT ,
	//    AFORMAT_MAX
	//} aformat_t;
	/*Audio decoder type*/
	//typedef enum {
	//    AV_AUDIO_CODEC_AUTO = 0,               // Unknown audio type (Unsupport)
	//    AV_AUDIO_CODEC_MP2 = 1,                // MPEG audio
	//    AV_AUDIO_CODEC_MP3 = 2,                // MP3
	//    AV_AUDIO_CODEC_AC3 = 3,                // AC3
	//    AV_AUDIO_CODEC_EAC3 = 4,               // DD PLUS
	//    AV_AUDIO_CODEC_DTS = 5,                // DD PLUS
	//    AV_AUDIO_CODEC_AAC = 6,                // AAC
	//    AV_AUDIO_CODEC_LATM = 7,               // AAC LATM
	//    AV_AUDIO_CODEC_PCM = 8,                // PCM
	//unknown:0, mp2:1, mp3:2, ac3:3, eac3:4, dts:5, aac:6[default], latm:7, pcm:8
    am_tsplayer_audio_codec aCodec = AV_AUDIO_CODEC_AAC;
	switch(fmt){
		//case AFORMAT_MPEG2:
		//	aCodec = AV_AUDIO_CODEC_MP2;
		//	break;
		case AFORMAT_MPEG:
			aCodec = AV_AUDIO_CODEC_MP3;
			break;
		case AFORMAT_AC3:
			aCodec = AV_AUDIO_CODEC_AC3;
			break;
		case AFORMAT_EAC3:
			aCodec = AV_AUDIO_CODEC_EAC3;
			break;
		case AFORMAT_DTS:
			aCodec = AV_AUDIO_CODEC_DTS;
			break;
		case AFORMAT_AAC:
			aCodec = AV_AUDIO_CODEC_AAC;
			break;
		case AFORMAT_AAC_LATM:
			aCodec = AV_AUDIO_CODEC_LATM;
			break;
		//PCM有点多,不知道对应哪个,暂时带PCM就全设置成这个解码
		case AFORMAT_PCM_S16LE:
		case AFORMAT_PCM_S16BE:
		case AFORMAT_PCM_U8:
		case AFORMAT_PCM_BLURAY:
		case AFORMAT_PCM_WIFIDISPLAY:
			aCodec = AV_AUDIO_CODEC_PCM;
			break;			
		default :
			aCodec = AV_AUDIO_CODEC_AUTO;
	}
    int32_t vPid = 0x100;
    int32_t aPid = 0x101;
	//-t给2
    int32_t dmxSourceType = 2;
	//-d默认值
    int32_t dmxDevId = 0;
	//-g 默认吧
    int32_t gain = 20;
	//-p 的选项,也不动
    int32_t adtype = -1;
	//-o adpid,不知道啥意思,默认
    int32_t adpid = -1;
	//-q
    int32_t admixlevel = 50;
	vPid = vid;
	aPid = pid;
	
	//signal(SIGINT, signHandler);
    set_osd_blank(1);
	//没太理解,为啥是这个kRwSize = 188*1024长度
    char* buf = new char[kRwSize];
    //am_tsplayer_handle session;
    am_tsplayer_init_params parm = {tsType, drmmode, 0, 0};
    AmTsPlayer_create(parm, &session);
    int tunnelid = 0;
    AmTsPlayer_setSurface(session,(void*)&tunnelid);
    uint32_t versionM, versionL;
    AmTsPlayer_getVersion(&versionM, &versionL);
    uint32_t instanceno;
    AmTsPlayer_getInstansNo(session, &instanceno);
    AmTsPlayer_setWorkMode(session, TS_PLAYER_MODE_NORMAL);
    AmTsPlayer_registerCb(session, video_callback, NULL);

#if NO_AUDIO
    AmTsPlayer_setSyncMode(session, avsyncMode);
#endif
    am_tsplayer_video_params vparm;
    vparm.codectype = vCodec;
    vparm.pid = vPid;
    if (tsType == TS_DEMOD) {
        AmTsPlayer_setPcrPid(session,vPid);
    }
    AmTsPlayer_setVideoParams(session, &vparm);
    AmTsPlayer_startVideoDecoding(session);
#if NO_AUDIO
    if (adpid != -1 && adtype != -1 && tsType == TS_DEMOD) {
        am_tsplayer_audio_params adparm;
        adparm.codectype = static_cast<am_tsplayer_audio_codec>(adtype);
        adparm.pid = adpid;
        AmTsPlayer_setADParams(session,&adparm);
        AmTsPlayer_enableADMix(session);
        //master_vol no use,just set slave_vol
        AmTsPlayer_setADMixLevel(session, 0, admixlevel);
    } else {
        AmTsPlayer_disableADMix(session);
    }

    am_tsplayer_audio_params aparm;
    aparm.codectype = aCodec;
    aparm.pid = aPid;
    AmTsPlayer_setAudioParams(session, &aparm);
    AmTsPlayer_startAudioDecoding(session);

    AmTsPlayer_setAudioVolume(session, gain);
#endif

 	AmTsPlayer_showVideo(session);
    AmTsPlayer_setTrickMode(session, vTrickMode);
    bool bExitManual = false;
    am_tsplayer_input_buffer ibuf = {TS_INPUT_BUFFER_TYPE_NORMAL, (char*)buf, 0};
	fprintf(stderr, "----into pthread_cond_wait\n");
	//开始播放,后解锁,等待信号
	pthread_cond_wait(&cond_play, &mutex_play);
	close(dev_fd);
	fprintf(stderr, "----pthread_cond_wait exit\n");
    if (buf) {
        delete [](buf);
        buf = NULL;
    }
    set_osd_blank(0);
    AmTsPlayer_stopVideoDecoding(session);
    AmTsPlayer_stopAudioDecoding(session);
    AmTsPlayer_release(session);
    fprintf(stderr, "AmTsPlayer exit\n");
	//播放释放后,解锁 1.播放这部分,如果是Ctrl结束的话,主线程会发信号,释放播放这边的资源即可
	//2.如果是第二次播放命令过来的话,由于播放开始的锁,不会影响这边释放即可,释放结束才解锁
	pthread_mutex_unlock(&mutex_play);
}
