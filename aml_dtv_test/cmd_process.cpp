#include "cmd_process.h"
#include "util.h"
#include "channel_search.h"
#include "signal_status.h"
	
void ParseCmd(int conn, const char *cmd){
	char ack[1024] = "ACK: ";
	char ret[20] = "RET:";//固定的回复结果,就回个true或false,应该不会超过20
	fprintf(stderr, "-- send back msg0 --\n");
	strcat(ack, cmd);
	write(conn, ack, strlen(ack));
	if(startsWith(cmd, "cmd:SearchChannel_manual") == 0){
		fprintf(stderr, "-- cmd:SearchChannel_manual --\n");
		fprintf(stderr, "-- send back msg1 --\n");
		write(conn, "RET:true", strlen("RET:true"));
		fprintf(stderr, "-- send back msg1 end --\n");
		if (contains(cmd, "DVBS")){//DVBS和DVB-S2、DVBS2啥区别?
			fprintf(stderr, "--process DVBS--\n");
			DVBS_Mode(cmd);
		}else if(contains(cmd, "DVBC")){
			fprintf(stderr, "--process DVBC--\n");
			DVBC_Mode(cmd);
		}else if(contains(cmd, "J83B")){
			fprintf(stderr, "--process J83B--\n");
			J83B_Mode(cmd);
		}else if(contains(cmd, "DTMB")){
			fprintf(stderr, "--process DTMB--\n");
		}
	}else if(startsWith(cmd, "cmd:isSignalStable") == 0){
		fprintf(stderr, "-- cmd:isSignalStable --\n");
		strcat(ret, BOOL_TO_STR(isSignalStable()));
		fprintf(stderr, "ret = %s !!!\n", ret);
		write(conn, ret, strlen(ret));
	}else if(startsWith(cmd, "cmd:isSignalSmooth") == 0){
		fprintf(stderr, "-- cmd:isSignalSmooth --\n");
		strcat(ret, BOOL_TO_STR(isSignalSmooth()));
		fprintf(stderr, "ret = %s !!!\n", ret);
		write(conn, ret, strlen(ret));
	}else if(startsWith(cmd, "cmd:BoardReset") == 0){
		fprintf(stderr, "-- cmd:BoardReset --\n");
		write(conn, "RET:true", strlen("RET:true"));
		system("reboot");
	}else if(startsWith(cmd, "cmd:ClearAllChannel") == 0){
		fprintf(stderr, "-- cmd:ClearAllChannel --\n");
		write(conn, "RET:true", strlen("RET:true"));
	}else if(startsWith(cmd, "cmd:MuteRemote") == 0){
		fprintf(stderr, "-- cmd:MuteRemote --\n");
		system("echo 5 > /sys/devices/virtual/remote/amremote/protocol");
		//echo 1 >/sys/devices/virtual/remote/amremote/protocol
		write(conn, "RET:true", strlen("RET:true"));
	}else{
		fprintf(stderr, " unprocessed info \n");
	}
	fprintf(stderr, " ParseCmd End \n");
}