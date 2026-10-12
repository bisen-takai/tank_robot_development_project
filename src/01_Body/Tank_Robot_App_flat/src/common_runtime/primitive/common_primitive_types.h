/**
 * @file common_primitive_types.h
 * @brief Common Runtime共通Primitiveで使用する共通公開型を定義する。
 */

#ifndef COMMON_PRIMITIVE_TYPES_H
#define COMMON_PRIMITIVE_TYPES_H

#include <stdint.h>

/**
 * @brief Common Runtime共通Primitive APIの処理結果。
 */
typedef enum
{
    PRIMITIVE_API_OK,
    PRIMITIVE_API_NO_CHANGE,
    PRIMITIVE_API_EMPTY,
    PRIMITIVE_API_FULL,
    PRIMITIVE_API_NOT_VALID,
    PRIMITIVE_API_SATURATED,
    PRIMITIVE_API_NOT_INITIALIZED,
    PRIMITIVE_API_INVALID_ARGUMENT,
    PRIMITIVE_API_INVALID_STATE,
    PRIMITIVE_API_GENERATION_MISMATCH,
    PRIMITIVE_API_RUNTIME_UNAVAILABLE
} PrimitiveApiResult;

/**
 * @brief 状態を保持するPrimitiveインスタンスの初期化状態。
 */
typedef enum
{
    PRIMITIVE_INSTANCE_UNINITIALIZED = 0,
    PRIMITIVE_INSTANCE_READY,
    PRIMITIVE_INSTANCE_FAULTED
} PrimitiveInstanceState;

/**
 * @brief 固定リソースの使用状況を表すスナップショット。
 */
typedef struct
{
    uint32_t capacity;                 /* 利用可能なリソースの総数。 */
    uint32_t currentUsed;              /* 現在使用中で再利用できないリソース数。 */
    uint32_t highWater;                /* MCU起動後のcurrentUsed最大値。 */
    uint32_t allocationRejectCount;    /* リソース不足により新規受付を拒否した回数。 */
} RuntimeResourceUsageSnapshot;

#endif /* COMMON_PRIMITIVE_TYPES_H */
