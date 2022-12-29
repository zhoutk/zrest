#pragma once
#include <assert.h>
#include "Idb.h"
#include "sqlite3.h"
#include "Utils.h"
#include "GlobalConstants.h"
#include <regex>
#include <algorithm>

namespace ZORM {

	using std::string;
	using std::vector;

	namespace Sqlit3 {

	#define SQLITECPP_ASSERT(expression, message)   assert(expression && message)

		const int   OPEN_READONLY = SQLITE_OPEN_READONLY;
		const int   OPEN_READWRITE = SQLITE_OPEN_READWRITE;
		const int   OPEN_CREATE = SQLITE_OPEN_CREATE;
		const int   OPEN_URI = SQLITE_OPEN_URI;

		const int   OK = SQLITE_OK;

		class ZORM_API Sqlit3Db : public Idb
		{
		public:
			struct Deleter
			{
				void operator()(sqlite3* apSQLite) {
					const int ret = sqlite3_close(apSQLite);
					(void)ret;
					SQLITECPP_ASSERT(SQLITE_OK == ret, "database is locked");
				};
			};

		private:
			std::unique_ptr<sqlite3, Deleter> mSQLitePtr;
			std::string mFilename;
			vector<string> QUERY_EXTRA_KEYS;
			vector<string> QUERY_UNEQ_OPERS;

		public:
			Sqlit3Db(const char* apFilename, bool logFlag = false, bool parameterized = false,
				const int   aFlags = OPEN_READWRITE | OPEN_CREATE,
				const int   aBusyTimeoutMs = 0,
				const char* apVfs = nullptr) : mFilename(apFilename)
			{
				QUERY_EXTRA_KEYS = DbUtils::MakeVector("ins,lks,ors");

				QUERY_UNEQ_OPERS.push_back(">,");
				QUERY_UNEQ_OPERS.push_back(">=,");
				QUERY_UNEQ_OPERS.push_back("<,");
				QUERY_UNEQ_OPERS.push_back("<=,");
				QUERY_UNEQ_OPERS.push_back("<>,");
				QUERY_UNEQ_OPERS.push_back("=,");

				sqlite3* handle;
				const int ret = sqlite3_open_v2(apFilename, &handle, aFlags, apVfs);
				mSQLitePtr.reset(handle);
				if (SQLITE_OK != ret)
				{
					string errmsg = "DB Error, code: ";
					errmsg.append(DbUtils::IntTransToString(ret)).append("; message: ");
					errmsg.append(sqlite3_errmsg(getHandle()));
					throw errmsg;
				}
				if (aBusyTimeoutMs > 0)
				{
					const int ret = sqlite3_busy_timeout(getHandle(), aBusyTimeoutMs);
					if (OK != ret)
					{
						string errmsg = "DB Error, code: ";
						errmsg.append(DbUtils::IntTransToString(ret)).append("; message: ");
						errmsg.append(sqlite3_errmsg(getHandle()));
						throw errmsg;
					}
				}
				DbLogClose = logFlag;
				queryByParameter = parameterized;
			};

			Sqlit3Db(const std::string& aFilename, bool logFlag = false, bool parameterized = false,
				const int          aFlags = OPEN_READWRITE | OPEN_CREATE,
				const int          aBusyTimeoutMs = 0,
				const std::string& aVfs = "") {
				new (this)Sqlit3Db(aFilename.c_str(), logFlag, parameterized, aFlags, aBusyTimeoutMs, aVfs.empty() ? nullptr : aVfs.c_str());
			};

			Json create(string tablename, Json& params)
			{
				if (!params.isError()) {
					Json values(JsonType::Array);
					string execSql = "insert into ";
					execSql.append(tablename).append(" ");

					vector<string> allKeys = params.getAllKeys();
					size_t len = allKeys.size();
					string fields = "", vs = "";
					for (size_t i = 0; i < len; i++) {
						string k = allKeys[i];
						fields.append(k);
						bool vIsString = params[k].isString();
						string v = params[k].toString();
						!queryByParameter && vIsString &&escapeString(v);
						if(queryByParameter){
							vs.append("?");
							vIsString ? values.addSubitem(v) : values.addSubitem(params[k].toDouble());
						}else{
							if (vIsString)
								vs.append("'").append(v).append("'");
							else
								vs.append(v);
						}
						
						if (i < len - 1) {
							fields.append(",");
							vs.append(",");
						}
					}
					execSql.append("(").append(fields).append(") values (").append(vs).append(")");
					return ExecNoneQuerySql(execSql, values);
				}
				else {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
			}

			Json update(string tablename, Json& params)
			{
				if (!params.isError()) {
					Json values(JsonType::Array);
					string execSql = "update ";
					execSql.append(tablename).append(" set ");

					vector<string> allKeys = params.getAllKeys();

					vector<string>::iterator iter = find(allKeys.begin(), allKeys.end(), "id");
					if (iter == allKeys.end()) {
						return DbUtils::MakeJsonObject(STPARAMERR);
					}
					else {
						size_t len = allKeys.size();
						size_t conditionLen = len - 2;
						string fields = "", where = " where id = ";
						Json idJson;
						for (size_t i = 0; i < len; i++) {
							string k = allKeys[i];
							bool vIsString = params[k].isString();
							string v = params[k].toString();
							!queryByParameter && vIsString &&escapeString(v);
							if (k.compare("id") == 0) {
								conditionLen++;
								if(queryByParameter){
									where.append(" ? ");
									idJson = params[k];
								}else{
									if (vIsString)
										where.append("'").append(v).append("'");
									else
										where.append(v);
								}
							}
							else {
								fields.append(k).append(" = ");
								if (queryByParameter)
								{
									fields.append(" ? ");
									vIsString ? values.addSubitem(v) : values.addSubitem(params[k].toDouble());
								}
								else
								{
									if (vIsString)
										fields.append("'").append(v).append("'");
									else
										fields.append(v);
								}
								if (i < conditionLen) {
									fields.append(",");
								}
							}
						}
						values.concat(idJson);
						execSql.append(fields).append(where);
						return ExecNoneQuerySql(execSql, values);
					}
				}
				else {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
			}

			Json remove(string tablename, Json& params)
			{
				if (!params.isError()) {
					Json values(JsonType::Array);
					string execSql = "delete from ";
					execSql.append(tablename).append(" where id = ");

					string k = "id";
					bool vIsString = params[k].isString();
					string v = params[k].toString();
					!queryByParameter && vIsString &&escapeString(v);
					if(queryByParameter){
						execSql.append(" ? ");
						vIsString ? values.addSubitem(v) : values.addSubitem(params[k].toDouble());
					}else{
						if (vIsString)
							execSql.append("'").append(v).append("'");
						else
							execSql.append(v);
					}
					return ExecNoneQuerySql(execSql, values);
				}
				else {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
			}

			Json select(string tablename, Json &params, vector<string> fields = vector<string>(), Json values = Json(JsonType::Array)) override
			{
				Json rs = genSql(tablename, values, params, fields, 1, queryByParameter);
				if(rs["status"].toInt() == 200)
					return ExecQuerySql(tablename, fields, values);
				else
					return rs;
			}

			Json execSql(string sql, Json params = Json(), Json values = Json(JsonType::Array)) override
			{
				bool parameterized = sql.find("?") != sql.npos;
				Json rs = genSql(sql, values, params, std::vector<string>(), 3, parameterized);
				if(rs["status"].toInt() == 200)
					return ExecNoneQuerySql(sql, values);
				else
					return rs;
			}

			Json querySql(string sql, Json params = Json(), Json values = Json(JsonType::Array), vector<string> fields = vector<string>()) override
			{
				bool parameterized = sql.find("?") != sql.npos;
				Json rs = genSql(sql, values, params, fields, 2, parameterized);
				if(rs["status"].toInt() == 200)
					return ExecQuerySql(sql, fields, values);
				else
					return rs;
			}

			Json insertBatch(string tablename, Json& elements, string constraint) {
				string sql = "insert into ";
				if (elements.size() < 2) {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
				else {
					Json values = Json(JsonType::Array);
					string keyStr = " (";
					keyStr.append(DbUtils::GetVectorJoinStr(elements[0].getAllKeys())).append(" ) ");
					for (int i = 0; i < elements.size(); i++) {
						vector<string> keys = elements[i].getAllKeys();
						string valueStr = " select ";
						for (int j = 0; j < keys.size(); j++) {
							bool vIsString = elements[i][keys[j]].isString();
							string v = elements[i][keys[j]].toString();
							!queryByParameter && vIsString && escapeString(v);
							if(queryByParameter){
								valueStr.append("?");
								values.addSubitem(v);
							}else{
								if(vIsString)
									valueStr.append("'").append(v).append("'");
								else
									valueStr.append(v);
							}
							if (j < keys.size() - 1) {
								valueStr.append(",");
							}
						}
						if (i < elements.size() - 1) {
							valueStr.append(" union all ");
						}
						keyStr.append(valueStr);
					}
					sql.append(tablename).append(keyStr);
					return ExecNoneQuerySql(sql, values);
				}
			}

			Json transGo(Json& sqls, bool isAsync = false) {
				if (sqls.size() < 2) {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
				else {
					char* zErrMsg = 0;
					string errmsg = "Running transaction error: ";
					bool isExecSuccess = true;
					//sqlite3_exec(getHandle(),"PRAGMA synchronous = FULL; ",0,0,0);
					sqlite3_exec(getHandle(),"PRAGMA synchronous = OFF; ",0,0,0);
					sqlite3_exec(getHandle(), "begin;", 0, 0, &zErrMsg);
					for (size_t i = 0; i < sqls.size(); i++) {
						string sql = sqls[i]["text"].toString();
						Json values = sqls[i]["values"].isError() ? Json(JsonType::Array) : sqls[i]["values"];
						isExecSuccess = ExecSqlForTransGo(sql, values, &errmsg);
						if (!isExecSuccess)
							break;
					}
					if (isExecSuccess)
					{
						sqlite3_exec(getHandle(), "commit;", 0, 0, 0);
						!DbLogClose && std::cout << "Transaction Success: run " << sqls.size() << " sqls." << std::endl;
						return DbUtils::MakeJsonObject(STSUCCESS, "Transaction success, run " + DbUtils::IntTransToString(sqls.size()) + " sqls.");
					}
					else
					{
						sqlite3_exec(getHandle(), "rollback;", 0, 0, 0);
						return DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg);
					}
				}
			}

			sqlite3* getHandle()
			{
				return mSQLitePtr.get();
			}

		private:
			Json genSql(string& querySql, Json& values, Json& params, vector<string> fields = vector<string>(), int queryType = 1, bool parameterized = false) {
				if (!params.isError()) {
					string tablename = querySql;
					querySql = "";
					string where = "";
					const string AndJoinStr = " and ";
					string fieldsJoinStr = "*";

					if (!fields.empty()) {
						fieldsJoinStr = DbUtils::GetVectorJoinStr(fields);
					}

					string fuzzy = params.getAndRemove("fuzzy").toString();
					string sort = params.getAndRemove("sort").toString();
					int page = atoi(params.getAndRemove("page").toString().c_str());
					int size = atoi(params.getAndRemove("size").toString().c_str());
					string sum = params.getAndRemove("sum").toString();
					string count = params.getAndRemove("count").toString();
					string group = params.getAndRemove("group").toString();

					vector<string> allKeys = params.getAllKeys();
					size_t len = allKeys.size();
					for (size_t i = 0; i < len; i++) {
						string k = allKeys[i];
						bool vIsString = params[k].isString();
						string v = params[k].toString();
						!parameterized && vIsString && escapeString(v);
						if (where.length() > 0) {
							where.append(AndJoinStr);
						}

						if (DbUtils::FindStringFromVector(QUERY_EXTRA_KEYS, k)) {   // process key
							string whereExtra = "";
							vector<string> ele = DbUtils::MakeVector(params[k].toString());
							if (ele.size() < 2 || ((k.compare("ors") == 0 || k.compare("lks") == 0) && ele.size() % 2 == 1)) {
								return DbUtils::MakeJsonObject(STPARAMERR, k + " is wrong.");
							}
							else {
								if (k.compare("ins") == 0) {
									string c = ele.at(0);
									vector<string>(ele.begin() + 1, ele.end()).swap(ele);
									if(parameterized){
										whereExtra.append(c).append(" in (");
										int eleLen = ele.size();
										for (int i = 0; i < eleLen; i++)
										{
											string el = ele[i];
											whereExtra.append("?");
											if (i < eleLen - 1)
												whereExtra.append(",");
											values.addSubitem(el);
										}
										whereExtra.append(")");
									}else
										whereExtra.append(c).append(" in ( ").append(DbUtils::GetVectorJoinStr(ele)).append(" )");
								}
								else if (k.compare("lks") == 0 || k.compare("ors") == 0) {
									whereExtra.append(" ( ");
									for (size_t j = 0; j < ele.size(); j += 2) {
										if (j > 0) {
											whereExtra.append(" or ");
										}
										whereExtra.append(ele.at(j)).append(" ");
										string eqStr = parameterized ? (k.compare("lks") == 0 ? " like ?" : " = ?") : (k.compare("lks") == 0 ? " like '" : " = '");
										string vsStr = ele.at(j + 1);
										if (k.compare("lks") == 0) {
											vsStr.insert(0, "%");
											vsStr.append("%");
										}
										whereExtra.append(eqStr);
										if(parameterized)
											values.addSubitem(vsStr);
										else{
											vsStr.append("'");
											whereExtra.append(vsStr);
										}
									}
									whereExtra.append(" ) ");
								}
							}
							where.append(whereExtra);
						}
						else {				// process value
							if (DbUtils::FindStartsStringFromVector(QUERY_UNEQ_OPERS, v)) {
								vector<string> vls = DbUtils::MakeVector(v);
								if (vls.size() == 2) {
									if(parameterized){
										where.append(k).append(vls.at(0)).append(" ? ");
										values.addSubitem(vls.at(1));
									}else
										where.append(k).append(vls.at(0)).append("'").append(vls.at(1)).append("'");
								}
								else if (vls.size() == 4) {
									if(parameterized){
										where.append(k).append(vls.at(0)).append(" ? ").append("and ");
										where.append(k).append(vls.at(2)).append("? ");
										values.addSubitem(vls.at(1));
										values.addSubitem(vls.at(3));
									}else{
										where.append(k).append(vls.at(0)).append("'").append(vls.at(1)).append("' and ");
										where.append(k).append(vls.at(2)).append("'").append(vls.at(3)).append("'");
									}
								}
								else {
									return DbUtils::MakeJsonObject(STPARAMERR, "not equal value is wrong.");
								}
							}
							else if (fuzzy == "1") {
								if(parameterized){
									where.append(k).append(" like ? ");
									values.addSubitem(v.insert(0, "%").append("%"));
								}
								else
									where.append(k).append(" like '%").append(v).append("%'");
								
							}
							else {
								if(parameterized){
									where.append(k).append(" = ? ");
									vIsString ? values.addSubitem(v) : values.addSubitem(params[k].toDouble());
								}else{
									if (vIsString)
										where.append(k).append(" = '").append(v).append("'");
									else
										where.append(k).append(" = ").append(v);
								}
							}
						}
					}

					string extra = "";
					if (!sum.empty()) {
						vector<string> ele = DbUtils::MakeVector(sum);
						if (ele.empty() || ele.size() % 2 == 1)
							return DbUtils::MakeJsonObject(STPARAMERR, "sum is wrong.");
						else {
							for (size_t i = 0; i < ele.size(); i += 2) {
								extra.append(",sum(").append(ele.at(i)).append(") as ").append(ele.at(i + 1)).append(" ");
							}
						}
					}
					if (!count.empty()) {
						vector<string> ele = DbUtils::MakeVector(count);
						if (ele.empty() || ele.size() % 2 == 1)
							return DbUtils::MakeJsonObject(STPARAMERR, "count is wrong.");
						else {
							for (size_t i = 0; i < ele.size(); i += 2) {
								extra.append(",count(").append(ele.at(i)).append(") as ").append(ele.at(i + 1)).append(" ");
							}
						}
					}

					if (queryType == 1) {
						querySql.append("select ").append(fieldsJoinStr).append(extra).append(" from ").append(tablename);
						if (where.length() > 0)
							querySql.append(" where ").append(where);
					}
					else {
						querySql.append(tablename);
						if (queryType == 2 && !fields.empty()) {
							size_t starIndex = querySql.find('*');
							if (starIndex < 10) {
								querySql.replace(starIndex, 1, fieldsJoinStr.c_str());
							}
						}
						if (where.length() > 0) {
							size_t whereIndex = querySql.find("where");
							if (whereIndex == querySql.npos) {
								querySql.append(" where ").append(where);
							}
							else {
								querySql.append(" and ").append(where);
							}
						}
					}

					if (!group.empty()) {
						querySql.append(" group by ").append(group);
					}

					if (!sort.empty()) {
						querySql.append(" order by ").append(sort);
					}

					if (page > 0) {
						page--;
						querySql.append(" limit ").append(DbUtils::IntTransToString(page * size)).append(",").append(DbUtils::IntTransToString(size));
					}
					return DbUtils::MakeJsonObject(STSUCCESS);
				}
				else {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
			};

			Json ExecQuerySql(string aQuery, vector<string> fields, Json& values) {
				Json rs = DbUtils::MakeJsonObject(STSUCCESS);
				sqlite3_stmt* stmt = NULL;
				sqlite3* handle = getHandle();
				const int ret = sqlite3_prepare_v2(handle, aQuery.c_str(), static_cast<int>(aQuery.size()), &stmt, NULL);
				if (SQLITE_OK != ret)
				{
					string errmsg = sqlite3_errmsg(getHandle());
					rs.extend(DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg));
				}
				else {
					int insertPot = aQuery.find("where");
					if (insertPot == aQuery.npos) {
						insertPot = aQuery.find("limit");
						if (insertPot == aQuery.npos) {
							insertPot = aQuery.length();
						}
					}
					string aQueryLimit0 = aQuery.substr(0, insertPot).append(" limit 1");
					char** pRes = NULL;
					int nRow = 0, nCol = 0;
					char* pErr = NULL;
					sqlite3_get_table(handle, aQueryLimit0.c_str(), &pRes, &nRow, &nCol, &pErr);
					for (int j = 0; j < nCol; j++)
					{
						string fs = *(pRes + j);
						if (find(fields.begin(), fields.end(), fs) == fields.end()) {
							fields.push_back(fs);
						}
					}
					if (pErr != NULL)
					{
						sqlite3_free(pErr);
					}
					sqlite3_free_table(pRes);

					for(int i = 0; i < values.size(); i++){
						string ele = values[i].toString();
						sqlite3_bind_text(stmt, i + 1, ele.c_str(), ele.length(), SQLITE_TRANSIENT);
					}

					vector<Json> arr;
					while (sqlite3_step(stmt) == SQLITE_ROW) {
						Json al;
						for (int j = 0; j < nCol; j++)
						{
							string k = fields.at(j);
							int nType = sqlite3_column_type(stmt, j);
							if (nType == 1) {					//SQLITE_INTEGER
								al.addSubitem(k, sqlite3_column_int(stmt, j));
							}
							else if (nType == 2) {				//SQLITE_FLOAT
								al.addSubitem(k, sqlite3_column_double(stmt, j));
							}
							else if (nType == 3) {				//SQLITE_TEXT
								al.addSubitem(k, (char*)sqlite3_column_text(stmt, j));
							}
							//else if (nType == 4) {				//SQLITE_BLOB

							//}
							//else if (nType == 5) {				//SQLITE_NULL

							//}
							else{
								al.addSubitem(k, "");
							}
						}
						arr.push_back(al);
					}
					if (arr.empty())
						rs.extend(DbUtils::MakeJsonObject(STQUERYEMPTY));
					rs.addSubitem("data", arr);
				}
				sqlite3_finalize(stmt);
				//if(!DbLogClose)
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				return rs;
			}

			Json ExecNoneQuerySql(string aQuery, Json values = Json(JsonType::Array)) {
				int stepRet = SQLITE_OK;
				std::string errStr = "Error Code : ";
				Json rs = DbUtils::MakeJsonObject(STSUCCESS);
				sqlite3_stmt* stmt = NULL;
				sqlite3* handle = getHandle();
				const int ret = sqlite3_prepare_v2(handle, aQuery.c_str(), static_cast<int>(aQuery.size()), &stmt, NULL);
				if (SQLITE_OK != ret)
				{
					string errmsg = sqlite3_errmsg(getHandle());
					rs.extend(DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg));
				}
				else {
					for(int i = 0; i < values.size(); i++){
						string ele = values[i].toString();
						sqlite3_bind_text(stmt, i + 1, ele.c_str(), ele.length(), SQLITE_TRANSIENT);
					}
					stepRet = sqlite3_step(stmt);
				}
				sqlite3_finalize(stmt);
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				return stepRet == SQLITE_DONE ? rs : DbUtils::MakeJsonObject(STDBOPERATEERR, errStr.append(DbUtils::IntTransToString(stepRet)));
			}

			bool ExecSqlForTransGo(string aQuery, Json values = Json(JsonType::Array), string* out = nullptr) {
				int stepRet = SQLITE_OK;
				sqlite3_stmt* stmt = NULL;
				sqlite3* handle = getHandle();
				const int ret = sqlite3_prepare_v2(handle, aQuery.c_str(), static_cast<int>(aQuery.size()), &stmt, NULL);
				if (SQLITE_OK == ret) {
					for(int i = 0; i < values.size(); i++){
						string ele = values[i].toString();
						sqlite3_bind_text(stmt, i + 1, ele.c_str(), ele.length(), SQLITE_TRANSIENT);
					}
					stepRet = sqlite3_step(stmt);
				}else{
					if(out)
						*out += sqlite3_errmsg(getHandle());
				}
				sqlite3_finalize(stmt);
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				return SQLITE_OK == ret && stepRet == SQLITE_DONE ? true : false;
			}

			bool escapeString(string& pStr)
			{
				pStr = std::regex_replace(pStr, std::regex("'"), "''");
				return true;
			}

			bool DbLogClose;
			bool queryByParameter;
		};
	}

}