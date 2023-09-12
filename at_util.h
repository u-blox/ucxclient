#ifndef AT_UTIL_H
#define AT_UTIL_H

#include <stdarg.h>
#include <stdint.h>

#ifndef ASSERT
# include <assert.h>
# define ASSERT assert
#endif

#define AT_UTIL_PARAM_LAST  NULL

void atUtil_byteToHex(uint8_t byte, char *pOutPtr);
int atUtil_hexToByte(char *pHex, uint8_t *pOutByte);
char *atUtil_findParamEnd(char *pStr);
int atUtil_parseParamsVaList(char *pParams, const char *pParamFmt, va_list args);
int atUtil_parseParamsF(char *pParams, const char *pParamFmt, ...);

#endif // AT_UTIL_H