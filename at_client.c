#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "at_util.h"
#include "at_client.h"

#define NO_STATUS   (INT_MAX)

size_t read(void *pData, size_t length)
{
    return fread(pData, 1, length, stdin);
}

void write(const void *pData, size_t length)
{
    fwrite(pData, 1, length, stdout);
}

enum atParserCode {
    AT_PARSER_NOP = 0,
    AT_PARSER_GOT_STATUS,
    AT_PARSER_GOT_RSP,
};

static int parseLine(atClient_t *pClient, char *pLine)
{
    int ret = AT_PARSER_NOP;

    if (pClient->executingCmd) {
        if ((pClient->pExpectedRsp != NULL) &&
            (strncmp(pLine, pClient->pExpectedRsp, pClient->pExpectedRspLen) == 0)) {
            pClient->pRspParams = &pLine[pClient->pExpectedRspLen + 1];
            ret = AT_PARSER_GOT_RSP;
        } else if (strcmp(pLine, "OK") == 0) {
            pClient->status = 0;
            ret = AT_PARSER_GOT_STATUS;
        } else if (strcmp(pLine, "ERROR") == 0) {
            pClient->status = -1;
            ret = AT_PARSER_GOT_STATUS;
        }
    }

    if (ret == AT_PARSER_NOP) {
        if ((pLine[0] == '+') || (pLine[0] == '*')) {
            if (pClient->urcCallback) {
                pClient->urcCallback(pLine);
            }
        }
    }

    return ret;
}

static int parseIncomingChar(atClient_t *pClient, char ch)
{
    int ret = AT_PARSER_NOP;

    if ((ch == '\r') || (ch == '\n')) {
        pClient->pRxBuffer[pClient->rxBufferPos] = 0;
        ret = parseLine(pClient, pClient->pRxBuffer);
        pClient->rxBufferPos = 0;
    } else if(isprint(ch)) {
        pClient->pRxBuffer[pClient->rxBufferPos++] = ch;
        if (pClient->rxBufferPos == pClient->rxBufferLen) {
            // Overflow - discard everything and start over
            pClient->rxBufferPos = 0;
        }
    }

    return ret;
}

static int handleRxData(atClient_t *pClient)
{
    int ret = AT_PARSER_NOP;
    char ch;

    while (read(&ch, 1) > 0) {
        ret = parseIncomingChar(pClient, ch);
        if (ret != AT_PARSER_NOP) {
            break;
        }
    }

    return ret;
}


static void cmdBeginF(atClient_t *pClient, const char *pCmd, const char *pParamFmt, va_list args)
{
    //handleRxData(pClient);

    pClient->pRspParams = NULL;
    pClient->executingCmd = true;
    pClient->status = NO_STATUS;
    atClient_sendCmdVaList(pClient, pCmd, pParamFmt, args);
}

static int cmdEnd(atClient_t *pClient)
{
    while (pClient->status == NO_STATUS) {
        handleRxData(pClient);
    }

    pClient->executingCmd = false;

    return pClient->status;
}


void atClient_init(void *pRxBuffer, size_t rxBufferLen, atClient_t *pClient)
{
    memset(pClient, 0, sizeof(atClient_t));
    pClient->pRxBuffer = pRxBuffer;
    pClient->rxBufferLen = rxBufferLen;
}

void atClient_sendCmdVaList(atClient_t *pClient, const char *pCmd, const char *pParamFmt, va_list args)
{
    char buf[16];

    write(pCmd, strlen(pCmd));
    const char *pCh = pParamFmt;
    while (*pCh != 0) {
        if (pCh != pParamFmt) {
            write(",", 1);
        }

        switch (*pCh) {
            case 'd':
                {
                    int i = va_arg(args, int);
                    int len = snprintf(buf, sizeof(buf), "%d", i);
                    write(buf, len);
                }
                break;
            case 'h':
                {
                    int i = va_arg(args, int);
                    int len = snprintf(buf, sizeof(buf), "%x", i);
                    write(buf, len);
                }
                break;
            case 's':
                {
                    char *pStr = va_arg(args, char *);
                    write(pStr, strlen(pStr));
                }
                break;
            case 'b':
                {
                    int len = va_arg(args, int);
                    uint8_t *pData = va_arg(args, uint8_t *);
                    for (int i = 0; i < len; i++) {
                        atUtil_byteToHex(pData[i], buf);
                        write(buf, 2);
                    }
                }
                break;
        }
        pCh++;
    }

    write("\r", 1);
}

int atClient_execSimpleCmdF(atClient_t *pClient, const char *pCmd, const char *pParamFmt, ...)
{
    va_list args;

    va_start(args, pParamFmt);
    cmdBeginF(pClient, pCmd, pParamFmt, args);
    va_end(args);

    return cmdEnd(pClient);
}

int atClient_execSimpleCmd(atClient_t *pClient, const char *pCmd)
{
    cmdBeginF(pClient, pCmd, "", NULL);

    return cmdEnd(pClient);
}

void atClient_cmdBeginF(atClient_t *pClient, const char *pCmd, const char *pParamFmt, ...)
{
    va_list args;

    va_start(args, pParamFmt);
    cmdBeginF(pClient, pCmd, pParamFmt, args);
    va_end(args);
}

char *atClient_cmdGetRspParamLine(atClient_t *pClient, const char *pExpectedRsp)
{
    char *pRet = NULL;
    pClient->pRspParams = NULL;
    pClient->pExpectedRsp = pExpectedRsp;
    pClient->pExpectedRspLen = strlen(pExpectedRsp);

    while (pClient->status == NO_STATUS) {
        if (handleRxData(pClient) == AT_PARSER_GOT_RSP) {
            pRet = pClient->pRspParams;
            break;
        }
    }

    return pRet;
}

int atClient_cmdGetRspParamsF(atClient_t *pClient, const char *pExpectedRsp, const char *pParamFmt, ...)
{
    va_list args;
    char *pRspParams = atClient_cmdGetRspParamLine(pClient, pExpectedRsp);

    va_start(args, pParamFmt);
    int ret = atUtil_parseParamsVaList(pRspParams, pParamFmt, args);
    va_end(args);

    return ret;
}

int atClient_cmdEnd(atClient_t *pClient)
{
    return cmdEnd(pClient);
}
