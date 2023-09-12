#include <string.h>
#include <stdio.h>

#include "at_client.h"

void myUrc(char *pUrcLine)
{
    printf("Got URC: %s\n", pUrcLine);
}

int main(void)
{
    atClient_t client;

    char *s1;
    char *s2;
    int d1, d2;
    int len;
    uint8_t *pData;
    char buf[64];
    char rxBuf[1024];
    strcpy(buf, "\"hej\",123,hopp,-100,10200a0b0c01");
    int ret = atUtil_parseParamsF(buf, "sdsdb", &s1, &d1, &s2, &d2, &len, &pData);
    printf("ret: %d, s1: %s, d1: %d, s2: %s, d2: %d\n", ret, s1, d1, s2, d2);
    printf("len: %d, pData: %02x%02x%02x%02x%02x%02x\n", len, pData[0], pData[1], pData[2], pData[3], pData[4], pData[5]);
    atClient_init(rxBuf, sizeof(rxBuf), &client);
    atClient_execSimpleCmd(&client, "TESTING");
    client.urcCallback = myUrc;
    atClient_cmdBeginF(&client, "AT+HEJ=", "dhsb", 123, 65535, "foo", 3, "abc", AT_UTIL_PARAM_LAST);
    printf("\n");
    char *pRsp = atClient_cmdGetRspParamLine(&client, "+RSP");
    if (pRsp) {
        printf("Got response: %s\n", pRsp);
    }

    printf("status: %d\n", atClient_cmdEnd(&client));
}