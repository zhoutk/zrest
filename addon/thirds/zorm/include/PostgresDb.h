#pragma once

#include "Idb.h"
#include "Utils.h"
#include "GlobalConstants.h"
#include <algorithm>
#include <cstring>
#include <libpq-fe.h>
#include "pg_type_d.h"

namespace ZORM {

	using std::string;

	namespace Postgres {
		vector<string> QUERY_EXTRA_KEYS;
		vector<string> QUERY_UNEQ_OPERS;

		class ZORM_API PostgresDb : public Idb
		{
		private:
			PGconn * GetConnection(string& err)
			{
				size_t index = (rand() % maxConn) + 1;
				if (index > pool.size())
				{
					PGconn *pqsql = PQconnectdb(connString.c_str());
					if (PQstatus(pqsql) == CONNECTION_OK)
					{
						pool.push_back(pqsql);
						return pqsql;
					}
					else
					{
						err = string(PQerrorMessage(pqsql));
						std::cout << "Error message : " << err;
						return nullptr;
					}
				}
				else
				{
					return pool.at(index - 1);
				}
			}

			void init(){
				QUERY_EXTRA_KEYS = DbUtils::MakeVector("ins,lks,ors");

				QUERY_UNEQ_OPERS.push_back(">,");
				QUERY_UNEQ_OPERS.push_back(">=,");
				QUERY_UNEQ_OPERS.push_back("<,");
				QUERY_UNEQ_OPERS.push_back("<=,");
				QUERY_UNEQ_OPERS.push_back("<>,");
				QUERY_UNEQ_OPERS.push_back("=,");

				connString = "dbname=" + dbname + " user=" + dbuser + " password=" + dbpwd + " hostaddr=" + dbhost + " port=" + DbUtils::IntTransToString(dbport);
			}

		public:

			PostgresDb(string dbhost, string dbuser, string dbpwd, string dbname, int dbport = 5432, Json options = Json()) :
				dbhost(dbhost), dbuser(dbuser), dbpwd(dbpwd), dbname(dbname), dbport(dbport), maxConn(2), DbLogClose(false), queryByParameter(false)
			{
				init();

				if(!options["db_conn"].isError() && options["db_conn"].toInt() > 2)
					maxConn = options["db_conn"].toInt();
				if(!options["db_char"].isError())
					charsetName = options["db_char"].toString();
				if (!options["DbLogClose"].isError())
					DbLogClose = options["DbLogClose"].toBool();
				if(!options["parameterized"].isError())
					queryByParameter = options["parameterized"].toBool();
			}

			Json create(string tablename, Json& params) override
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
							vs.append("$").append(DbUtils::IntTransToString(i+1));
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

			Json update(string tablename, Json& params) override
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
						int index = 1;
						Json idJson;
						for (size_t i = 0; i < len; i++) {
							string k = allKeys[i];
							bool vIsString = params[k].isString();
							string v = params[k].toString();
							!queryByParameter && vIsString &&escapeString(v);
							if (k.compare("id") == 0) {
								conditionLen++;
								if(queryByParameter){
									//where.append(" ? ");
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
									fields.append(" $").append(DbUtils::IntTransToString(index++));
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
						if(queryByParameter){
							where.append(" $").append(DbUtils::IntTransToString(index++));
							values.concat(idJson);
						}
						execSql.append(fields).append(where);
						return ExecNoneQuerySql(execSql, values);
					}
				}
				else {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
			}

			Json remove(string tablename, Json& params) override
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
						execSql.append(" $1 ");
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

			Json querySql(string sql, Json params = Json(), Json values = Json(JsonType::Array), vector<string> fields = vector<string>()) override
			{
				bool parameterized = sql.find("$") != sql.npos;
				Json rs = genSql(sql, values, params, fields, 2, parameterized);
				if(rs["status"].toInt() == 200)
					return ExecQuerySql(sql, fields, values);
				else
					return rs;
			}

			Json execSql(string sql, Json params = Json(), Json values = Json(JsonType::Array)) override
			{
				bool parameterized = sql.find("$") != sql.npos;
				Json rs = genSql(sql, values, params, std::vector<string>(), 3, parameterized);
				if(rs["status"].toInt() == 200)
					return ExecNoneQuerySql(sql, values);
				else
					return rs;
			}

			Json insertBatch(string tablename, Json& elements, string constraint) override
			{
				string sql = "insert into ";
				vector<string> restrain = DbUtils::MakeVector(constraint);
				if (elements.size() < 2) {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
				else {
					Json values = Json(JsonType::Array);
					string keyStr = " ( ";
					string updateStr = "";
					keyStr.append(DbUtils::GetVectorJoinStr(elements[0].getAllKeys())).append(" ) values ");
					int index = 1;
					for (size_t i = 0; i < elements.size(); i++) {
						vector<string> keys = elements[i].getAllKeys();
						string valueStr = " ( ";
						for (size_t j = 0; j < keys.size(); j++) {
							if (i == 0) {
								vector<string>::iterator iter = find(restrain.begin(), restrain.end(), keys[j]);
								if (iter == restrain.end())
									updateStr.append(keys[j]).append(" = excluded.").append(keys[j]).append(",");
							}
							bool vIsString = elements[i][keys[j]].isString();
							string v = elements[i][keys[j]].toString();
							!queryByParameter && vIsString && escapeString(v);
							if(queryByParameter){
								valueStr.append("$").append(DbUtils::IntTransToString(index++));
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
						valueStr.append(" )");
						if (i < elements.size() - 1) {
							valueStr.append(",");
						}
						keyStr.append(valueStr);
					}
					if (updateStr.length() == 0) {
						sql.append(tablename).append(keyStr);
					}
					else
					{
						updateStr = updateStr.substr(0, updateStr.length() - 1);
						sql.append(tablename).append(keyStr).append(" on conflict (").append(constraint).append(") do update set ").append(updateStr);
					}
					return ExecNoneQuerySql(sql, values);
				}
			}

			Json transGo(Json& sqls, bool isAsync = false) override
			{
				if (sqls.size() < 2) {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
				else {
					bool isExecSuccess = true;
					string errmsg = "Running transaction error: ";
					string err = "";
					PGconn* pq = GetConnection(err);
					if (pq == nullptr)
						return DbUtils::MakeJsonObject(STDBCONNECTERR, err);
					sqls.push_front(Json({{"text", "BEGIN TRANSACTION;"}}));
					sqls.push_back(Json({{"text", "END TRANSACTION;"}}));
					for (size_t i = 0; i < sqls.size(); i++) {
						string sql = sqls[i]["text"].toString();
						Json values = sqls[i]["values"].isError() ? Json(JsonType::Array) : sqls[i]["values"];
						if(!ExecSqlForTransGo(pq, sql, values, &err)){
							ExecSqlForTransGo(pq, "ROLLBACK;");
							return DbUtils::MakeJsonObject(STDBOPERATEERR, err);
						}
					}
					!DbLogClose && std::cout << "Transaction Success: run " << sqls.size() - 2 << " sqls." << std::endl;
					return DbUtils::MakeJsonObject(STSUCCESS, "Transaction success."); 
				}
			}

			~PostgresDb()
			{
				while (pool.size())
				{
					PQfinish(pool.back());
					pool.pop_back();
				}
			}

		private:
			int getParameterizedIndex(std::string_view sql){
				int index = 0, cur = 0;
				do{
					cur = sql.find("$", cur);
					++index;
				}while(cur++ != sql.npos);
				return index;
			}

			Json genSql(string& querySql, Json& values, Json& params, vector<string> fields = vector<string>(), int queryType = 1, bool parameterized = false)
			{
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
					int index = getParameterizedIndex(tablename);
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
											whereExtra.append("$").append(DbUtils::IntTransToString(index++));
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
										whereExtra.append("CAST(").append(ele.at(j)).append(" as TEXT) ");
										string curIndexStr = string("$").append(DbUtils::IntTransToString(index++));
										string eqStr = parameterized ? 
													   (k.compare("lks") == 0 ? string(" like ").append(curIndexStr) : string(" = ").append(curIndexStr)) : 
													   (k.compare("lks") == 0 ? " like '" : " = '");
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
										where.append(k).append(vls.at(0)).append(" $").append(DbUtils::IntTransToString(index++)).append(" ");
										values.addSubitem(vls.at(1));
									}else
										where.append(k).append(vls.at(0)).append("'").append(vls.at(1)).append("'");
								}
								else if (vls.size() == 4) {
									if(parameterized){
										where.append(k).append(vls.at(0)).append(" $").append(DbUtils::IntTransToString(index++)).append(" ").append("and ");
										where.append(k).append(vls.at(2)).append(" $").append(DbUtils::IntTransToString(index++)).append(" ");
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
									where.append("CAST(").append(k).append(" as TEXT) ").append(" like ").append(" $").append(DbUtils::IntTransToString(index++)).append(" ");
									values.addSubitem(v.insert(0, "%").append("%"));
								}
								else
									where.append(k).append(" like '%").append(v).append("%'");
								
							}
							else {
								if(parameterized){
									where.append(k).append(" =").append(" $").append(DbUtils::IntTransToString(index++)).append(" ");
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
								extra.append("sum(").append(ele.at(i)).append(") as ").append(ele.at(i + 1)).append(" ");
							}
						}
					}
					if (!count.empty()) {
						vector<string> ele = DbUtils::MakeVector(count);
						if (ele.empty() || ele.size() % 2 == 1)
							return DbUtils::MakeJsonObject(STPARAMERR, "count is wrong.");
						else {
							for (size_t i = 0; i < ele.size(); i += 2) {
								extra.append("count(").append(ele.at(i)).append(") as ").append(ele.at(i + 1)).append(" ");
							}
						}
					}

					if (queryType == 1) {
						if(extra.find("count(") != extra.npos || extra.find("sum(") != extra.npos)
							fieldsJoinStr = "";
						querySql.append("select ").append(fieldsJoinStr).append(extra).append(" from ").append(tablename);
						if (where.length() > 0){
							querySql.append(" where ").append(where);
						}
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
						querySql.append(" limit ").append(DbUtils::IntTransToString(size)).append(" OFFSET ").append(DbUtils::IntTransToString(page * size));
					}
					return DbUtils::MakeJsonObject(STSUCCESS);
				}
				else {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
			}

			Json ExecQuerySql(string aQuery, vector<string> fields, Json& values) {
				Json rs = DbUtils::MakeJsonObject(STSUCCESS);
				string err = "";
				PGconn *pq = GetConnection(err);
				if (pq == nullptr)
					return DbUtils::MakeJsonObject(STDBCONNECTERR, err);
				int vLen = values.size();
				std::vector<char *> dataInputs;
				if(vLen > 0){
					dataInputs.resize(vLen);
					for (int i = 0; i < vLen; i++) {
						string ele = values[i].toString();
						int eleLen = ele.length() + 1;
						dataInputs[i] = new char[eleLen];
						memset(dataInputs[i], 0, eleLen);
						memcpy(dataInputs[i], ele.c_str(), eleLen);
					}
				}
				PGresult *res = PQexecParams(pq, aQuery.c_str(), vLen, nullptr, vLen > 0 ? dataInputs.data() : nullptr, nullptr, nullptr,0);
				for (auto el : dataInputs)
					delete[] el;
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				if (PQresultStatus(res) == PGRES_TUPLES_OK) {
					int coLen = PQnfields(res);
					vector<Json> arr;
					for (int i = 0; i < PQntuples(res); i++) {
						Json al;
						for (int j = 0; j < coLen; j++)
						{
							auto rsType = PQftype(res, j);
							switch (rsType)
							{
							case INT2OID:
							case INT4OID:
							case INT8OID:
							case NUMERICOID:
								al.addSubitem(PQfname(res, j), atof(PQgetvalue(res, i, j)));
								break;

							default:
								al.addSubitem(PQfname(res, j), PQgetvalue(res, i, j));
								break;
							}
						}
						arr.push_back(al);
					}
					if (arr.empty())
						rs.extend(DbUtils::MakeJsonObject(STQUERYEMPTY));
					rs.addSubitem("data", arr);
					PQclear(res);
					return rs;
				}else{
					std::cout << PQerrorMessage(pq) << std::endl;
					PQclear(res);
					return DbUtils::MakeJsonObject(STDBOPERATEERR, PQerrorMessage(pq));
				}
			}

			Json ExecNoneQuerySql(string aQuery, Json values = Json(JsonType::Array)) {
				Json rs = DbUtils::MakeJsonObject(STSUCCESS);
				string err = "";
				PGconn *pq = GetConnection(err);
				if (pq == nullptr)
					return DbUtils::MakeJsonObject(STDBCONNECTERR, err);
				int vLen = values.size();
				std::vector<char *> dataInputs;
				if(vLen > 0){
					dataInputs.resize(vLen);
					for (int i = 0; i < vLen; i++) {
						string ele = values[i].toString();
						int eleLen = ele.length() + 1;
						dataInputs[i] = new char[eleLen];
						memset(dataInputs[i], 0, eleLen);
						memcpy(dataInputs[i], ele.c_str(), eleLen);
					}
				}
				PGresult *res = PQexecParams(pq, aQuery.c_str(), vLen, nullptr, vLen > 0 ? dataInputs.data() : nullptr, nullptr, nullptr,0);
				for (auto el : dataInputs)
					delete[] el;
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				if (PQresultStatus(res) != PGRES_COMMAND_OK) {
					std::cout << PQerrorMessage(pq) << std::endl;
					rs = DbUtils::MakeJsonObject(STDBOPERATEERR, PQerrorMessage(pq));
				}
				PQclear(res);
				return rs;
			}

			bool escapeString(string& pStr)
			{
				// string err = "";
				// pStr = GetConnection(err)->esc(pStr);
				return true;
			}

			std::string getEscapeString(string& pStr)
			{
				string err = "";
				return err;//GetConnection(err)->esc(pStr);
			}

			bool ExecSqlForTransGo(PGconn *pq, string aQuery, Json values = Json(JsonType::Array), string* out = nullptr) {
				int vLen = values.size();
				std::vector<char *> dataInputs;
				if(vLen > 0){
					dataInputs.resize(vLen);
					for (int i = 0; i < vLen; i++) {
						string ele = values[i].toString();
						int eleLen = ele.length() + 1;
						dataInputs[i] = new char[eleLen];
						memset(dataInputs[i], 0, eleLen);
						memcpy(dataInputs[i], ele.c_str(), eleLen);
					}
				}
				PGresult *res = PQexecParams(pq, aQuery.c_str(), vLen, nullptr, vLen > 0 ? dataInputs.data() : nullptr, nullptr, nullptr,0);
				for (auto el : dataInputs)
					delete[] el;
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				if (PQresultStatus(res) != PGRES_COMMAND_OK) {
					string err = PQerrorMessage(pq);
					std::cout << err << std::endl;
					if(out)
						*out = err;
					return false;
				}
				PQclear(res);
				return true;
			}

		private:
			std::vector<PGconn*> pool;
			int maxConn;
			string dbhost;
			string dbuser;
			string dbpwd;
			string dbname;
			int dbport;
			string charsetName;
			bool DbLogClose;
			bool queryByParameter;
			string connString;
		};
	}

}