#include "signal_status.h"
#include <mutex>
#include<fcntl.h>
#include <sys/ioctl.h>

using namespace std;
mutex g_num_mutex;
extern int dev_fd;
extern int lastDropCount;
extern int lastFrameCount;
bool isSignalStable(){
	lock_guard<mutex> lock(g_num_mutex);
	fprintf(stderr, "into isSignalStable\n");
	int r,i;	
	int count = 10;
	struct av_param_mvdec_t para;
	if(dev_fd > -1)
	{
		while (count-- > 0) {//也不知道为啥需要循环10次,取不到的话,可能正忙
			memset(&para.minfo, 0, 8 * sizeof(struct vframe_counter_s));
			// Prepare the parameter
			para.vdec_id = 0;
			para.struct_size = sizeof(struct av_param_mvdec_t);
			fprintf(stderr, "para.struct_size= %u \n", para.struct_size);
			//Get the vdec frames information via ioctl
			r = ioctl(dev_fd, AMSTREAM_IOC_GET_MVDECINFO, &para);
			fprintf(stderr, "ioctl end \n");
			if (r < 0) {
				fprintf(stderr, "ioctl error for vdec %d, ret= %d \n", 0, r);
				break;
			} else if (para.slots == 0) {
				//The decoder does not provide the info yet, wait sometime
				usleep(100 * 1000);
				continue;
			}
			//Print the vdec frames related informations
			fprintf(stderr, "got %d frames info, vdec_id=%d\n" ,para.slots ,0);
			if (para.slots > 0) {
				i = para.slots -1; // get the lastes frame info
				fprintf(stderr, "slots is : %d\n", i);
				fprintf(stderr, "vdec_name : %s\n", para.comm.vdec_name);
				fprintf(stderr, "vdec_type : %d\n", para.comm.vdec_type);
				fprintf(stderr, "frame_width : %d\n", para.minfo[i].frame_width);
				fprintf(stderr, "frame_height : %d\n", para.minfo[i].frame_height);
				fprintf(stderr, "frame_rate : %d\n", para.minfo[i].frame_rate);
				fprintf(stderr, "double_write_mode : %d\n", para.minfo[i].double_write_mode);
				fprintf(stderr, "decode_time_cost : %d\n", para.minfo[i].decode_time_cost);
				fprintf(stderr, "error_count: %d\n", para.minfo[i].error_count);
				fprintf(stderr, "frame_count : %d\n", para.minfo[i].frame_count);
				fprintf(stderr, "frame_error_count : %d\n", para.minfo[i].error_frame_count);
				fprintf(stderr, "drop_frame : %d\n", para.minfo[i].drop_frame_count);
				fprintf(stderr, "----------------\n");
				//close(fd);
				fprintf(stderr, "lastFrameCount : %d\n", lastFrameCount);
				if(para.minfo[i].frame_count > lastFrameCount)
				{
					lastFrameCount = para.minfo[i].frame_count;
					return true;//貌似帧数有增加,就算稳定。。返回true
				}
				lastFrameCount = para.minfo[i].frame_count;
				return false;
			}
		}
	}else{
		fprintf(stderr, "%s, open /dev/amstream_userdata error", __FUNCTION__);
	}
	fprintf(stderr, "isSignalStable exit \n");
	return false;
}

bool isSignalSmooth(){
	lock_guard<mutex> lock(g_num_mutex);
	fprintf(stderr, "into isSignalSmooth\n");
	int r,i;	
	int count = 10;
	struct av_param_mvdec_t para;
	if(dev_fd > -1)
	{
		while (count-- > 0) {//也不知道为啥需要循环10次,取不到的话,可能正忙
			memset(&para.minfo, 0, 8 * sizeof(struct vframe_counter_s));
			// Prepare the parameter
			para.vdec_id = 0;
			para.struct_size = sizeof(struct av_param_mvdec_t);
			fprintf(stderr, "para.struct_size= %u \n", para.struct_size);
			//Get the vdec frames information via ioctl
			r = ioctl(dev_fd, AMSTREAM_IOC_GET_MVDECINFO, &para);
			fprintf(stderr, "ioctl end \n");
			if (r < 0) {
				fprintf(stderr, "ioctl error for vdec %d, ret= %d \n", 0, r);
				break;
			} else if (para.slots == 0) {
				//The decoder does not provide the info yet, wait sometime
				usleep(100 * 1000);
				continue;
			}
			//Print the vdec frames related informations
			fprintf(stderr, "got %d frames info, vdec_id=%d\n" ,para.slots ,0);
			if (para.slots > 0) {
				i = para.slots -1; // get the lastes frame info
				fprintf(stderr, "slots is : %d\n", i);
				fprintf(stderr, "vdec_name : %s\n", para.comm.vdec_name);
				fprintf(stderr, "vdec_type : %d\n", para.comm.vdec_type);
				fprintf(stderr, "frame_width : %d\n", para.minfo[i].frame_width);
				fprintf(stderr, "frame_height : %d\n", para.minfo[i].frame_height);
				fprintf(stderr, "frame_rate : %d\n", para.minfo[i].frame_rate);
				fprintf(stderr, "double_write_mode : %d\n", para.minfo[i].double_write_mode);
				fprintf(stderr, "decode_time_cost : %d\n", para.minfo[i].decode_time_cost);
				fprintf(stderr, "error_count: %d\n", para.minfo[i].error_count);
				fprintf(stderr, "frame_count : %d\n", para.minfo[i].frame_count);
				fprintf(stderr, "frame_error_count : %d\n", para.minfo[i].error_frame_count);
				fprintf(stderr, "drop_frame : %d\n", para.minfo[i].drop_frame_count);
				fprintf(stderr, "----------------\n");
				fprintf(stderr, "lastDropCount : %d\n", lastDropCount);
				//close(fd);
				//小于30帧时,丢一帧就算卡
				//大于30帧时,允许丢一帧,看不太出
				if (para.minfo[i].frame_count == 0 )
					return false;
				if ((para.minfo[i].drop_frame_count > lastDropCount && para.minfo[i].frame_rate <= 30) ||
					(para.minfo[i].drop_frame_count > lastDropCount + 1 && para.minfo[i].frame_rate > 30)) {
					lastDropCount = para.minfo[i].drop_frame_count;
					return false;
				}
				return true;	
			}
		}
	}else{
		fprintf(stderr, "%s, open /dev/amstream_userdata error", __FUNCTION__);
	}
	fprintf(stderr, "isSignalSmooth exit \n");
	return false;
}