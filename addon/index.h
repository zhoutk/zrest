#ifndef INDEX_H
#define INDEX_H

#include <napi.h>
#include "Idb.h"
#include "DbBase.h"

class Zorm : public Napi::ObjectWrap<Zorm>{
public:
    //导出函数
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static Napi::FunctionReference constructor;
    
    Zorm(const Napi::CallbackInfo& info);

    Napi::Value select(const Napi::CallbackInfo& info);
    Napi::Value create(const Napi::CallbackInfo& info);
    Napi::Value update(const Napi::CallbackInfo& info);
    Napi::Value remove(const Napi::CallbackInfo& info);
    Napi::Value querySql(const Napi::CallbackInfo& info);
    Napi::Value execSql(const Napi::CallbackInfo& info);
    Napi::Value insertBatch(const Napi::CallbackInfo& info);
    Napi::Value transGo(const Napi::CallbackInfo& info);

private:
    ZORM::Idb* db;
};

#endif

