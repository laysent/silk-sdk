#ifndef SKP_SILK_API_H
#define SKP_SILK_API_H

#include <node_api.h>
#include "SKP_Silk_typedef.h"

void Decode(
  napi_env env,
  char* bitInFileName,
  char* speechOutFileName,
  SKP_int32 quiet,
  SKP_float loss_prob,
  SKP_int32 API_Fs_Hz
);

void Encode(
  napi_env env,
  char* speechInFileName,
  char* bitOutFileName,
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

#endif
