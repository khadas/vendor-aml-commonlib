#ifndef _SIGNAL_STATUS_H_
#define _SIGNAL_STATUS_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define _A_M  'S'
#define AMSTREAM_IOC_GET_MVDECINFO _IOR((_A_M), 0xcb, int)
#define BOOL_TO_STR(bool_expr) (bool_expr)? "true" :"false"
#ifdef __cplusplus
extern "C" {
#endif

struct vframe_comm_s {
	int vdec_id;
	char vdec_name[16];
	unsigned int vdec_type;
};

struct vframe_qos_s {
	unsigned int num;
	unsigned int type;
	unsigned int size;
	unsigned int pts;
	int max_qp;
	int avg_qp;
	int min_qp;
	int max_skip;
	int avg_skip;
	int min_skip;
	int max_mv;
	int min_mv;
	int avg_mv;
	int decode_buffer;
} /*vframe_qos */;

struct vframe_counter_s {
	struct vframe_qos_s qos;
	unsigned int  decode_time_cost;/*us*/
	unsigned int frame_width;
	unsigned int frame_height;
	unsigned int frame_rate;
	unsigned int bit_depth_luma;//original bit_rate;
	unsigned int frame_dur;
	unsigned int bit_depth_chroma;//original frame_data;
	unsigned int error_count;
	unsigned int status;
	unsigned int frame_count;
	unsigned int error_frame_count;
	unsigned int drop_frame_count;
	unsigned long long total_data;//this member must be 8 bytes alignment
	unsigned int double_write_mode;//original samp_cnt;
	unsigned int offset;
	unsigned int ratio_control;
	unsigned int vf_type;
	unsigned int signal_type;
	unsigned int pts;
	unsigned long long pts_us64;
};

struct av_param_mvdec_t {
	int vdec_id;
	/*This member is used for versioning this structure.
	*When passed from userspace, its value must be
	*sizeof(struct av_param_mvdec_t)
	*/
	int struct_size;
	int slots;
	struct vframe_comm_s comm;
	struct vframe_counter_s minfo[8];
};

bool isSignalStable();
bool isSignalSmooth();

#ifdef __cplusplus
}
#endif

#endif