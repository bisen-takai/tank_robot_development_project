/**
 * @file fixed_queue.c
 * @brief FixedQueueの実装。
 */

#include "fixed_queue.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

static bool validateQueueForUse(const FixedQueue *pst_queue);

/*---------------------------------------------------------------------------------------------------------------------
 * FixedQueue_Init
 *-------------------------------------------------------------------------------------------------------------------*/
PrimitiveApiResult FixedQueue_Init(
    FixedQueue *pst_queue,
    void *p_storage,
    size_t sz_storageBytes,
    uint32_t u4_capacity,
    size_t sz_elementSize)
{
    PrimitiveApiResult en_primitiveApiResult = PRIMITIVE_API_INVALID_ARGUMENT;
    size_t sz_requiredBytes = 0U;

    if ((pst_queue != NULL)
        && (p_storage != NULL)
        && (u4_capacity > 0U)
        && (sz_elementSize > 0U))
    {
        /* 必要バイト数を計算する前に、乗算のオーバーフローを防ぐ。 */
        if (u4_capacity <= (SIZE_MAX / sz_elementSize))
        {
            sz_requiredBytes = (size_t)u4_capacity * sz_elementSize;

            if (sz_requiredBytes <= sz_storageBytes)
            {
                if (pst_queue->instanceState == PRIMITIVE_INSTANCE_UNINITIALIZED)
                {
                    pst_queue->storage = p_storage;
                    pst_queue->capacity = u4_capacity;
                    pst_queue->elementSize = sz_elementSize;
                    pst_queue->readIndex = 0U;
                    pst_queue->writeIndex = 0U;
                    pst_queue->count = 0U;
                    pst_queue->highWater = 0U;
                    pst_queue->fullCount = 0U;
                    pst_queue->instanceState = PRIMITIVE_INSTANCE_READY;

                    en_primitiveApiResult = PRIMITIVE_API_OK;
                }
                else
                {
                    en_primitiveApiResult = PRIMITIVE_API_INVALID_STATE;
                }
            }
        }
    }

    return en_primitiveApiResult;
}

/*---------------------------------------------------------------------------------------------------------------------
 * FixedQueue_Push
 *-------------------------------------------------------------------------------------------------------------------*/
PrimitiveApiResult FixedQueue_Push(
    FixedQueue *pst_queue,
    const void *p_inputElement)
{
    PrimitiveApiResult en_primitiveApiResult = PRIMITIVE_API_INVALID_ARGUMENT;
    const uint8_t *pu1_input = (const uint8_t *)p_inputElement; /* 入力要素を型に依存しないバイト列としてコピーする。 */
    size_t sz_byteIndex = 0U;
    size_t sz_writeOffset = 0U;
    bool b_isConsistent = true;

    if ((pst_queue != NULL)
        && (p_inputElement != NULL))
    {
        if (pst_queue->instanceState == PRIMITIVE_INSTANCE_READY)
        {
            /* TODO: Common Runtimeの利用可能状態を確認する（詳細設計4.6節）。 */

            b_isConsistent = validateQueueForUse(pst_queue);

            if (b_isConsistent == false)
            {
                pst_queue->instanceState = PRIMITIVE_INSTANCE_FAULTED;
                /* TODO: Common Runtimeの健全性情報へ異常を通知する（詳細設計7.11節）。 */
                en_primitiveApiResult = PRIMITIVE_API_RUNTIME_UNAVAILABLE;
            }
            else if (pst_queue->count == pst_queue->capacity)
            {
                if (pst_queue->fullCount < UINT32_MAX)
                {
                    pst_queue->fullCount++;
                }

                en_primitiveApiResult = PRIMITIVE_API_FULL;
            }
            else
            {
                /* writeIndexは要素番号なので、バイト単位の位置へ変換する。 */
                sz_writeOffset = (size_t)pst_queue->writeIndex * pst_queue->elementSize;

                for (sz_byteIndex = 0U; sz_byteIndex < pst_queue->elementSize; sz_byteIndex++)
                {
                    pst_queue->storage[sz_writeOffset + sz_byteIndex] = pu1_input[sz_byteIndex];
                }

                if (pst_queue->writeIndex < (pst_queue->capacity - 1U))
                {
                    pst_queue->writeIndex++;
                }
                else
                {
                    pst_queue->writeIndex = 0U;
                }

                pst_queue->count++;

                if (pst_queue->count > pst_queue->highWater)
                {
                    pst_queue->highWater = pst_queue->count;
                }

                en_primitiveApiResult = PRIMITIVE_API_OK;
            }
        }
        else if (pst_queue->instanceState == PRIMITIVE_INSTANCE_UNINITIALIZED)
        {
            en_primitiveApiResult = PRIMITIVE_API_NOT_INITIALIZED;
        }
        else
        {
            en_primitiveApiResult = PRIMITIVE_API_RUNTIME_UNAVAILABLE;
        }
    }

    return en_primitiveApiResult;
}

/*---------------------------------------------------------------------------------------------------------------------
 * FixedQueue_Pop
 *-------------------------------------------------------------------------------------------------------------------*/
PrimitiveApiResult FixedQueue_Pop(
    FixedQueue *pst_queue,
    void *p_outputElement)
{
    PrimitiveApiResult en_primitiveApiResult = PRIMITIVE_API_INVALID_ARGUMENT;
    uint8_t *pu1_output = (uint8_t *)p_outputElement; /* 出力先をバイト単位で扱う。 */
    size_t sz_byteIndex = 0U;
    size_t sz_readOffset = 0U;
    bool b_isConsistent = true;

    if ((pst_queue != NULL)
        && (p_outputElement != NULL))
    {
        if (pst_queue->instanceState == PRIMITIVE_INSTANCE_READY)
        {
            /* TODO: Common Runtimeの利用可能状態を確認する（詳細設計4.6節）。 */

            b_isConsistent = validateQueueForUse(pst_queue);

            if (b_isConsistent == false)
            {
                pst_queue->instanceState = PRIMITIVE_INSTANCE_FAULTED;
                /* TODO: Common Runtimeの健全性情報へ異常を通知する（詳細設計7.11節）。 */
                en_primitiveApiResult = PRIMITIVE_API_RUNTIME_UNAVAILABLE;
            }
            else if (pst_queue->count == 0U)
            {
                en_primitiveApiResult = PRIMITIVE_API_EMPTY;
            }
            else
            {
                /* readIndexは要素番号なので、バイト単位の位置へ変換する。 */
                sz_readOffset = (size_t)pst_queue->readIndex * pst_queue->elementSize;

                for (sz_byteIndex = 0U; sz_byteIndex < pst_queue->elementSize; sz_byteIndex++)
                {
                    pu1_output[sz_byteIndex] = pst_queue->storage[sz_readOffset + sz_byteIndex];
                }

                if (pst_queue->readIndex < (pst_queue->capacity - 1U))
                {
                    pst_queue->readIndex++;
                }
                else
                {
                    pst_queue->readIndex = 0U;
                }

                pst_queue->count--;

                en_primitiveApiResult = PRIMITIVE_API_OK;
            }
        }
        else if (pst_queue->instanceState == PRIMITIVE_INSTANCE_UNINITIALIZED)
        {
            en_primitiveApiResult = PRIMITIVE_API_NOT_INITIALIZED;
        }
        else
        {
            en_primitiveApiResult = PRIMITIVE_API_RUNTIME_UNAVAILABLE;
        }
    }

    return en_primitiveApiResult;
}

/*---------------------------------------------------------------------------------------------------------------------
 * FixedQueue_Peek
 *-------------------------------------------------------------------------------------------------------------------*/
PrimitiveApiResult FixedQueue_Peek(
    FixedQueue *pst_queue,
    void *p_outputElement)
{
    PrimitiveApiResult en_primitiveApiResult = PRIMITIVE_API_INVALID_ARGUMENT;
    uint8_t *pu1_output = (uint8_t *)p_outputElement; /* 出力先をバイト単位で扱う。 */
    size_t sz_byteIndex = 0U;
    size_t sz_readOffset = 0U;
    bool b_isConsistent = true;

    if ((pst_queue != NULL)
        && (p_outputElement != NULL))
    {
        if (pst_queue->instanceState == PRIMITIVE_INSTANCE_READY)
        {
            /* TODO: Common Runtimeの利用可能状態を確認する（詳細設計4.6節）。 */

            b_isConsistent = validateQueueForUse(pst_queue);

            if (b_isConsistent == false)
            {
                pst_queue->instanceState = PRIMITIVE_INSTANCE_FAULTED;
                /* TODO: Common Runtimeの健全性情報へ異常を通知する（詳細設計7.11節）。 */
                en_primitiveApiResult = PRIMITIVE_API_RUNTIME_UNAVAILABLE;
            }
            else if (pst_queue->count == 0U)
            {
                en_primitiveApiResult = PRIMITIVE_API_EMPTY;
            }
            else
            {
                /* readIndexは要素番号なので、バイト単位の位置へ変換する。 */
                sz_readOffset = (size_t)pst_queue->readIndex * pst_queue->elementSize;

                for (sz_byteIndex = 0U; sz_byteIndex < pst_queue->elementSize; sz_byteIndex++)
                {
                    pu1_output[sz_byteIndex] = pst_queue->storage[sz_readOffset + sz_byteIndex];
                }

                en_primitiveApiResult = PRIMITIVE_API_OK;
            }
        }
        else if (pst_queue->instanceState == PRIMITIVE_INSTANCE_UNINITIALIZED)
        {
            en_primitiveApiResult = PRIMITIVE_API_NOT_INITIALIZED;
        }
        else
        {
            en_primitiveApiResult = PRIMITIVE_API_RUNTIME_UNAVAILABLE;
        }
    }

    return en_primitiveApiResult;
}

/*---------------------------------------------------------------------------------------------------------------------
 * FixedQueue_GetCount
 *-------------------------------------------------------------------------------------------------------------------*/
PrimitiveApiResult FixedQueue_GetCount(
    FixedQueue *pst_queue,
    uint32_t *pu4_count)
{
    PrimitiveApiResult en_primitiveApiResult = PRIMITIVE_API_INVALID_ARGUMENT;
    bool b_isConsistent = true;

    if ((pst_queue != NULL)
        && (pu4_count != NULL))
    {
        if (pst_queue->instanceState == PRIMITIVE_INSTANCE_READY)
        {
            /* TODO: Common Runtimeの利用可能状態を確認する（詳細設計4.6節）。 */

            b_isConsistent = validateQueueForUse(pst_queue);

            if (b_isConsistent == false)
            {
                pst_queue->instanceState = PRIMITIVE_INSTANCE_FAULTED;
                /* TODO: Common Runtimeの健全性情報へ異常を通知する（詳細設計7.11節）。 */
                en_primitiveApiResult = PRIMITIVE_API_RUNTIME_UNAVAILABLE;
            }
            else
            {
                *pu4_count = pst_queue->count;

                en_primitiveApiResult = PRIMITIVE_API_OK;
            }
        }
        else if (pst_queue->instanceState == PRIMITIVE_INSTANCE_UNINITIALIZED)
        {
            en_primitiveApiResult = PRIMITIVE_API_NOT_INITIALIZED;
        }
        else
        {
            en_primitiveApiResult = PRIMITIVE_API_RUNTIME_UNAVAILABLE;
        }
    }

    return en_primitiveApiResult;
}

/*---------------------------------------------------------------------------------------------------------------------
 * FixedQueue_GetStats
 *-------------------------------------------------------------------------------------------------------------------*/
PrimitiveApiResult FixedQueue_GetStats(
    FixedQueue *pst_queue,
    FixedQueueStats *pst_stats)
{
    PrimitiveApiResult en_primitiveApiResult = PRIMITIVE_API_INVALID_ARGUMENT;
    bool b_isConsistent = true;

    if ((pst_queue != NULL)
        && (pst_stats != NULL))
    {
        if (pst_queue->instanceState == PRIMITIVE_INSTANCE_READY)
        {
            /* TODO: Common Runtimeの利用可能状態を確認する（詳細設計4.6節）。 */

            b_isConsistent = validateQueueForUse(pst_queue);

            if (b_isConsistent == false)
            {
                pst_queue->instanceState = PRIMITIVE_INSTANCE_FAULTED;
                /* TODO: Common Runtimeの健全性情報へ異常を通知する（詳細設計7.11節）。 */
                en_primitiveApiResult = PRIMITIVE_API_RUNTIME_UNAVAILABLE;
            }
            else
            {
                pst_stats->usage.allocationRejectCount = pst_queue->fullCount;
                pst_stats->usage.capacity = pst_queue->capacity;
                pst_stats->usage.currentUsed = pst_queue->count;
                pst_stats->usage.highWater = pst_queue->highWater;
                pst_stats->fullCount = pst_queue->fullCount;

                en_primitiveApiResult = PRIMITIVE_API_OK;
            }
        }
        else if (pst_queue->instanceState == PRIMITIVE_INSTANCE_UNINITIALIZED)
        {
            en_primitiveApiResult = PRIMITIVE_API_NOT_INITIALIZED;
        }
        else
        {
            en_primitiveApiResult = PRIMITIVE_API_RUNTIME_UNAVAILABLE;
        }
    }

    return en_primitiveApiResult;
}

/*---------------------------------------------------------------------------------------------------------------------
 * validateQueueForUse
 *-------------------------------------------------------------------------------------------------------------------*/
static bool validateQueueForUse(const FixedQueue *pst_queue)
{
    bool b_isConsistent = true;

    if ((pst_queue->capacity == 0U)
        || (pst_queue->elementSize == 0U)
        || (pst_queue->storage == NULL)
        || (pst_queue->count > pst_queue->capacity)
        || (pst_queue->readIndex >= pst_queue->capacity)
        || (pst_queue->writeIndex >= pst_queue->capacity))
    {
        b_isConsistent = false;
    }

    return b_isConsistent;
}
