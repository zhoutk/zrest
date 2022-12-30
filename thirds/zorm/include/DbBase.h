#pragma once
#include "Idb.h"
#include "Sqlit3Db.h"
#include "MysqlDb.h"
#include "PostgresDb.h"
#include <algorithm>

namespace ZORM
{

	class ZORM_API DbBase : public Idb
	{
	public:
		DbBase(string dbType, Json options = Json()) {
			transform(dbType.begin(), dbType.end(), dbType.begin(), ::tolower);
			bool DbLogClose = options["DbLogClose"].toBool();
			if (dbType.compare("sqlite3") == 0)
				db = new Sqlit3::Sqlit3Db(options["connString"].toString(), DbLogClose, options["parameterized"].toBool());
			else if(dbType.compare("mysql") == 0){
				string dbhost = options.getAndRemove("db_host").toString();
				string dbuser = options.getAndRemove("db_user").toString();
				string dbpwd = options.getAndRemove("db_pass").toString();
				string dbname = options.getAndRemove("db_name").toString();
				int dbport = options.getAndRemove("db_port").toInt();

				db = new Mysql::MysqlDb(dbhost, dbuser, dbpwd, dbname, dbport, options);
			}
			else if(dbType.compare("postgres") == 0){
				string dbhost = options.getAndRemove("db_host").toString();
				string dbuser = options.getAndRemove("db_user").toString();
				string dbpwd = options.getAndRemove("db_pass").toString();
				string dbname = options.getAndRemove("db_name").toString();
				int dbport = options.getAndRemove("db_port").toInt();

				db = new Postgres::PostgresDb(dbhost, dbuser, dbpwd, dbname, dbport, options);
			}
			else {
				throw "Db Type error or not be supported. ";
			}
		};
		~DbBase() {
			if (db) {
				delete db;
			}
		};

		Json select(string tablename, Json& params, vector<string> fields = vector<string>(), Json values = Json(JsonType::Array)) {
			return values.isArray() ? db->select(tablename, params, fields) : DbUtils::MakeJsonObject(STPARAMERR);
		};

		Json create(string tablename, Json& params) {
			return db->create(tablename, params);
		};

		Json update(string tablename, Json& params) {
			return db->update(tablename, params);
		};

		Json remove(string tablename, Json& params) {
			return db->remove(tablename, params);
		};

		Json querySql(string sql, Json params = Json(), Json values = Json(JsonType::Array), vector<string> fields = vector<string>()) {
			return params.isObject() && values.isArray() ? db->querySql(sql, params, values, fields) : DbUtils::MakeJsonObject(STPARAMERR);
		}

		Json execSql(string sql, Json params = Json(), Json values = Json(JsonType::Array)) {
			return params.isObject() && values.isArray() ? db->execSql(sql, params, values) : DbUtils::MakeJsonObject(STPARAMERR);
		}

		Json insertBatch(string tablename, Json& elements, string constraint = "id") {
			return elements.isArray() ? db->insertBatch(tablename, elements, constraint) : DbUtils::MakeJsonObject(STPARAMERR);
		}

		Json transGo(Json& sqls, bool isAsync = false) {
			return sqls.isArray() ? db->transGo(sqls) : DbUtils::MakeJsonObject(STPARAMERR);
		}

	private:
		Idb * db;
	};

}
