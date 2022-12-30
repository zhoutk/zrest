#pragma once

#include "Idb.h"
#include "Utils.h"
#include "GlobalConstants.h"
#include "mysql.h"
#include <algorithm>
#include <cstring>

namespace ZORM {

	using std::string;

	namespace Mysql {

		class ZORM_API MysqlDb : public Idb {

		public:
			const vector<string> QUERY_EXTRA_KEYS { "ins", "lks", "ors"};
			const vector<string> QUERY_UNEQ_OPERS { ">,", ">=,", "<,", "<=,", "<>,", "=,"};
			 
		private:
			MYSQL* GetConnection(string& err) {
				//srand((unsigned int)(time(nullptr)));
				size_t index = (rand() % maxConn) + 1;
				if (index > pool.size()) {
					MYSQL* pmysql;
					pmysql = mysql_init((MYSQL*)nullptr);
					if (pmysql != nullptr)
					{
					 	!charsetName.empty() && mysql_options(pmysql, MYSQL_SET_CHARSET_NAME, charsetName.c_str());
						if (mysql_real_connect(pmysql, dbhost.c_str(), dbuser.c_str(), dbpwd.c_str(), dbname.c_str(), dbport, nullptr, 0))
						{
							pool.push_back(pmysql);
							return pmysql;
						}else{
							err = (char*)mysql_error(pmysql);
							std::cout << "Error message : " << mysql_error(pmysql);
						}

					}
					return nullptr;
				}
				else {
					return pool.at(index - 1);
				}
			}

		public:

			MysqlDb(string dbhost, string dbuser, string dbpwd, string dbname, int dbport = 3306, Json options = Json()) :
				dbhost(dbhost), dbuser(dbuser), dbpwd(dbpwd), dbname(dbname), dbport(dbport), maxConn(2), DbLogClose(false), queryByParameter(false)
			{
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
					return queryByParameter ? ExecNoneQuerySql(execSql, values) : ExecNoneQuerySql(execSql);
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
						return queryByParameter ? ExecNoneQuerySql(execSql, values) : ExecNoneQuerySql(execSql);
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
						execSql.append(" ? ");
						vIsString ? values.addSubitem(v) : values.addSubitem(params[k].toDouble());
					}else{
						if (vIsString)
							execSql.append("'").append(v).append("'");
						else
							execSql.append(v);
					}
					return queryByParameter ? ExecNoneQuerySql(execSql, values) : ExecNoneQuerySql(execSql);
				}
				else {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
			}


			Json select(string tablename, Json &params, vector<string> fields = vector<string>(), Json values = Json(JsonType::Array)) override
			{
				Json rs = genSql(tablename, values, params, fields, 1, queryByParameter);
				if(rs["status"].toInt() == 200)
					return queryByParameter ? ExecQuerySql(tablename, fields, values) : ExecQuerySql(tablename, fields);
				else
					return rs;
			}

			Json querySql(string sql, Json params = Json(), Json values = Json(JsonType::Array), vector<string> fields = vector<string>()) override
			{
				bool parameterized = sql.find("?") != sql.npos;
				Json rs = genSql(sql, values, params, fields, 2, parameterized);
				if(rs["status"].toInt() == 200)
					return parameterized ? ExecQuerySql(sql, fields, values) : ExecQuerySql(sql, fields);
				else
					return rs;
			}


			Json execSql(string sql, Json params = Json(), Json values = Json(JsonType::Array)) override
			{
				bool parameterized = sql.find("?") != sql.npos;
				Json rs = genSql(sql, values, params, std::vector<string>(), 3, parameterized);
				if(rs["status"].toInt() == 200)
					return parameterized ? ExecNoneQuerySql(sql, values) : ExecNoneQuerySql(sql);
				else
					return rs;
			}


			Json insertBatch(string tablename, Json& elements, string constraint) override
			{
				string sql = "insert into ";
				if (elements.size() < 2) {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
				else {
					Json values = Json(JsonType::Array);
					string keyStr = " ( ";
					string updateStr = "";
					keyStr.append(DbUtils::GetVectorJoinStr(elements[0].getAllKeys())).append(" ) values ");
					for (int i = 0; i < elements.size(); i++) {
						vector<string> keys = elements[i].getAllKeys();
						string valueStr = " ( ";
						for (int j = 0; j < keys.size(); j++) {
							if(i == 0)
								updateStr.append(keys[j]).append(" = values(").append(keys[j]).append(")");
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
								if (i == 0)
									updateStr.append(",");
							}
						}
						valueStr.append(" )");
						if (i < elements.size() - 1) {
							valueStr.append(",");
						}
						keyStr.append(valueStr);
					}
					sql.append(tablename).append(keyStr).append(" on duplicate key update ").append(updateStr);
					return queryByParameter ? ExecNoneQuerySql(sql,values) : ExecNoneQuerySql(sql);
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
					MYSQL* mysql = GetConnection(err);
					if (mysql == nullptr)
						return DbUtils::MakeJsonObject(STDBCONNECTERR, err);

					mysql_query(mysql, "begin;");
					for (size_t i = 0; i < sqls.size(); i++) {
						string sql = sqls[i]["text"].toString();
						Json values = sqls[i]["values"].isError() ? Json(JsonType::Array) : sqls[i]["values"];
						isExecSuccess = ExecSqlForTransGo(sql, values, &errmsg);
						if (!isExecSuccess)
							break;
					}
					if (isExecSuccess)
					{
						mysql_query(mysql, "commit;");
						!DbLogClose && std::cout << "Transaction Success: run " << sqls.size() << " sqls." << std::endl;
						return DbUtils::MakeJsonObject(STSUCCESS, "Transaction success.");
					}
					else
					{
						mysql_query(mysql, "rollback;");
						return DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg);
					}
				}
			}

			~MysqlDb()
			{
				while (pool.size())
				{
					mysql_close(pool.back());
					pool.pop_back();
				}
			}

		private:
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
								extra.append(",cast(sum(").append(ele.at(i)).append(") as double) as ").append(ele.at(i + 1)).append(" ");
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
						querySql.append(" limit ").append(DbUtils::IntTransToString(page * size)).append(",").append(DbUtils::IntTransToString(size));
					}
					return DbUtils::MakeJsonObject(STSUCCESS);
				}
				else {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
			}

			Json ExecQuerySql(string aQuery, vector<string> fields)
			{
				Json rs = DbUtils::MakeJsonObject(STSUCCESS);
				string err = "";
				MYSQL *mysql = GetConnection(err);
				if (mysql == nullptr)
					return DbUtils::MakeJsonObject(STDBCONNECTERR, err);
				if (mysql_query(mysql, aQuery.c_str()))
				{
					string errmsg = "";
					errmsg.append((char *)mysql_error(mysql)).append(". error code: ");
					errmsg.append(DbUtils::IntTransToString(mysql_errno(mysql)));
					return rs.extend(DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg));
				}
				else
				{
					MYSQL_RES *result = mysql_use_result(mysql);
					if (result != NULL)
					{
						MYSQL_ROW row;
						int num_fields = mysql_num_fields(result);
						MYSQL_FIELD *fields = mysql_fetch_fields(result);
						vector<Json> arr;
						while ((row = mysql_fetch_row(result)) && row != NULL)
						{
							Json al;
							for (int i = 0; i < num_fields; ++i)
							{
								if(IS_NUM(fields[i].type))
									al.addSubitem(fields[i].name, atof(row[i]));
								else
									al.addSubitem(fields[i].name, row[i]);
							}
							arr.push_back(al);
						}
						if (arr.empty())
							rs.extend(DbUtils::MakeJsonObject(STQUERYEMPTY));
						rs.addSubitem("data", arr);
					}
					mysql_free_result(result);
				}
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				return rs;
			}

			Json ExecQuerySql(string aQuery, vector<string> fields, Json& values) {
				Json rs = DbUtils::MakeJsonObject(STSUCCESS);
				string err = "";
				MYSQL* mysql = GetConnection(err);
				if (mysql == nullptr)
					return DbUtils::MakeJsonObject(STDBCONNECTERR, err);
				MYSQL_STMT* stmt = mysql_stmt_init(mysql);
				if (mysql_stmt_prepare(stmt, aQuery.c_str(), aQuery.length()))
				{
					string errmsg = "";
					errmsg.append((char*)mysql_error(mysql)).append(". error code: ");
					errmsg.append(DbUtils::IntTransToString(mysql_errno(mysql)));
					return rs.extend(DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg));
				}
				else
				{
					const int vLen = values.size();
					std::vector<char *> dataInputs;
					if (vLen > 0)
					{
						MYSQL_BIND *bind = new MYSQL_BIND[vLen];
						std::memset(bind, 0, sizeof(MYSQL_BIND) * vLen);
						dataInputs.resize(vLen);
						for (int i = 0; i < vLen; i++)
						{
							string ele = values[i].toString();
							int eleLen = ele.length() + 1;
							dataInputs[i] = new char[eleLen];
							memset(dataInputs[i], 0, eleLen);
							memcpy(dataInputs[i], ele.c_str(), eleLen);
							bind[i].buffer_type = MYSQL_TYPE_STRING;
							bind[i].buffer = (void *)dataInputs[i];
							bind[i].buffer_length = eleLen - 1;
						}
						if (mysql_stmt_bind_param(stmt, bind))
						{
							string errmsg = "";
							errmsg.append((char *)mysql_error(mysql)).append(". error code: ");
							errmsg.append(DbUtils::IntTransToString(mysql_errno(mysql)));
							return rs.extend(DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg));
						}
						delete [] bind;
					}

					MYSQL_RES* prepare_meta_result = mysql_stmt_result_metadata(stmt);
					MYSQL_FIELD* fields;
					if (prepare_meta_result != nullptr)
					{
						int ret = 1;
						mysql_stmt_attr_set(stmt, STMT_ATTR_UPDATE_MAX_LENGTH, (void *)&ret);
						if (mysql_stmt_execute(stmt))
						{
							string errmsg = "";
							errmsg.append((char *)mysql_error(mysql)).append(". error code: ");
							errmsg.append(DbUtils::IntTransToString(mysql_errno(mysql)));
							return rs.extend(DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg));
						}
						ret = mysql_stmt_store_result(stmt);
						int num_fields = mysql_num_fields(prepare_meta_result);
						fields = mysql_fetch_fields(prepare_meta_result);
						MYSQL_BIND *ps = new MYSQL_BIND[num_fields];
						std::memset(ps, 0, sizeof(MYSQL_BIND) * num_fields);
						std::vector<char *> dataOuts(num_fields);
						char* is_null = new char[num_fields];
						memset(is_null, 0, sizeof(char) * num_fields);
						for (int i = 0; i < num_fields; ++i)
						{
							auto p = allocate_buffer_for_field(fields[i]);
							dataOuts[i] = new char[p.size];
							memset(dataOuts[i], 0, p.size);
							ps[i].buffer_type = p.type;
							ps[i].buffer = (void *)dataOuts[i];
							ps[i].buffer_length = p.size;
							ps[i].is_null = &is_null[i];
						}
						ret = mysql_stmt_bind_result(stmt, ps);
						vector<Json> arr;
						while (mysql_stmt_fetch(stmt) != MYSQL_NO_DATA)
						{
							Json al;
							for (int i = 0; i < num_fields; ++i)
							{
								if (is_null[i])
									al.addSubitem(fields[i].name, nullptr);
								else if (fields[i].type == MYSQL_TYPE_LONG || fields[i].type == MYSQL_TYPE_LONGLONG)  //count
									al.addSubitem(fields[i].name, (long)*((int *)dataOuts[i]));
								else if (fields[i].type == MYSQL_TYPE_DOUBLE || fields[i].type == MYSQL_TYPE_NEWDECIMAL) //sum
									al.addSubitem(fields[i].name, *((double *)dataOuts[i]));
								else
									al.addSubitem(fields[i].name, dataOuts[i]);
							}
							arr.push_back(al);
						}
						if (arr.empty())
							rs.extend(DbUtils::MakeJsonObject(STQUERYEMPTY));
						rs.addSubitem("data", arr);
						delete [] ps;
						delete [] is_null;
						for(auto el : dataOuts)
							delete [] el;
					}
					for (auto el : dataInputs)
						delete[] el;
				}
				mysql_stmt_close(stmt);
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				return rs;
			}

			Json ExecNoneQuerySql(string aQuery) {
				Json rs = DbUtils::MakeJsonObject(STSUCCESS);
				string err = "";
				MYSQL* mysql = GetConnection(err);
				if (mysql == nullptr)
					return DbUtils::MakeJsonObject(STDBCONNECTERR, err);

				if (mysql_query(mysql, aQuery.c_str()))
				{
					string errmsg = "";
					errmsg.append((char*)mysql_error(mysql)).append(". error code: ");
					errmsg.append(DbUtils::IntTransToString(mysql_errno(mysql)));
					return rs.extend(DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg));
				}
				else {
					int affected = (int)mysql_affected_rows(mysql);
					rs.addSubitem("affected", affected);
				}
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				return rs;
			}

			Json ExecNoneQuerySql(string aQuery, Json values) {
				Json rs = DbUtils::MakeJsonObject(STSUCCESS);
				string err = "";
				MYSQL* mysql = GetConnection(err);
				if (mysql == nullptr)
					return DbUtils::MakeJsonObject(STDBCONNECTERR, err);

				MYSQL_STMT* stmt = mysql_stmt_init(mysql);
				if (mysql_stmt_prepare(stmt, aQuery.c_str(), aQuery.length()))
				{
					string errmsg = "";
					errmsg.append((char*)mysql_error(mysql)).append(". error code: ");
					errmsg.append(DbUtils::IntTransToString(mysql_errno(mysql)));
					return rs.extend(DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg));
				}
				else {
					int vLen = values.size();
					std::vector<char *> dataInputs;
					if (vLen > 0)
					{
						MYSQL_BIND *bind = new MYSQL_BIND[vLen];
						std::memset(bind, 0, sizeof(MYSQL_BIND) * vLen);
						dataInputs.resize(vLen);
						for (int i = 0; i < vLen; i++)
						{
							string ele = values[i].toString();
							int eleLen = ele.length() + 1;
							dataInputs[i] = new char[eleLen];
							memset(dataInputs[i], 0, eleLen);
							memcpy(dataInputs[i], ele.c_str(), eleLen);
							bind[i].buffer_type = MYSQL_TYPE_STRING;
							bind[i].buffer = (void *)dataInputs[i];
							bind[i].buffer_length = eleLen - 1;
						}
						if (mysql_stmt_bind_param(stmt, bind))
						{
							string errmsg = "";
							errmsg.append((char *)mysql_error(mysql)).append(". error code: ");
							errmsg.append(DbUtils::IntTransToString(mysql_errno(mysql)));
							return rs.extend(DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg));
						}
						delete [] bind;
					}
					if (mysql_stmt_execute(stmt))
					{
						string errmsg = "";
						errmsg.append((char *)mysql_error(mysql)).append(". error code: ");
						errmsg.append(DbUtils::IntTransToString(mysql_errno(mysql)));
						return rs.extend(DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg));
					}
					int affected = (int)mysql_affected_rows(mysql);
					rs.addSubitem("affected", affected);
					for (auto el : dataInputs)
						delete[] el;
				}
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				return rs;
			}

			struct st_buffer_size_type
			{
				size_t size;
				enum_field_types type;
				st_buffer_size_type(size_t s, enum_field_types t) : size(s + 1), type(t) {}
			};

			
			st_buffer_size_type allocate_buffer_for_field(const MYSQL_FIELD field)
			{
				switch (field.type)
				{
				case MYSQL_TYPE_NULL:
					return st_buffer_size_type(0, field.type);
				case MYSQL_TYPE_TINY:
					return st_buffer_size_type(1, field.type);
				case MYSQL_TYPE_SHORT:
					return st_buffer_size_type(2, field.type);
				case MYSQL_TYPE_INT24:
				case MYSQL_TYPE_LONG:
				case MYSQL_TYPE_FLOAT:
					return st_buffer_size_type(4, field.type);
				case MYSQL_TYPE_DOUBLE:
				case MYSQL_TYPE_LONGLONG:
					return st_buffer_size_type(8, field.type);
				case MYSQL_TYPE_YEAR:
					return st_buffer_size_type(2, MYSQL_TYPE_SHORT);
				case MYSQL_TYPE_TIMESTAMP:
				case MYSQL_TYPE_DATE:
				case MYSQL_TYPE_TIME:
				case MYSQL_TYPE_DATETIME:
					return st_buffer_size_type(sizeof(MYSQL_TIME), field.type);

				case MYSQL_TYPE_TINY_BLOB:
				case MYSQL_TYPE_MEDIUM_BLOB:
				case MYSQL_TYPE_LONG_BLOB:
				case MYSQL_TYPE_BLOB:
				case MYSQL_TYPE_STRING:
				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_JSON:{
					return st_buffer_size_type(field.max_length, field.type);
				}
				case MYSQL_TYPE_DECIMAL:
				case MYSQL_TYPE_NEWDECIMAL:
					return st_buffer_size_type(64, field.type);
#if A1
				case MYSQL_TYPE_TIMESTAMP:
				case MYSQL_TYPE_YEAR:
					return st_buffer_size_type(10, field.type);
#endif
#if A0
				case MYSQL_TYPE_ENUM:
				case MYSQL_TYPE_SET:
#endif
				case MYSQL_TYPE_BIT:
					return st_buffer_size_type(8, MYSQL_TYPE_BIT);
				case MYSQL_TYPE_GEOMETRY:
					return st_buffer_size_type(field.max_length, MYSQL_TYPE_BIT);
				default:
					return st_buffer_size_type(field.max_length, field.type);
				}
			};

			bool ExecSqlForTransGo(string aQuery, Json values = Json(JsonType::Array), string* out = nullptr) {
				string err = "";
				MYSQL* mysql = GetConnection(err);
				if (mysql == nullptr){
					if(out)
						*out += "can not connect the database.";
					return false;
				}

				MYSQL_STMT* stmt = mysql_stmt_init(mysql);
				if (mysql_stmt_prepare(stmt, aQuery.c_str(), aQuery.length()))
				{
					string errmsg = "";
					errmsg.append((char*)mysql_error(mysql)).append(". error code: ");
					errmsg.append(DbUtils::IntTransToString(mysql_errno(mysql)));
					if(out)
						*out += errmsg;
					return false;
				}
				else {
					int vLen = values.size();
					std::vector<char *> dataInputs;
					if (vLen > 0)
					{
						MYSQL_BIND *bind = new MYSQL_BIND[vLen];
						std::memset(bind, 0, sizeof(MYSQL_BIND) * vLen);
						dataInputs.resize(vLen);
						for (int i = 0; i < vLen; i++)
						{
							string ele = values[i].toString();
							int eleLen = ele.length() + 1;
							dataInputs[i] = new char[eleLen];
							memset(dataInputs[i], 0, eleLen);
							memcpy(dataInputs[i], ele.c_str(), eleLen);
							bind[i].buffer_type = MYSQL_TYPE_STRING;
							bind[i].buffer = (void *)dataInputs[i];
							bind[i].buffer_length = eleLen - 1;
						}
						if (mysql_stmt_bind_param(stmt, bind))
						{
							string errmsg = "";
							errmsg.append((char *)mysql_error(mysql)).append(". error code: ");
							errmsg.append(DbUtils::IntTransToString(mysql_errno(mysql)));
							if (out)
								*out += errmsg;
							return false;
						}
						delete [] bind;
					}
					if (mysql_stmt_execute(stmt))
					{
						string errmsg = "";
						errmsg.append((char *)mysql_error(mysql)).append(". error code: ");
						errmsg.append(DbUtils::IntTransToString(mysql_errno(mysql)));
						if (out)
							*out += errmsg;
						return false;
					}
					for (auto el : dataInputs)
						delete[] el;
				}
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				return true;
			}

			bool escapeString(string& pStr)
			{
				char *tStr = new char[pStr.length() * 2 + 1];
				string err = "";
				mysql_real_escape_string(GetConnection(err), tStr, pStr.c_str(), pStr.length());
				pStr = std::string(tStr);
				delete[] tStr;
				return true;
			}

		private:
			vector<MYSQL*> pool;
			int maxConn;
			string dbhost;
			string dbuser;
			string dbpwd;
			string dbname;
			int dbport;
			string charsetName;
			bool DbLogClose;
			bool queryByParameter;
		};

	}

}