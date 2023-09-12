#ifndef AT_URC_DISPATCHER_H
#define AT_URC_DISPATCHER_H

#include <stdarg.h>
#include <stdint.h>


typedef void (*urcCallback_t)(char *pUrcLine);

void atUrcDispatcher_init(void);
void atUrcDispatcher_registerListener(const char *pUrc, urcCallback_t callback);
void atUrcDispatcher_unregisterListener(const char *pUrc);

void atUrcDispatcher_notify(char *pUrcLine);

#endif // AT_URC_DISPATCHER_H