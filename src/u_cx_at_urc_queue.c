/*
 * Copyright 2024 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file
 * @brief Queue for incoming URCs
 */

#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#include "u_cx_log.h"
#include "u_cx_at_urc_queue.h"

#if U_CX_USE_URC_QUEUE == 1

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define U_URC_ENTRY_SIZE(ENTRY) \
    ((size_t)(ENTRY->strLineLen + 1 + ENTRY->payloadSize))

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static inline size_t getUnusedBuf(uCxAtUrcQueue_t *pUrcQueue)
{
    return pUrcQueue->bufferLen - pUrcQueue->bufferPos;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uCxAtUrcQueueInit(uCxAtUrcQueue_t *pUrcQueue, void *pBuffer, size_t bufferLen)
{
    memset(pUrcQueue, 0, sizeof(uCxAtUrcQueue_t));
    U_CX_MUTEX_CREATE(pUrcQueue->queueMutex);
    U_CX_MUTEX_CREATE(pUrcQueue->dequeueMutex);
    pUrcQueue->pBuffer = pBuffer;
    pUrcQueue->bufferLen = bufferLen;
}

void uCxAtUrcQueueDeInit(uCxAtUrcQueue_t *pUrcQueue)
{
    U_CX_MUTEX_DELETE(pUrcQueue->queueMutex);
    U_CX_MUTEX_DELETE(pUrcQueue->dequeueMutex);
}

bool uCxAtUrcQueueEnqueueBegin(uCxAtUrcQueue_t *pUrcQueue, const char *pUrcLine, size_t urcLineLen)
{
    bool ret;

    U_CX_MUTEX_LOCK(pUrcQueue->queueMutex);
    U_CX_AT_PORT_ASSERT(pUrcQueue->pEnqueueEntry == NULL);

    int32_t availableDataSpace = (int32_t)(getUnusedBuf(pUrcQueue) - sizeof(uUrcEntry_t));
    if (availableDataSpace >= (int32_t)urcLineLen + 1) {
        uUrcEntry_t *pEntry = (uUrcEntry_t *)&pUrcQueue->pBuffer[pUrcQueue->bufferPos];
        memcpy(&pEntry->data[0], pUrcLine, urcLineLen);
        pEntry->data[urcLineLen] = 0; // Add null term
        pEntry->strLineLen = (uint16_t)urcLineLen;
        pEntry->payloadSize = 0;
        pUrcQueue->bufferPos += sizeof(uUrcEntry_t) + urcLineLen + 1;
        pUrcQueue->pEnqueueEntry = pEntry;
        ret = true;
    } else {
        // Not enough space available
        U_CX_MUTEX_UNLOCK(pUrcQueue->queueMutex);
        ret = false;
    }

    return ret;
}

uint16_t uCxAtUrcQueueEnqueueGetPayloadPtr(uCxAtUrcQueue_t *pUrcQueue, uint8_t **ppPayload)
{
    U_CX_AT_PORT_ASSERT(pUrcQueue->pEnqueueEntry);

    uUrcEntry_t *pEntry = pUrcQueue->pEnqueueEntry;
    *ppPayload = &pEntry->data[pEntry->strLineLen + 1];
    return (uint16_t)getUnusedBuf(pUrcQueue);
}

void uCxAtUrcQueueEnqueueEnd(uCxAtUrcQueue_t *pUrcQueue, uint16_t payloadSize)
{
    U_CX_AT_PORT_ASSERT(pUrcQueue->pEnqueueEntry);
    U_CX_AT_PORT_ASSERT(getUnusedBuf(pUrcQueue) >= payloadSize);

    uUrcEntry_t *pEntry = pUrcQueue->pEnqueueEntry;
    pEntry->payloadSize = payloadSize;
    pUrcQueue->bufferPos += payloadSize;
    pUrcQueue->pEnqueueEntry = NULL;
    U_CX_MUTEX_UNLOCK(pUrcQueue->queueMutex);
}

void uCxAtUrcQueueEnqueueAbort(uCxAtUrcQueue_t *pUrcQueue)
{
    U_CX_AT_PORT_ASSERT(pUrcQueue->pEnqueueEntry);

    uint8_t *pEntry = (uint8_t *)pUrcQueue->pEnqueueEntry;
    pUrcQueue->bufferPos = (size_t)(pEntry - pUrcQueue->pBuffer);
    pUrcQueue->pEnqueueEntry = NULL;
    U_CX_MUTEX_UNLOCK(pUrcQueue->queueMutex);
}

uUrcEntry_t *uCxAtUrcQueueDequeueBegin(uCxAtUrcQueue_t *pUrcQueue)
{
    uUrcEntry_t *pEntry = NULL;

    if (U_CX_MUTEX_TRY_LOCK(pUrcQueue->dequeueMutex, 0) == 0) {
        U_CX_AT_PORT_ASSERT(pUrcQueue->pDequeueEntry == NULL);

        U_CX_MUTEX_LOCK(pUrcQueue->queueMutex);
        if (pUrcQueue->bufferPos > 0) {
            pEntry = (uUrcEntry_t *)&pUrcQueue->pBuffer[0];
        }
        U_CX_MUTEX_UNLOCK(pUrcQueue->queueMutex);

        if (pEntry) {
            pUrcQueue->pDequeueEntry = pEntry;
        } else {
            U_CX_MUTEX_UNLOCK(pUrcQueue->dequeueMutex);
        }
    }

    return pEntry;
}

void uCxAtUrcQueueDequeueEnd(uCxAtUrcQueue_t *pUrcQueue, uUrcEntry_t *pEntry)
{
    int32_t remainingData;
    U_CX_AT_PORT_ASSERT(pUrcQueue->pDequeueEntry != NULL);
    U_CX_AT_PORT_ASSERT(pUrcQueue->pDequeueEntry == pEntry);

    U_CX_MUTEX_LOCK(pUrcQueue->queueMutex);
    size_t totEntrySize = sizeof(uUrcEntry_t) + U_URC_ENTRY_SIZE(pEntry);
    remainingData = (int32_t)pUrcQueue->bufferPos - (int32_t)totEntrySize;
    if (remainingData > 0) {
        // Move the remaining data to start of buffer
        // TODO: Replace with ring buffer to improve performance
        memmove(pUrcQueue->pBuffer, &pUrcQueue->pBuffer[totEntrySize], (size_t)remainingData);
        pUrcQueue->bufferPos -= totEntrySize;
    } else {
        // This was the only entry so no need to move anything
        pUrcQueue->bufferPos = 0;
    }
    U_CX_MUTEX_UNLOCK(pUrcQueue->queueMutex);

    pUrcQueue->pDequeueEntry = NULL;

    U_CX_MUTEX_UNLOCK(pUrcQueue->dequeueMutex);
}

#endif // U_CX_USE_URC_QUEUE == 1
