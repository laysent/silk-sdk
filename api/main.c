#include <node_api.h>
#include <assert.h>
#include <string.h>
#include "SKP_Silk_API.h"
#include "SKP_Silk_define.h"

#define DECLARE_NAPI_METHOD(name, func)                          \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

#define READ_PROP(key_str, input, variable, converter) \
  status = napi_create_string_utf8(env, key_str, strlen(key_str), &key); \
  assert(status == napi_ok); \
  status = napi_has_property(env, input, key, &hasProperty); \
  assert(status == napi_ok); \
  if (hasProperty) { \
    status = napi_get_property(env, input, key, &value); \
    assert(status == napi_ok); \
    status = converter(env, value, &variable); \
    assert(status == napi_ok); \
  }

napi_value Decode_Entry(napi_env env, napi_callback_info info) {
  napi_status status;
  void* buffer;

  size_t bufferLength = 0;
  size_t argc = 2;
  napi_value args[2];
  status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if( argc < 1 ) {
    napi_throw_error(env, NULL, "Wrong number of arguments");
    return NULL;
  }

  /* get arguments */
  status = napi_get_buffer_info(env, args[0], &buffer, &bufferLength);
  assert(status == napi_ok);

  bool hasProperty = false;
  napi_value value;
  napi_value key;

  /* default settings */
  bool quiet = true;
  int API_Fs_Hz = 24000;
  double loss_prob = 0.0f;

  if (argc > 1) {
    READ_PROP("quiet", args[1], quiet, napi_get_value_bool);
    READ_PROP("lossProb", args[1], loss_prob, napi_get_value_double);
    READ_PROP("fsHz", args[1], API_Fs_Hz, napi_get_value_int32);
  }

  return Decode(env, buffer, bufferLength, quiet ? 1 : 0, (float)loss_prob, API_Fs_Hz);
}

napi_value Encode_Entry(napi_env env, napi_callback_info info) {
  napi_status status;
  void* buffer;

  size_t bufferLength = 0;
  size_t argc = 2;
  napi_value args[2];
  status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if (argc < 1) {
    napi_throw_error(env, NULL, "Wrong number of arguments");
    return NULL;
  }

  /* get arguments */
  status = napi_get_buffer_info(env, args[0], &buffer, &bufferLength);
  assert(status == napi_ok);

  bool hasProperty = false;
  napi_value value;
  napi_value key;

  /* default settings */
  int API_Fs_Hz = 24000;
  int max_internal_fs_Hz = 0;
  int packetSize_ms = 20;
  int targetRate_bps = 25000;
  int packetLoss_perc = 0;
#if LOW_COMPLEXITY_ONLY
  int complexity_mode = 0;
#else
  int complexity_mode = 2;
#endif
  bool INBandFEC_enabled = false;
  bool DTX_enabled = false;
  bool quiet = true;
  bool tencent = false;
  bool tencent_amr = false;

  if (argc > 1) {
    READ_PROP("fsHz", args[1], API_Fs_Hz, napi_get_value_int32);
    READ_PROP("fsMaxInternal", args[1], max_internal_fs_Hz, napi_get_value_int32);
    READ_PROP("packetLength", args[1], packetSize_ms, napi_get_value_int32);
    READ_PROP("rate", args[1], targetRate_bps, napi_get_value_int32);
    READ_PROP("loss", args[1], packetLoss_perc, napi_get_value_int32);
    READ_PROP("complexity", args[1], complexity_mode, napi_get_value_int32);
    READ_PROP("inbandFEC", args[1], INBandFEC_enabled, napi_get_value_bool);
    READ_PROP("dtx", args[1], DTX_enabled, napi_get_value_bool);
    READ_PROP("quiet", args[1], quiet, napi_get_value_bool);
    READ_PROP("tencent", args[1], tencent, napi_get_value_bool);
    READ_PROP("tencentAmr", args[1], tencent_amr, napi_get_value_bool);
  }

  return Encode(
    env,
    buffer,
    bufferLength,
    API_Fs_Hz,
    max_internal_fs_Hz,
    packetSize_ms,
    targetRate_bps,
    packetLoss_perc,
    complexity_mode,
    INBandFEC_enabled ? 1 : 0,
    DTX_enabled ? 1 : 0,
    quiet ? 1 : 0,
    tencent || tencent_amr ? 1 : 0,
    tencent_amr ? 1 : 0
  );
}

napi_value Compare_Entry(napi_env env, napi_callback_info info) {
  void *refBuffer, *testBuffer;
  size_t refBufferLength, testBufferLength;
  napi_status status;

  size_t argc = 3;
  napi_value args[3];
  status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if (argc < 2) {
    napi_throw_error(env, NULL, "Wrong number of arguments");
    return NULL;
  }

  /* get arguments */
  status = napi_get_buffer_info(env, args[0], &refBuffer, &refBufferLength);
  assert(status == napi_ok);

  status = napi_get_buffer_info(env, args[1], &testBuffer, &testBufferLength);
  assert(status == napi_ok);

  bool hasProperty = false;
  napi_value value;
  napi_value key;

  bool useDiff = false;
  bool quiet = true;
  int Fs_Hz = 24000;
  if (argc > 2) {
    READ_PROP("diff", args[2], useDiff, napi_get_value_bool);
    READ_PROP("fs", args[2], Fs_Hz, napi_get_value_int32);
    READ_PROP("quiet", args[2], quiet, napi_get_value_bool);
  }

  bool result = Compare(env, refBuffer, refBufferLength, testBuffer, testBufferLength, useDiff ? 1 : 0, Fs_Hz, quiet ? 1 : 0);
  napi_value ret;

  status = napi_get_boolean(env, result, &ret);
  assert(status == napi_ok);

  return ret;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor desc[] = {
    DECLARE_NAPI_METHOD("decode", Decode_Entry),
    DECLARE_NAPI_METHOD("encode", Encode_Entry),
    DECLARE_NAPI_METHOD("compare", Compare_Entry),
  };
  status = napi_define_properties(env, exports, 3, desc);
  assert(status == napi_ok);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
