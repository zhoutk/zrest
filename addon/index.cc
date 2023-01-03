#include "index.h"

typedef std::uint64_t hash_t;
 
constexpr hash_t prime = 0x100000001B3ull;
constexpr hash_t basis = 0xCBF29CE484222325ull;

constexpr hash_t hash_(char const* str, hash_t last_value = basis)
{
	return *str ? hash_(str+1, (*str ^ last_value) * prime) : last_value;
}

constexpr unsigned long long operator "" _hash(char const* p, size_t)
{
	return hash_(p);
}

Napi::FunctionReference Zorm::constructor;

Napi::Object Zorm::Init(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);

    Napi::Function func =
        DefineClass(env,
                    "Zorm",
                    {
                        InstanceMethod("select", &Zorm::select),
                        InstanceMethod("create", &Zorm::create),
                        InstanceMethod("update", &Zorm::update),
                        InstanceMethod("remove", &Zorm::remove),
                        InstanceMethod("querySql", &Zorm::querySql),
                        InstanceMethod("execSql", &Zorm::execSql),
                        InstanceMethod("insertBatch", &Zorm::insertBatch),
                        InstanceMethod("transGo", &Zorm::transGo),
                    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();

    exports.Set("Zorm", func);
    return exports;
}

Zorm::Zorm(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Zorm>(info), db(nullptr)
{
    int len = info.Length();
    Napi::Env env = info.Env();
    if (len < 2 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
    }
    std::string dbDialect = info[0].As<Napi::String>().ToString();
    std::string opStr = info[1].As<Napi::String>();
    ZJSON::Json options(opStr);
    switch (hash_(dbDialect.c_str()))
    {
    case "postgres"_hash:
        db = new ZORM::DbBase("postgres", options);
        break;
    case "mysql"_hash:
        db = new ZORM::DbBase("mysql", options);
        break;
    case "sqlite3"_hash: {
        db = new ZORM::DbBase("sqlite3", options);
        db->execSql("DROP TABLE IF EXISTS \"table_for_test\";");
        db->execSql("CREATE TABLE \"table_for_test\" (\
                        \"id\" char(64) NOT NULL,\
                        \"name\" TEXT DEFAULT '',\
                        \"age\" integer DEFAULT 0,\
                        \"score\" real DEFAULT 0.0,\
                        PRIMARY KEY (\"id\"));");
        ZORM::Json cObj{
            {"id", "a1b2c3d4"},
            {"name", "Kevin 凯文"},
            {"age", 18},
            {"score", 99.99}
        };
        db->create("table_for_test", cObj);
        break;
    }
    default:
        break;
    }
}

Napi::Value Zorm::select(const Napi::CallbackInfo& info)
{
    int len = info.Length();
    Napi::Env env = info.Env();
    if (len < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
    }
    std::string tableName = info[0].As<Napi::String>().ToString().Utf8Value();

    ZJSON::Json params;
    if(len >= 2){
        params.extend(ZJSON::Json(info[1].As<Napi::String>().ToString().Utf8Value()));
    }
    std::string fieldStr;
    if(len >= 3){
        fieldStr = info[2].As<Napi::String>().ToString().Utf8Value();
    }

    ZJSON::Json rs = db->select(tableName, params, ZORM::DbUtils::MakeVector(fieldStr));
    return Napi::String::New(info.Env(), rs.toString());
}

Napi::Value Zorm::querySql(const Napi::CallbackInfo& info)
{
    int len = info.Length();
    Napi::Env env = info.Env();
    if (len < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
    }
    std::string sql = info[0].As<Napi::String>().ToString().Utf8Value();

    ZJSON::Json params;
    if(len >= 2){
        params.extend(ZJSON::Json(info[1].As<Napi::String>().ToString().Utf8Value()));
    }

    ZJSON::Json values(ZJSON::JsonType::Array);
    if(len >= 3){
        values.extend(ZJSON::Json(info[2].As<Napi::String>().ToString().Utf8Value()));
    }

    std::string fieldStr;
    if(len >= 4){
        fieldStr = info[3].As<Napi::String>().ToString().Utf8Value();
    }

    ZJSON::Json rs = db->querySql(sql, params, values, ZORM::DbUtils::MakeVector(fieldStr));
    return Napi::String::New(info.Env(), rs.toString());
}

Napi::Value Zorm::create(const Napi::CallbackInfo& info)
{
    int len = info.Length();
    Napi::Env env = info.Env();
    if (len < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
    }
    std::string tableName = info[0].As<Napi::String>().ToString().Utf8Value();

    ZJSON::Json params;
    if(len >= 2){
        //std::string tmp = info[1].As<Napi::String>().ToString().Utf8Value();
        params.extend(ZJSON::Json(info[1].As<Napi::String>().ToString().Utf8Value()));
    }

    ZJSON::Json rs = db->create(tableName, params);
    return Napi::String::New(info.Env(), rs.toString());
}

Napi::Value Zorm::update(const Napi::CallbackInfo& info)
{
    int len = info.Length();
    Napi::Env env = info.Env();
    if (len < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
    }
    std::string tableName = info[0].As<Napi::String>().ToString().Utf8Value();

    ZJSON::Json params;
    if(len >= 2){
        params.extend(ZJSON::Json(info[1].As<Napi::String>().ToString().Utf8Value()));
    }

    ZJSON::Json rs = db->update(tableName, params);
    return Napi::String::New(info.Env(), rs.toString());
}

Napi::Value Zorm::remove(const Napi::CallbackInfo& info)
{
    int len = info.Length();
    Napi::Env env = info.Env();
    if (len < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
    }
    std::string tableName = info[0].As<Napi::String>().ToString().Utf8Value();

    ZJSON::Json params;
    if(len >= 2){
        params.extend(ZJSON::Json(info[1].As<Napi::String>().ToString().Utf8Value()));
    }

    ZJSON::Json rs = db->remove(tableName, params);
    return Napi::String::New(info.Env(), rs.toString());
}

Napi::Value Zorm::execSql(const Napi::CallbackInfo& info)
{
    int len = info.Length();
    Napi::Env env = info.Env();
    if (len < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
    }
    std::string sql = info[0].As<Napi::String>().ToString().Utf8Value();

    ZJSON::Json params;
    if(len >= 2){
        params.extend(ZJSON::Json(info[1].As<Napi::String>().ToString().Utf8Value()));
    }

    ZJSON::Json values(ZJSON::JsonType::Array);
    if(len >= 3){
        values.extend(ZJSON::Json(info[2].As<Napi::String>().ToString().Utf8Value()));
    }

    ZJSON::Json rs = db->execSql(sql, params, values);
    return Napi::String::New(info.Env(), rs.toString());
}

Napi::Value Zorm::insertBatch(const Napi::CallbackInfo& info)
{
    int len = info.Length();
    Napi::Env env = info.Env();
    if (len < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
    }
    std::string tableName = info[0].As<Napi::String>().ToString().Utf8Value();

    ZJSON::Json jsonArr(ZJSON::JsonType::Array);
    if(len >= 2){
        jsonArr.concat(ZJSON::Json(info[1].As<Napi::String>().ToString().Utf8Value()));
    }

    ZJSON::Json rs = db->insertBatch(tableName, jsonArr);
    return Napi::String::New(info.Env(), rs.toString());
}

Napi::Value Zorm::transGo(const Napi::CallbackInfo& info)
{
    int len = info.Length();
    Napi::Env env = info.Env();
    if (len < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
    }
    ZJSON::Json sqls = ZJSON::Json(info[0].As<Napi::String>().ToString().Utf8Value());

    ZJSON::Json rs = db->transGo(sqls);
    return Napi::String::New(info.Env(), rs.toString());
}