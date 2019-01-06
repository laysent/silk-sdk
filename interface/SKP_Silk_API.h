#ifndef SKP_SILK_API_H
#define SKP_SILK_API_H

#include <node_api.h>
#include "SKP_Silk_typedef.h"

napi_value Decode(
  napi_env env,
  void* inStream,
  size_t total,
  SKP_int32 quiet,
  SKP_float loss_prob,
  SKP_int32 API_Fs_Hz
);

napi_value Encode(
  napi_env env,
  void* inStream,
  size_t total,
  SKP_int32 API_Fs_Hz,
  SKP_int32 max_internal_fs_Hz,
  SKP_int32 packetSize_ms,
  SKP_int32 targetRate_bps,
  SKP_int32 packetLoss_perc,
  SKP_int32 complexity_mode,
  SKP_int32 INBandFEC_enabled,
  SKP_int32 DTX_enabled,
  SKP_int32 quiet,
  SKP_int32 tencent,
  SKP_int32 tencent_amr
);

int Compare(
  napi_env env,
  void* refInStream,
  size_t refTotal,
  void* testInStream,
  size_t testTotal,
  SKP_int32 diff,
  SKP_int32 fs,
  SKP_int32 quiet
);

#endif
