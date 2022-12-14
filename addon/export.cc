#include "index.h"

//导出接口
Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return Zorm::Init(env, exports);
}

NODE_API_MODULE(Zorm, InitAll)

