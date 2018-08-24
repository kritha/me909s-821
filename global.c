#include "global.h"

globalData_t gData;

void emergency_sighandler(int signum)
{
    DEBUG_PRINTF("signum: %d", signum);
    exit(0);
}

//constructor func
void __attribute__((constructor)) initializer_before_main(void){
    //init global Data env
    bzero(&gData, sizeof(globalData_t));
    gData.tryCount = TRY_COUNT_INFINITE_SIGN;
    gData.refreshIntervelMSec = MONITOR_INTERVEL(REFRESH_INTERVEL);
    gData.dialIntervelMSec = MONITOR_INTERVEL(DIALING_INTERVEL);

    //deal with emergency situation
    signal(SIGINT, emergency_sighandler);
    signal(SIGKILL, emergency_sighandler);
    signal(SIGQUIT, emergency_sighandler);

    DEBUG_PRINTF();
}
