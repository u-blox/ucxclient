#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "at_util.h"


typedef struct {
    char *pRxBuffer;
    size_t rxBufferLen;
    size_t rxBufferPos;
    bool executingCmd;
    const char *pExpectedRsp;
    size_t pExpectedRspLen;
    char *pRspParams;
    int status;
    void (*urcCallback)(char *pLine);
} atClient_t;



void atClient_init(void *pRxBuffer, size_t rxBufferLen, atClient_t *pClient);

void atClient_sendCmdVaList(atClient_t *pClient, const char *pCmd, const char *pParamFmt, va_list args);

int atClient_execSimpleCmdF(atClient_t *pClient, const char *pCmd, const char *pParamFmt, ...);
int atClient_execSimpleCmd(atClient_t *pClient, const char *pCmd);

void atClient_cmdBeginF(atClient_t *pClient, const char *pCmd, const char *pParamFmt, ...);

char *atClient_cmdGetRspParamLine(atClient_t *pClient, const char *pExpectedRsp);

int atClient_cmdGetRspParamsF(atClient_t *pClient, const char *pExpectedRsp, const char *pParamFmt, ...);

int atClient_cmdEnd(atClient_t *pClient);

