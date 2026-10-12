/**
 * @file fixed_queue.h
 * @brief FixedQueueの公開型および公開関数を定義する。
 */

#ifndef FIXED_QUEUE_H
#define FIXED_QUEUE_H

#include <stddef.h>
#include <stdint.h>

#include "common_primitive_types.h"

/**
 * @brief プロセスコンテキスト専用の固定容量FIFOキュー。
 *
 * @note 初期化後の管理メンバは呼出し側から直接変更せず、
 *       FixedQueueの公開APIを使用して操作する。
 */
typedef struct
{
    uint8_t *storage;                      /* 要素を保持する固定格納領域。 */
    uint32_t capacity;                     /* 格納可能な最大要素数。 */
    size_t elementSize;                    /* 1要素のサイズ（バイト）。 */
    uint32_t readIndex;                    /* 次に読み出すスロット位置。 */
    uint32_t writeIndex;                   /* 次に書き込むスロット位置。 */
    uint32_t count;                        /* 現在格納している要素数。 */
    uint32_t highWater;                    /* MCU起動後のcount最大値。 */
    uint32_t fullCount;                    /* FULLによりPushを拒否した回数。 */
    PrimitiveInstanceState instanceState;  /* Primitiveインスタンスの初期化状態。 */
} FixedQueue;

/**
 * @brief FixedQueueの使用状況および統計情報。
 */
typedef struct
{
    RuntimeResourceUsageSnapshot usage;    /* 固定リソースの共通使用状況。 */
    uint32_t fullCount;                    /* FULLによりPushを拒否した回数。 */
} FixedQueueStats;

/**
 * @brief FixedQueueを初期化する。
 *
 * @param[in,out] pst_queue 初期化するFixedQueueのアドレス。
 * @param[in] p_storage 要素を保持する固定格納領域のアドレス。
 *                      格納領域は静的に確保し、MCU起動期間中は保持する。
 * @param[in] sz_storageBytes 実際に確保した格納領域のサイズ（バイト）。
 *                            capacity × elementSize以上を指定する。
 * @param[in] u4_capacity 格納可能な最大要素数。
 * @param[in] sz_elementSize 1要素のサイズ（バイト）。
 *
 * @return 処理結果。
 */
PrimitiveApiResult FixedQueue_Init(
    FixedQueue *pst_queue,
    void *p_storage,
    size_t sz_storageBytes,
    uint32_t u4_capacity,
    size_t sz_elementSize);

/**
 * @brief 要素をキューの末尾へ格納する。
 *
 * @param[in,out] pst_queue 操作対象のFixedQueueのアドレス。
 * @param[in] p_inputElement 格納する要素のアドレス。
 *                           初期化時に指定したelementSizeバイト以上を
 *                           読み出せる領域を指定する。
 *
 * @return 処理結果。キューが満杯の場合はPRIMITIVE_API_FULLを返す。
 */
PrimitiveApiResult FixedQueue_Push(
    FixedQueue *pst_queue,
    const void *p_inputElement);

/**
 * @brief キューの先頭要素を取り出す。
 *
 * @param[in,out] pst_queue 操作対象のFixedQueueのアドレス。
 * @param[out] p_outputElement 取り出した要素を書き込む領域のアドレス。
 *                             初期化時に指定したelementSizeバイト以上を
 *                             書き込める領域を指定する。
 *                             キューが空の場合およびエラー時は変更しない。
 *
 * @return 処理結果。キューが空の場合はPRIMITIVE_API_EMPTYを返す。
 */
PrimitiveApiResult FixedQueue_Pop(
    FixedQueue *pst_queue,
    void *p_outputElement);

/**
 * @brief キューの先頭要素を取り出さずに参照する。
 *
 * @param[in,out] pst_queue 操作対象のFixedQueueのアドレス。
 *                          内部不整合時はFAULTEDへ変更する。
 * @param[out] p_outputElement 先頭要素を書き込む領域のアドレス。
 *                             初期化時に指定したelementSizeバイト以上を
 *                             書き込める領域を指定する。
 *                             キューが空の場合およびエラー時は変更しない。
 *
 * @return 処理結果。キューが空の場合はPRIMITIVE_API_EMPTYを返す。
 */
PrimitiveApiResult FixedQueue_Peek(
    FixedQueue *pst_queue,
    void *p_outputElement);

/**
 * @brief キューに現在格納されている要素数を取得する。
 *
 * @param[in,out] pst_queue 操作対象のFixedQueueのアドレス。
 *                          内部不整合時はFAULTEDへ変更する。
 * @param[out] pu4_count 現在の要素数を書き込むuint32_t変数のアドレス。
 *                       エラー時は変更しない。
 *
 * @return 処理結果。
 */
PrimitiveApiResult FixedQueue_GetCount(
    FixedQueue *pst_queue,
    uint32_t *pu4_count);

/**
 * @brief FixedQueueの使用状況および統計情報を取得する。
 *
 * @param[in,out] pst_queue 操作対象のFixedQueueのアドレス。
 *                          内部不整合時はFAULTEDへ変更する。
 * @param[out] pst_stats 統計情報を書き込むFixedQueueStatsのアドレス。
 *                       エラー時は変更しない。
 *
 * @return 処理結果。
 */
PrimitiveApiResult FixedQueue_GetStats(
    FixedQueue *pst_queue,
    FixedQueueStats *pst_stats);

#endif /* FIXED_QUEUE_H */
