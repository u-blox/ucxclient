#include <string.h>
#include <stdbool.h>

#include "unity.h"
#include "u_cx_at_urc_queue.h"

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    const char *pString;
    const uSockIpAddress_t expectedAddr;
} uIpTestEntry_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uint8_t gBuffer[512];
static uCxAtUrcQueue_t gQueue;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TEST FUNCTIONS
 * -------------------------------------------------------------- */

void setUp(void)
{
    memset(&gBuffer[0], 1, sizeof(gBuffer));
    uCxAtUrcQueueInit(&gQueue, &gBuffer[0], sizeof(gBuffer));
}

void tearDown(void)
{
    uCxAtUrcQueueDeInit(&gQueue);
}

void test_queueingOfNonNullString_expectNullTermString(void)
{
    char myString[] = "FOO123!";
    char expected[] = "FOO123";
    TEST_ASSERT_TRUE(uCxAtUrcQueueEnqueueBegin(&gQueue, myString, strlen(expected)));
    uCxAtUrcQueueEnqueueEnd(&gQueue, 0);
    uUrcEntry_t *pEntry = uCxAtUrcQueueDequeueBegin(&gQueue);

    TEST_ASSERT_EQUAL(strlen(expected), pEntry->strLineLen);
    TEST_ASSERT_EQUAL_STRING(expected, pEntry->data);
}

void test_queueingWithPayload_expectPayload(void)
{
    char myString[] = "FOO123";
    uint8_t myPayload[] = { 0x00, 0x01, 0x02 };
    uint8_t *pPayload = NULL;

    TEST_ASSERT_TRUE(uCxAtUrcQueueEnqueueBegin(&gQueue, myString, strlen(myString)));
    size_t length = uCxAtUrcQueueEnqueueGetPayloadPtr(&gQueue, &pPayload);
    TEST_ASSERT_EQUAL(sizeof(gBuffer) - sizeof(uUrcEntry_t) - 1 - strlen(myString), length);
    memcpy(pPayload, &myPayload[0], sizeof(myPayload));
    uCxAtUrcQueueEnqueueEnd(&gQueue, sizeof(myPayload));

    uUrcEntry_t *pEntry = uCxAtUrcQueueDequeueBegin(&gQueue);
    TEST_ASSERT_NOT_NULL(pEntry);
    TEST_ASSERT_EQUAL(strlen(myString), pEntry->strLineLen);
    TEST_ASSERT_EQUAL_STRING(myString, pEntry->data);
    pPayload = &pEntry->data[pEntry->strLineLen + 1];
    TEST_ASSERT_EQUAL(sizeof(myPayload), pEntry->payloadSize);
    TEST_ASSERT_EQUAL_MEMORY(myPayload, pPayload, sizeof(myPayload));
}

void test_queueingMultiple_expectMultiple(void)
{
    char *myStrings[3] = { "FOO1", "FOO2", "FOO3" };
    uint8_t myPayloads[3][3] = {
        { 0x01, 0x01, 0x01 },
        { 0x02, 0x02, 0x02 },
        { 0x03, 0x03, 0x03 }
    };
    uint8_t *pPayload = NULL;

    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_TRUE(uCxAtUrcQueueEnqueueBegin(&gQueue, myStrings[i], strlen(myStrings[i])));
        uCxAtUrcQueueEnqueueGetPayloadPtr(&gQueue, &pPayload);
        memcpy(pPayload, &myPayloads[i][0], sizeof(myPayloads[i]));
        uCxAtUrcQueueEnqueueEnd(&gQueue, sizeof(myPayloads[i]));
    }

    for (int i = 0; i < 3; i++) {
        uUrcEntry_t *pEntry = uCxAtUrcQueueDequeueBegin(&gQueue);
        TEST_ASSERT_NOT_NULL(pEntry);
        TEST_ASSERT_EQUAL(strlen(myStrings[i]), pEntry->strLineLen);
        TEST_ASSERT_EQUAL_STRING(myStrings[i], pEntry->data);
        pPayload = &pEntry->data[pEntry->strLineLen + 1];
        TEST_ASSERT_EQUAL(sizeof(myPayloads[i]), pEntry->payloadSize);
        TEST_ASSERT_EQUAL_MEMORY(myPayloads[i], pPayload, sizeof(myPayloads[i]));
        uCxAtUrcQueueDequeueEnd(&gQueue, pEntry);
    }
}

void test_abortedQueueing_expectEmptyQueue(void)
{
    char myString[] = "FOO123";
    TEST_ASSERT_TRUE(uCxAtUrcQueueEnqueueBegin(&gQueue, myString, strlen(myString)));
    uCxAtUrcQueueEnqueueAbort(&gQueue);
    uUrcEntry_t *pEntry = uCxAtUrcQueueDequeueBegin(&gQueue);

    TEST_ASSERT_EQUAL(NULL, pEntry);
}

void test_uCxAtUrcQueueEnqueueBegin_withFullQueue_expectFailure(void)
{
    char myString[sizeof(gBuffer) / 2];
    memset(&myString[0], 'A', sizeof(myString));
    // Make sure we fill up the queue by queueing a string that is half the buffer size
    TEST_ASSERT_TRUE(uCxAtUrcQueueEnqueueBegin(&gQueue, myString, sizeof(myString)));
    uCxAtUrcQueueEnqueueEnd(&gQueue, 0);
    TEST_ASSERT_FALSE(uCxAtUrcQueueEnqueueBegin(&gQueue, myString, sizeof(myString)));
}

void test_uCxAtUrcQueueDequeueBegin_withEmptyQueue_expectNull(void)
{
    TEST_ASSERT_NULL(uCxAtUrcQueueDequeueBegin(&gQueue));
}

void test_uCxAtUrcQueueDequeueBegin_calledTwiceWithNonEmptyQueue_expectNull(void)
{
    char myString[] = "FOO123";
    TEST_ASSERT_TRUE(uCxAtUrcQueueEnqueueBegin(&gQueue, myString, strlen(myString)));
    uCxAtUrcQueueEnqueueEnd(&gQueue, 0);
    TEST_ASSERT_NOT_NULL(uCxAtUrcQueueDequeueBegin(&gQueue));
    TEST_ASSERT_NULL(uCxAtUrcQueueDequeueBegin(&gQueue));
}
