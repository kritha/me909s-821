#include "global.h"

errInfo_t errInfo;

int tryCount = TRY_COUNT_INFINITE_SIGN;

void emergency_sighandler(int signum)
{
    DEBUG_PRINTF("signum: %d", signum);
    exit(0);
}

