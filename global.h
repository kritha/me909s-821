#ifndef GLOBAL_H
#define GLOBAL_H
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define BOXV3CHECKAPP_VERSION "V0.5.1"

#define BOXV3_NODEPATH_LTE   "/dev/huawei_lte"
#define BOXV3_BAUDRATE_UART 9600
#define BOXV3_ERRMSGBUF_LEN 1024
#define BUF_TMP_LENGTH    1024

#define AT_CMD_LENGTH_MAX 32
#define AT_CMD_SUFFIX   "\r\n"

#define LTE_MODULE_NETNODENAME "usb0"
#define INTERNET_ACCESS_POINT  "8.8.8.8"
#define NET_ACCESS_FAILEDCNT_MAX    2

#if 0
#define TIMESPEND_WHOLE_DIALING   24      //s
#else
#define TIMESPEND_WHOLE_DIALING   3      //s
#endif
#define TIMEINTERVAL_LTE_NET_CHECK (1000*TIMESPEND_WHOLE_DIALING)     //ms

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

enum parseEnum{
    NOTPARSEACK,
    PARSEACK_OK,
    PARSEACK_RESET,
    PARSEACK_AT,
    PARSEACK_CPIN,
    PARSEACK_REG,
    PARSEACK_COPS_CH_M,
    PARSEACK_COPS_CH_U,
    PARSEACK_COPS_CH_T,
    PARSEACK_NDISSTATQRY,
};

enum dialingStage{
    NOT_START_DIALING,
    STAGE1_AT_MODULE_OK,
    STAGE1_AT_MODULE_FAILED,
    DIALING_END_SUCCESS,
    DIALING_END_FAILED,
};

typedef struct _dialingResult{
    char isDialedOk;
    enum dialingStage stage;
}dialingResult_t;

extern errInfo_t errInfo;

#endif // GLOBAL_H

