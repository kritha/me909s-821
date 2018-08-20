#ifndef GLOBAL_H
#define GLOBAL_H
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#define BOXV3CHECKAPP_VERSION "V0.8.7"

#define BOXV3_NODEPATH_LTE   "/dev/huawei_lte"
#define BOXV3_NODEPATH_LENGTH   128
#define BOXV3_BAUDRATE_UART 115200
#define BOXV3_ERRMSGBUF_LEN 1024
#define BUF_TMP_LENGTH    1024
#define AT_ACK_RESULT_INFO_LENGTH 128
#define SIM_CSQ_SIGNAL_MIN  17
#define SIM_TEMP_VALUE_MAX  650

#define AT_CMD_LENGTH_MAX 64
#define AT_CMD_SUFFIX   "\r\n"

#define LTE_MODULE_NETNODENAME "usb0"
#define INTERNET_ACCESS_POINT  "8.8.8.8"
#define NET_ACCESS_FAILEDCNT_MAX    2

#define NET_ACCESS_LOG_DIR "/tmp/lte"
#define NET_ACCESS_LOG_FILENAME "logLTE.txt"

#define TIMESPEND_WHOLE_DIALING   30      //s

#define TIMEINTERVAL_LTE_NET_CHECK (1000*TIMESPEND_WHOLE_DIALING)     //ms
#define MONITOR_TIMER_CHECK_INTERVAL    (1000*1) //ms
#define MONITOR_TIMER_CHECK_SPECIAL_MULT    7

/*
 * Save __FILE__ ,__FUNCTION__, __LINE__, and err msg, when err occured.
*/
#define ERR_RECORDER(notifyMsg) do{ \
        bzero(&errInfo, sizeof(errInfo_t));\
        errInfo.filep = (char*)__FILE__;\
        errInfo.funcp = (char*)__func__;\
        errInfo.line = __LINE__;\
        if(notifyMsg){\
            strcpy(errInfo.errMsgBuf, NULL==notifyMsg?"\n":notifyMsg);\
        }\
    }while(0)

#define DEBUG_PRINTF(fmt, args...) printf("---debug---%s(line:%d)"fmt"\n", __func__, __LINE__, ##args)

typedef struct _errInfo{
    char* filep;
    char* funcp;
    int line;
    char errMsgBuf[BOXV3_ERRMSGBUF_LEN];
}errInfo_t;

enum checkStageLTE{
    STAGE_UNKNOWN,
    STAGE_WAIT_SHORT,

    STAGE_MODE_DIAL,
    STAGE_MODE_REFRESH,

    STAGE_RESET,
    STAGE_DEFAULT,
    STAGE_INITENV,
    STAGE_NODE,
    STAGE_AT,
    STAGE_ICCID,
    STAGE_SIMSWITCH,
    STAGE_CPIN,
    STAGE_SYSINFOEX,
    STAGE_COPS,
    STAGE_CSQ,
    STAGE_CHIPTEMP,
    STAGE_NDISDUP,
    STAGE_NDISSTATQRY,
    STAGE_GET_SPECIAL_DATA,
    STAGE_CHECK_IP,
    STAGE_CHECK_PING,

    STAGE_RESULT_SUCCESS,
    STAGE_RESULT_FAILED,
    STAGE_RESULT_UNKNOWN,

    STAGE_PARSE_SIMPLE,
    STAGE_PARSE_OFF,

    STAGE_OPERATOR_MOBILE,
    STAGE_OPERATOR_TELECOM,
    STAGE_OPERATOR_UNICOM,

    STAGE_SLOT_0,
    STAGE_SLOT_1,
    STAGE_SLOT_2_bck,
    STAGE_SLOT_3_bck,
    STAGE_SLOT_4_bck,


    STAGE_DISPLAY_INIT,
    STAGE_DISPLAY_BOX,
    STAGE_DISPLAY_LINEEDIT,
    STAGE_DISPLAY_NOTES,
    STAGE_DISPLAY_CNT_SUCCESS,
    STAGE_DISPLAY_CNT_FAILED,
};
typedef struct _dialingBaseMsg{
    char checkCnt;
    enum checkStageLTE result;
    char meAckMsg[AT_ACK_RESULT_INFO_LENGTH];
}dialingBaseMsg_t;

typedef struct _dialingInfo{
#define TIME_BUFLEN 32
    int dialingCnt;
    char startTime[TIME_BUFLEN];
    enum checkStageLTE isDialedSuccess;
    enum checkStageLTE currentOperator;
    enum checkStageLTE currentSlot;
    enum checkStageLTE currentStage;
    char endTime[TIME_BUFLEN];

    dialingBaseMsg_t reset;
    dialingBaseMsg_t at;
    dialingBaseMsg_t swit;
    dialingBaseMsg_t cpin;
    dialingBaseMsg_t sysinfoex;
    dialingBaseMsg_t cops;
    dialingBaseMsg_t ndisdup;
    dialingBaseMsg_t qry;

    dialingBaseMsg_t csq;
    dialingBaseMsg_t iccid;
    dialingBaseMsg_t chiptemp;

    dialingBaseMsg_t ip;
    dialingBaseMsg_t ping;
}dialingInfo_t;

extern errInfo_t errInfo;

#endif // GLOBAL_H

