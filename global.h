#ifndef GLOBAL_H
#define GLOBAL_H
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#define BOXV3CHECKAPP_VERSION "V0.8.9"

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
#define NET_ACCESS_CHECKPOINT_MAXCNT 3

#define APN_NODE_INFO_LEGNTH    64

#define NET_ACCESS_LOG_DIR "/tmp/lte"
#define NET_ACCESS_LOG_FILENAME "logLTE.txt"
#define APNNODE_XML_CONFIG_FILE "/etc/dialLTE.config"

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
    STAGE_CCLK,
    STAGE_EONS,
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
    char startTime[TIME_BUFLEN];
    enum checkStageLTE isDialedSuccess;
    enum checkStageLTE currentOperator;
    enum checkStageLTE currentSlot;
    enum checkStageLTE currentStage;
    char endTime[TIME_BUFLEN];

    //module
    dialingBaseMsg_t cgmi;  //module operator info
    dialingBaseMsg_t cgsn;  //module AT soft version info
    dialingBaseMsg_t imei;  //module serial number info
    dialingBaseMsg_t gcap;  //means of communication of module support

    //sim
    dialingBaseMsg_t reset;
    dialingBaseMsg_t at;
    dialingBaseMsg_t swit;
    dialingBaseMsg_t cpin;
    dialingBaseMsg_t sysinfoex;
    dialingBaseMsg_t cops;
    dialingBaseMsg_t ndisdup;
    dialingBaseMsg_t qry;
    dialingBaseMsg_t cclk;
    dialingBaseMsg_t eons;
    dialingBaseMsg_t sleepcfg;


    dialingBaseMsg_t csq;
    dialingBaseMsg_t iccid;
    dialingBaseMsg_t chiptemp;

    dialingBaseMsg_t ip;
    dialingBaseMsg_t ping;
}dialingInfo_t;

typedef struct _apnNodeInfo{
    char apn[APN_NODE_INFO_LEGNTH];
    char name[APN_NODE_INFO_LEGNTH];
    char proxy[APN_NODE_INFO_LEGNTH];
    char server[APN_NODE_INFO_LEGNTH];
    char port[APN_NODE_INFO_LEGNTH];
    char user[APN_NODE_INFO_LEGNTH];
    char passwd[APN_NODE_INFO_LEGNTH];
    char mmsc[APN_NODE_INFO_LEGNTH];
    char mmsproxy[APN_NODE_INFO_LEGNTH];
    char mmsport[APN_NODE_INFO_LEGNTH];
    char mcc[APN_NODE_INFO_LEGNTH];
    char mnc[APN_NODE_INFO_LEGNTH];
    char mumeric[APN_NODE_INFO_LEGNTH];
    char type[APN_NODE_INFO_LEGNTH];
    char pingCheckDns[NET_ACCESS_CHECKPOINT_MAXCNT][APN_NODE_INFO_LEGNTH];

    dialingBaseMsg_t imeiLastTime;//module serial number which was recorded last time
    dialingBaseMsg_t iccidLastTime;//sim serial number which was recorded last time
    int dialingCnt;
    int refreshCnt;
    int successCnt;
    int failedCnt;
    dialingInfo_t* dialingInfo;  //current base dialing info
    struct _apnNodeInfo* next;
}apnNodeInfo_t;

extern errInfo_t errInfo;

#endif // GLOBAL_H

