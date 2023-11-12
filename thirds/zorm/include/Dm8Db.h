#pragma once

#include "Idb.h"
#include "DbUtils.h"
#include "GlobalConstants.h"
#include <algorithm>
#include <iostream>
#include "DPI.h"
#include "DPIext.h"
#include "DPItypes.h"


namespace ZORM {

	using std::string;

	namespace Dm8 {

		struct Dm8Con {
			dhenv henv;
			dhcon hcon;
			dhstmt hstmt;
			Dm8Con() { henv = nullptr; hcon = nullptr; hstmt = nullptr; }
		};

		class ZORM_API Dm8Db : public Idb {

		public:
			const vector<string> QUERY_EXTRA_KEYS { "ins", "lks", "ors"};
			const vector<string> QUERY_UNEQ_OPERS { ">,", ">=,", "<,", "<=,", "<>,", "=,"};
			 
		private:
			void dpi_err_msg_print(sdint2 hndl_type, dhandle hndl, string& errOut)
			{
				sdint4 err_code;
				sdint2 msg_len;
				sdbyte err_msg[SDBYTE_MAX];
				char err[SDBYTE_MAX];

				dpi_get_diag_rec(hndl_type, hndl, 1, &err_code, err_msg, sizeof(err_msg), &msg_len);
				printf("err_msg = %s, err_code = %d\n", err_msg, err_code);
				if (hndl_type == DSQL_HANDLE_STMT)
					dpi_free_stmt(hndl);
				sprintf(err, "err_msg = %s, err_code = %d\n", err_msg, err_code);
				errOut = string(err);
			}

			Dm8Con* GetConnection(string& err) {
				size_t index = (rand() % maxConn) + 1;
				if (index > pool.size()) {
					Dm8Con* dmCon = new Dm8Con;
					DPIRETURN rt; 
					rt = dpi_alloc_env(&dmCon->henv);
					rt = dpi_alloc_con(dmCon->henv, &dmCon->hcon);
					dbhost.append(":").append(DbUtils::IntTransToString(dbport));
					rt = dpi_login(dmCon->hcon, (sdbyte*)dbhost.c_str(), (sdbyte*)dbuser.c_str(), (sdbyte*)dbpwd.c_str());
					if (!DSQL_SUCCEEDED(rt))
					{
						dpi_err_msg_print(DSQL_HANDLE_DBC, dmCon->hcon, err);
						return nullptr;
					}
					pool.push_back(dmCon);
					return dmCon;
				}
				else {
					return pool.at(index - 1);
				}
			}

		public:

			Dm8Db(string dbhost, string dbuser, string dbpwd, Json options = Json()) :
				dbhost(dbhost), dbuser(dbuser), dbpwd(dbpwd), dbname(""), dbport(5236), maxConn(1), DbLogClose(false), queryByParameter(false)
			{
				if(!options["db_conn"].isError() && options["db_conn"].toInt() > 1)
					maxConn = options["db_conn"].toInt();
				if(!options["db_char"].isError())
					charsetName = options["db_char"].toString();
				if (!options["db_name"].isError())
					dbname = options["db_name"].toString();
				if (!options["db_port"].isError())
					dbport = options["db_port"].toInt();
				if (!options["DbLogClose"].isError())
					DbLogClose = options["DbLogClose"].toBool();
				if(!options["parameterized"].isError())
					queryByParameter = options["parameterized"].toBool();
			}

			Json create(const string& tablename, const Json& params) override
			{
				if (!params.isError()) {
					Json values(JsonType::Array);
					string execSql = "insert into ";
					execSql.append("\"").append(dbname).append("\"").append(".").append("\"").append(tablename).append("\"").append(" ");

					vector<string> allKeys = params.getAllKeys();
					size_t len = allKeys.size();
					string fields = "", vs = "";
					for (size_t i = 0; i < len; i++) {
						string k = allKeys[i];
						fields.append("\"").append(k).append("\"");
						bool vIsString = params[k].isString() || params[k].isArray() || params[k].isObject();
						string v = params[k].toString();
						!queryByParameter && vIsString && escapeString(v);
						if(queryByParameter){
							vs.append("?");
							vIsString ? values.add(v) : values.add(params[k].toDouble());
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

			Json update(const string& tablename, const Json& params) override
			{
				if (!params.isError()) {
					Json values(JsonType::Array);
					string execSql = "update ";
					execSql.append("\"").append(dbname).append("\"").append(".").append("\"").append(tablename).append("\"").append(" set ");

					vector<string> allKeys = params.getAllKeys();
					vector<string>::iterator iter = find(allKeys.begin(), allKeys.end(), "id");
					if (iter == allKeys.end()) {
						return DbUtils::MakeJsonObject(STPARAMERR);
					}
					else {
						size_t len = allKeys.size();
						size_t conditionLen = len - 2;
						string fields = "", where = " where \"id\" = ";
						Json idJson;
						for (size_t i = 0; i < len; i++) {
							string k = allKeys[i];
							bool vIsString = params[k].isString() || params[k].isArray() || params[k].isObject();
							string v = params[k].toString();
							!queryByParameter && vIsString&& escapeString(v);
							if (k.compare("id") == 0) {
								conditionLen++;
								if (queryByParameter) {
									where.append(" ? ");
									idJson = params[k];
								}
								else {
									if (vIsString)
										where.append("'").append(v).append("'");
									else
										where.append(v);
								}
							}
							else {
								fields.append("\"").append(k).append("\"").append(" = ");
								if (queryByParameter)
								{
									fields.append(" ? ");
									vIsString ? values.add(v) : values.add(params[k].toDouble());
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

			Json remove(const string& tablename, const Json& params) override
			{
				if (!params.isError()) {
					Json values(JsonType::Array);
					string execSql = "delete from ";
					execSql.append("\"").append(dbname).append("\"").append(".").append("\"").append(tablename).append("\"").append(" where \"id\" = ");

					string k = "id";
					bool vIsString = params[k].isString() || params[k].isArray() || params[k].isObject();
					string v = params[k].toString();
					!queryByParameter && vIsString&& escapeString(v);
					if (queryByParameter) {
						execSql.append(" ? ");
						vIsString ? values.add(v) : values.add(params[k].toDouble());
					}
					else {
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

			Json select(const string& tbname, const Json &params, vector<string> fields = vector<string>(), Json values = Json(JsonType::Array)) override
			{
				string tablename = tbname;
				Json rs = genSql(tablename, values, params, fields, 1, queryByParameter);
				if(rs["status"].toInt() == 200)
					return queryByParameter ? ExecQuerySql(tablename, fields, values) : ExecQuerySql(tablename, fields);
				else
					return rs;
			}

			Json querySql(const string& sqlstr, Json params = Json(), Json values = Json(JsonType::Array), vector<string> fields = vector<string>()) override
			{
				string sql(sqlstr);
				bool parameterized = sql.find("?") != sql.npos;
				Json rs = genSql(sql, values, params, fields, 2, parameterized);
				if(rs["status"].toInt() == 200)
					return parameterized ? ExecQuerySql(sql, fields, values) : ExecQuerySql(sql, fields);
				else
					return rs;
			}

			Json execSql(const string& sqlstr, Json params = Json(), Json values = Json(JsonType::Array)) override
			{
				string sql(sqlstr);
				bool parameterized = sql.find("?") != sql.npos;
				Json rs = genSql(sql, values, params, std::vector<string>(), 3, parameterized);
				if(rs["status"].toInt() == 200)
					return parameterized ? ExecNoneQuerySql(sql, values) : ExecNoneQuerySql(sql);
				else
					return rs;
			}

			Json insertBatch(const string& tablename, const Json& elements, string constraint) override
			{
				string sql = "insert into ";
				if (elements.size() < 2) {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
				else {
					Json values = Json(JsonType::Array);
					string keyStr = " ( ";
					keyStr.append(DbUtils::GetVectorJoinStrArroundQuots(elements[0].getAllKeys())).append(" ) values ");
					for (int i = 0; i < elements.size(); i++) {
						vector<string> keys = elements[i].getAllKeys();
						string valueStr = " ( ";
						for (int j = 0; j < keys.size(); j++) {
							bool vIsString = elements[i][keys[j]].isString() || elements[i][keys[j]].isArray() || elements[i][keys[j]].isObject();
							string v = elements[i][keys[j]].toString();
							!queryByParameter && vIsString && escapeString(v);
							if(queryByParameter){
								valueStr.append("?");
								values.add(v);
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
					sql.append("\"").append(dbname).append("\"").append(".").append("\"").append(tablename).append("\"").append(keyStr);
					return queryByParameter ? ExecNoneQuerySql(sql,values) : ExecNoneQuerySql(sql);
				}
			}

			Json transGo(const Json& sqls, bool isAsync = false) override
			{
				if (sqls.size() < 2) {
					return DbUtils::MakeJsonObject(STPARAMERR);
				}
				else {
					bool isExecSuccess = true;
					string errmsg = "Running transaction error: ";
					string err = "";
					Dm8Con* con = GetConnection(err);
					if (con == nullptr)
						return DbUtils::MakeJsonObject(STDBCONNECTERR, err);
					DPIRETURN rt = dpi_set_con_attr(con->hcon, DSQL_ATTR_AUTOCOMMIT, 0, 0);
					if (!DSQL_SUCCEEDED(rt))
					{
						dpi_err_msg_print(DSQL_HANDLE_DBC, con->hcon, err);
					}

					for (size_t i = 0; i < sqls.size(); i++) {
						string sql = sqls[i]["text"].toString();
						Json values = sqls[i]["values"].isError() ? Json(JsonType::Array) : sqls[i]["values"];
						isExecSuccess = ExecSqlForTransGo(con, sql, values, &errmsg);
						if (!isExecSuccess)
							break;
					}
					if (isExecSuccess)
					{
						rt = dpi_commit(con->hcon);
						!DbLogClose && std::cout << "Transaction Success: run " << sqls.size() << " sqls." << std::endl;
					}
					else
					{
						rt = dpi_rollback(con->hcon);
					}
					rt = dpi_set_con_attr(con->hcon, DSQL_ATTR_AUTOCOMMIT, (dpointer)1, 0);
					if (!DSQL_SUCCEEDED(rt))
					{
						dpi_err_msg_print(DSQL_HANDLE_DBC, con->hcon, err);
					}
					return isExecSuccess ? 
						DbUtils::MakeJsonObject(STSUCCESS, "Transaction success.") : 
						DbUtils::MakeJsonObject(STDBOPERATEERR, errmsg);
				}
			}

			~Dm8Db()
			{
				while (pool.size())
				{
					Dm8Con* con = pool.back();
					dpi_logout(con->hcon);
					dpi_free_con(con->hcon);
					dpi_free_env(con->henv);
					pool.pop_back();
				}
			}

		private:
			Json genSql(string& querySql, Json& values, const Json& ps, vector<string> fields = vector<string>(), int queryType = 1, bool parameterized = false)
			{
				if (!ps.isError()) {
					Json params(ps);
					string tablename = querySql;
					querySql = "";
					string where = "";
					const string AndJoinStr = " and ";
					string fieldsJoinStr = "*";

					if (!fields.empty()) {
						fieldsJoinStr = DbUtils::GetVectorJoinStr(fields);
					}

					string fuzzy = params.take("fuzzy").toString();
					string sort = params.take("sort").toString();
					int page = atoi(params.take("page").toString().c_str());
					int size = atoi(params.take("size").toString().c_str());
					string sum = params.take("sum").toString();
					string count = params.take("count").toString();
					string group = params.take("group").toString();

					if (!count.empty() || !sum.empty())
						fieldsJoinStr = " ";

					vector<string> allKeys = params.getAllKeys();
					size_t len = allKeys.size();
					for (size_t i = 0; i < len; i++) {
						string k = allKeys[i];
						bool vIsString = params[k].isString() || params[k].isArray() || params[k].isObject();
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
										whereExtra.append("\"").append(c).append("\"").append(" in (");
										int eleLen = ele.size();
										for (int i = 0; i < eleLen; i++)
										{
											string el = ele[i];
											whereExtra.append("?");
											if (i < eleLen - 1)
												whereExtra.append(",");
											values.add(el);
										}
										whereExtra.append(")");
									}else
										whereExtra.append("\"").append(c).append("\"").append(" in ( ").append(DbUtils::GetVectorJoinStr(ele)).append(" )");
								}
								else if (k.compare("lks") == 0 || k.compare("ors") == 0) {
									whereExtra.append(" ( ");
									for (size_t j = 0; j < ele.size(); j += 2) {
										if (j > 0) {
											whereExtra.append(" or ");
										}
										whereExtra.append("\"").append(ele.at(j)).append("\"").append(" ");
										string eqStr = parameterized ? (k.compare("lks") == 0 ? " like ?" : " = ?") : (k.compare("lks") == 0 ? " like '" : " = '");
										string vsStr = ele.at(j + 1);
										if (k.compare("lks") == 0) {
											vsStr.insert(0, "%");
											vsStr.append("%");
										}
										whereExtra.append(eqStr);
										if(parameterized)
											values.add(vsStr);
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
										where.append("\"").append(k).append("\"").append(vls.at(0)).append(" ? ");
										values.add(vls.at(1));
									}else
										where.append("\"").append(k).append("\"").append(vls.at(0)).append("'").append(vls.at(1)).append("'");
								}
								else if (vls.size() == 4) {
									if(parameterized){
										where.append("\"").append(k).append("\"").append(vls.at(0)).append(" ? ").append("and ");
										where.append("\"").append(k).append("\"").append(vls.at(2)).append("? ");
										values.add(vls.at(1));
										values.add(vls.at(3));
									}else{
										where.append("\"").append(k).append("\"").append(vls.at(0)).append("'").append(vls.at(1)).append("' and ");
										where.append("\"").append(k).append("\"").append(vls.at(2)).append("'").append(vls.at(3)).append("'");
									}
								}
								else {
									return DbUtils::MakeJsonObject(STPARAMERR, "not equal value is wrong.");
								}
							}
							else if (fuzzy == "1") {
								if(parameterized){
									where.append("\"").append(k).append("\"").append(" like ? ");
									values.add(v.insert(0, "%").append("%"));
								}
								else
									where.append("\"").append(k).append("\"").append(" like '%").append(v).append("%'");
								
							}
							else {
								if(parameterized){
									where.append("\"").append(k).append("\"").append(" = ? ");
									vIsString ? values.add(v) : values.add(params[k].toDouble());
								}else{
									if (vIsString)
										where.append("\"").append(k).append("\"").append(" = '").append(v).append("'");
									else
										where.append("\"").append(k).append("\"").append(" = ").append(v);
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
								extra.append("sum(\"").append(ele.at(i)).append("\") as ").append(ele.at(i + 1)).append(" ");
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

					if (!group.empty()) {
						vector<string> gs = DbUtils::MakeVector(group);
						for (int i = 0; i < gs.size(); i++)
							gs[i] = "\"" + gs[i] + "\"";
						string gpstr = DbUtils::GetVectorJoinStr(gs);
						querySql = "select " + gpstr + (extra.empty() ? "" : ","+extra) + " from ";//.append(" group by ").append("\"").append(group).append("\"");
						querySql.append("\"").append(dbname).append("\"").append(".").append("\"").append(tablename).append("\"");
						querySql.append(" group by ").append(gpstr);
					}
					else if (queryType == 1) {
						querySql.append("select ").append(fieldsJoinStr).append(extra).append(" from ").append("\"").append(dbname).append("\"").append(".").append("\"").append(tablename).append("\"");
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

					if (!sort.empty()) {
						querySql.append(" order by ").append(sort);
					}

					if (page > 0) {
						page--;
						size = size < 1 ? 10 : size;
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
				Dm8Con* con = GetConnection(err);
				if (con == nullptr)
					return DbUtils::MakeJsonObject(STDBCONNECTERR, err);
				dpi_alloc_stmt(con->hcon, &con->hstmt);
				DPIRETURN rt = dpi_exec_direct(con->hstmt, (sdbyte*)aQuery.c_str());
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				if (!DSQL_SUCCEEDED(rt))
				{
					dpi_err_msg_print(DSQL_HANDLE_STMT, con->hstmt, err);
					return DbUtils::MakeJsonObject(STDBOPERATEERR, err);
				}
				sdint2 num_fields;
				rt = dpi_number_columns(con->hstmt, &num_fields);

				std::vector<sdbyte*> fieldNames;
				fieldNames.resize(num_fields);
				std::vector<sdint2> fieldType;
				fieldType.resize(num_fields);
				std::vector<sdint2> name_len;
				name_len.resize(num_fields);

				std::vector<slength> outPtrs;
				outPtrs.resize(num_fields);
				std::vector<char*> dataOuts;
				dataOuts.resize(num_fields);
				std::vector<double> outDoubles;
				outDoubles.resize(num_fields);
				std::vector<int> outInts;
				outInts.resize(num_fields);

				for (int i = 0; i < num_fields; ++i)
				{
					ulength col_sz;
					sdint2 dec_digits;
					sdint2 nullable;
					fieldNames[i] = new sdbyte[SDBYTE_MAX];
					memset(fieldNames[i], 0, SDBYTE_MAX);
					rt = dpi_desc_column(con->hstmt, i + 1, fieldNames[i], SDBYTE_MAX, &name_len[i],
						&fieldType[i], &col_sz, &dec_digits, &nullable);
					if (!DSQL_SUCCEEDED(rt))
					{
						dpi_err_msg_print(DSQL_HANDLE_STMT, con->hstmt, err);
						return DbUtils::MakeJsonObject(STDBOPERATEERR, err);
					}
					if (fieldType[i] == DSQL_DOUBLE) {
						rt = dpi_bind_col(con->hstmt, i + 1, DSQL_C_DOUBLE, &outDoubles[i], sizeof(double), &outPtrs[i]);
					}
					else if (fieldType[i] == DSQL_INT) {
						rt = dpi_bind_col(con->hstmt, i + 1, DSQL_C_SLONG, &outInts[i], sizeof(int), &outPtrs[i]);
					}
					else {
						dataOuts[i] = new char[SDINT2_MAX];
						memset(dataOuts[i], 0, SDINT2_MAX);
						rt = dpi_bind_col(con->hstmt, i + 1, DSQL_C_NCHAR, dataOuts[i], SDINT2_MAX, &outPtrs[i]);
					}

				}
				
				Json arr(JsonType::Array);
				ulength row_num;
				while (dpi_fetch(con->hstmt, &row_num) != DSQL_NO_DATA)
				{
					Json al;
					for (int i = 0; i < num_fields; ++i)
					{
						if (fieldType[i] == DSQL_DOUBLE) {
							al.add(string((char*)(fieldNames[i])), outDoubles[i]);
						}
						else if (fieldType[i] == DSQL_INT) {
							al.add(string((char*)(fieldNames[i])), outInts[i]);
						}
						else {
							string tmp(dataOuts[i]);
							al.add(string((char*)(fieldNames[i])), tmp.erase(tmp.find_last_not_of(" ") + 1));
						}
					}
					arr.push_back(al);
				}
				if (arr.isEmpty())
					rs.extend(DbUtils::MakeJsonObject(STQUERYEMPTY));
				rs.add("data", arr);
				dpi_free_stmt(con->hstmt);
				for (auto el : dataOuts)
					delete[] el;
				for (auto el : fieldNames)
					delete[] el;
				return rs;
			}

			Json ExecQuerySql(string aQuery, vector<string> fields, Json& values) {
				Json rs = DbUtils::MakeJsonObject(STSUCCESS);
				string err = "";
				Dm8Con* con = GetConnection(err);
				if (con == nullptr)
					return DbUtils::MakeJsonObject(STDBCONNECTERR, err);
				dpi_alloc_stmt(con->hcon, &con->hstmt);
				
				DPIRETURN rt = dpi_prepare(con->hstmt, (sdbyte*)aQuery.c_str());
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				if (!DSQL_SUCCEEDED(rt))
				{
					dpi_err_msg_print(DSQL_HANDLE_STMT, con->hstmt, err);
					return DbUtils::MakeJsonObject(STDBOPERATEERR, err);;
				}
				int vLen = values.size();
				std::vector<char*> dataInputs;
				dataInputs.resize(vLen);
				std::vector<slength> in_ptrs;
				in_ptrs.resize(vLen);
				std::vector<double> in_dbs;
				in_dbs.resize(vLen);
				std::vector<int> in_ints;
				in_ints.resize(vLen);
				if (vLen > 0)
				{
					for (int i = 0; i < vLen; i++)
					{
						if (values[i].isString()) {
							string ele = values[i].toString();
							int eleLen = ele.length() + 1;
							dataInputs[i] = new char[eleLen];
							memset(dataInputs[i], 0, eleLen);
							memcpy(dataInputs[i], ele.c_str(), eleLen);
							in_ptrs[i] = eleLen - 1;
							rt = dpi_bind_param(con->hstmt, i + 1,
								DSQL_PARAM_INPUT, DSQL_C_NCHAR, DSQL_VARCHAR,
								in_ptrs[i], 0, (void*)dataInputs[i], in_ptrs[i], &in_ptrs[i]);
						}
						else {
							if (getDecimalCount(values[i].toDouble()) > 0) {
								in_dbs[i] = values[i].toDouble();
								in_ptrs[i] = sizeof(in_dbs[i]);
								rt = dpi_bind_param(con->hstmt, i + 1,
									DSQL_PARAM_INPUT, DSQL_C_DOUBLE, DSQL_DOUBLE,
									in_ptrs[i], 0, &in_dbs[i], in_ptrs[i], &in_ptrs[i]);
							}
							else {
								in_ints[i] = values[i].toInt();
								in_ptrs[i] = sizeof(in_ints[i]);
								rt = dpi_bind_param(con->hstmt, i + 1,
									DSQL_PARAM_INPUT, DSQL_C_SLONG, DSQL_INT,
									in_ptrs[i], 0, &in_ints[i], in_ptrs[i], &in_ptrs[i]);
							}

						}
					}
				}
				rt = dpi_exec(con->hstmt);
				if (!DSQL_SUCCEEDED(rt))
				{
					dpi_err_msg_print(DSQL_HANDLE_STMT, con->hstmt, err);
					return DbUtils::MakeJsonObject(STDBOPERATEERR, err);;
				}

				sdint2 num_fields;
				rt = dpi_number_columns(con->hstmt, &num_fields);

				std::vector<sdbyte*> fieldNames;
				fieldNames.resize(num_fields);
				std::vector<sdint2> fieldType;
				fieldType.resize(num_fields);
				std::vector<sdint2> name_len;
				name_len.resize(num_fields);

				std::vector<slength> outPtrs;
				outPtrs.resize(num_fields);
				std::vector<char*> dataOuts;
				dataOuts.resize(num_fields);
				std::vector<double> outDoubles;
				outDoubles.resize(num_fields);
				std::vector<int> outInts;
				outInts.resize(num_fields);

				for (int i = 0; i < num_fields; ++i)
				{
					ulength col_sz;
					sdint2 dec_digits;
					sdint2 nullable;
					fieldNames[i] = new sdbyte[SDBYTE_MAX];
					memset(fieldNames[i], 0, SDBYTE_MAX);
					rt = dpi_desc_column(con->hstmt, i + 1, fieldNames[i], SDBYTE_MAX, &name_len[i],
						&fieldType[i], &col_sz, &dec_digits, &nullable);
					if (!DSQL_SUCCEEDED(rt))
					{
						dpi_err_msg_print(DSQL_HANDLE_STMT, con->hstmt, err);
						return DbUtils::MakeJsonObject(STDBOPERATEERR, err);
					}
					if (fieldType[i] == DSQL_DOUBLE) {
						rt = dpi_bind_col(con->hstmt, i + 1, DSQL_C_DOUBLE, &outDoubles[i], sizeof(double), &outPtrs[i]);
					}
					else if (fieldType[i] == DSQL_INT) {
						rt = dpi_bind_col(con->hstmt, i + 1, DSQL_C_SLONG, &outInts[i], sizeof(int), &outPtrs[i]);
					}
					else {
						dataOuts[i] = new char[SDINT2_MAX];
						memset(dataOuts[i], 0, SDINT2_MAX);
						rt = dpi_bind_col(con->hstmt, i + 1, DSQL_C_NCHAR, dataOuts[i], SDINT2_MAX, &outPtrs[i]);
					}

				}

				Json arr(JsonType::Array);
				ulength row_num;
				while (dpi_fetch(con->hstmt, &row_num) != DSQL_NO_DATA)
				{
					Json al;
					for (int i = 0; i < num_fields; ++i)
					{
						if (fieldType[i] == DSQL_DOUBLE) {
							al.add(string((char*)(fieldNames[i])), outDoubles[i]);
						}
						else if (fieldType[i] == DSQL_INT) {
							al.add(string((char*)(fieldNames[i])), outInts[i]);
						}
						else {
							string tmp(dataOuts[i]);
							al.add(string((char*)(fieldNames[i])), tmp.erase(tmp.find_last_not_of(" ") + 1));
						}
					}
					arr.push_back(al);
				}
				if (arr.isEmpty())
					rs.extend(DbUtils::MakeJsonObject(STQUERYEMPTY));
				rs.add("data", arr);
				dpi_free_stmt(con->hstmt);
				for (auto el : dataOuts)
					delete[] el;
				for (auto el : fieldNames)
					delete[] el;
				return rs;
			}

			Json ExecNoneQuerySql(string aQuery) {
				Json rs = DbUtils::MakeJsonObject(STSUCCESS);
				string err = "";
				Dm8Con* con = GetConnection(err);
				if (con == nullptr)
					return DbUtils::MakeJsonObject(STDBCONNECTERR, err);
				dpi_alloc_stmt(con->hcon, &con->hstmt); 
				DPIRETURN rt = dpi_exec_direct(con->hstmt, (sdbyte*)aQuery.c_str());
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				if (!DSQL_SUCCEEDED(rt))
				{
					dpi_err_msg_print(DSQL_HANDLE_STMT, con->hstmt, err);
					return DbUtils::MakeJsonObject(STDBOPERATEERR, err);
				}
				dpi_free_stmt(con->hstmt);
				return rs;
			}

			Json ExecNoneQuerySql(string aQuery, Json values) {
				Json rs = DbUtils::MakeJsonObject(STSUCCESS);
				string err = "";
				Dm8Con* con = GetConnection(err);
				if (con == nullptr)
					return DbUtils::MakeJsonObject(STDBCONNECTERR, err);
				dpi_alloc_stmt(con->hcon, &con->hstmt);
				DPIRETURN rt = dpi_prepare(con->hstmt, (sdbyte*)aQuery.c_str());
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				if (!DSQL_SUCCEEDED(rt))
				{
					dpi_err_msg_print(DSQL_HANDLE_STMT, con->hstmt, err);
					return DbUtils::MakeJsonObject(STDBOPERATEERR, err);;
				}
				int vLen = values.size();
				std::vector<char*> dataInputs;
				dataInputs.resize(vLen); 
				std::vector<slength> in_ptrs;
				in_ptrs.resize(vLen);
				std::vector<double> in_dbs;
				in_dbs.resize(vLen);
				std::vector<int> in_ints;
				in_ints.resize(vLen);
				if (vLen > 0)
				{
					for (int i = 0; i < vLen; i++)
					{
						if (values[i].isString()) {
							string ele = values[i].toString();
							int eleLen = ele.length() + 1;
							dataInputs[i] = new char[eleLen];
							memset(dataInputs[i], 0, eleLen);
							memcpy(dataInputs[i], ele.c_str(), eleLen);
							in_ptrs[i] = eleLen - 1;
							rt = dpi_bind_param(con->hstmt, i + 1, 
								DSQL_PARAM_INPUT, DSQL_C_NCHAR, DSQL_VARCHAR, 
								in_ptrs[i], 0, (void*)dataInputs[i], in_ptrs[i], &in_ptrs[i]);
						}
						else {
							if (getDecimalCount(values[i].toDouble()) > 0) {
								in_dbs[i] = values[i].toDouble();
								in_ptrs[i] = sizeof(in_dbs[i]);
								rt = dpi_bind_param(con->hstmt, i + 1,
									DSQL_PARAM_INPUT, DSQL_C_DOUBLE, DSQL_DOUBLE,
									in_ptrs[i], 0, &in_dbs[i], in_ptrs[i], &in_ptrs[i]);
							}
							else {
								in_ints[i] = values[i].toInt();
								in_ptrs[i] = sizeof(in_ints[i]);
								rt = dpi_bind_param(con->hstmt, i + 1,
									DSQL_PARAM_INPUT, DSQL_C_SLONG, DSQL_INT,
									in_ptrs[i], 0, &in_ints[i], in_ptrs[i], &in_ptrs[i]);
							}
							
						}
					}
				}
				rt = dpi_exec(con->hstmt);
				if (!DSQL_SUCCEEDED(rt))
				{
					dpi_err_msg_print(DSQL_HANDLE_STMT, con->hstmt, err);
					return DbUtils::MakeJsonObject(STDBOPERATEERR, err);;
				}
				dpi_free_stmt(con->hstmt);
				for (auto el : dataInputs)
					delete[] el;
				return rs;
			}

			int getDecimalCount(double data) {
				data = std::abs(data);
				data -= (int)data;
				int ct = 0;
				double minValue = 0.0000000001;
				while (!(std::abs(data - 1) < minValue || std::abs(data) < minValue) && ct < 11) {
					data *= 10;
					data -= (int)data;
					ct++;
					minValue *= 10;
				}
				return ct;
			}

			bool ExecSqlForTransGo(Dm8Con* con, string aQuery, Json values = Json(JsonType::Array), string* out = nullptr) {
				DPIRETURN rt = dpi_alloc_stmt(con->hcon, &con->hstmt);
				rt = dpi_prepare(con->hstmt, (sdbyte*)aQuery.c_str());
				!DbLogClose && std::cout << "SQL: " << aQuery << std::endl;
				if (!DSQL_SUCCEEDED(rt))
				{
					string err;
					dpi_err_msg_print(DSQL_HANDLE_STMT, con->hstmt, err);
					if (out)
						*out += err;
					return false;
				}
				int vLen = values.size();
				std::vector<char*> dataInputs;
				dataInputs.resize(vLen);
				std::vector<slength> in_ptrs;
				in_ptrs.resize(vLen);
				std::vector<double> in_dbs;
				in_dbs.resize(vLen);
				std::vector<int> in_ints;
				in_ints.resize(vLen);
				if (vLen > 0)
				{
					for (int i = 0; i < vLen; i++)
					{
						if (values[i].isString()) {
							string ele = values[i].toString();
							int eleLen = ele.length() + 1;
							dataInputs[i] = new char[eleLen];
							memset(dataInputs[i], 0, eleLen);
							memcpy(dataInputs[i], ele.c_str(), eleLen);
							in_ptrs[i] = eleLen - 1;
							rt = dpi_bind_param(con->hstmt, i + 1,
								DSQL_PARAM_INPUT, DSQL_C_NCHAR, DSQL_VARCHAR,
								in_ptrs[i], 0, (void*)dataInputs[i], in_ptrs[i], &in_ptrs[i]);
						}
						else {
							if (getDecimalCount(values[i].toDouble()) > 0) {
								in_dbs[i] = values[i].toDouble();
								in_ptrs[i] = sizeof(in_dbs[i]);
								rt = dpi_bind_param(con->hstmt, i + 1,
									DSQL_PARAM_INPUT, DSQL_C_DOUBLE, DSQL_DOUBLE,
									in_ptrs[i], 0, &in_dbs[i], in_ptrs[i], &in_ptrs[i]);
							}
							else {
								in_ints[i] = values[i].toInt();
								in_ptrs[i] = sizeof(in_ints[i]);
								rt = dpi_bind_param(con->hstmt, i + 1,
									DSQL_PARAM_INPUT, DSQL_C_SLONG, DSQL_INT,
									in_ptrs[i], 0, &in_ints[i], in_ptrs[i], &in_ptrs[i]);
							}
						}
					}
				}
				rt = dpi_exec(con->hstmt);
				if (!DSQL_SUCCEEDED(rt))
				{
					string err;
					dpi_err_msg_print(DSQL_HANDLE_STMT, con->hstmt, err);
					if (out)
						*out += err;
					return false;
				}
				rt = dpi_free_stmt(con->hstmt);
				for (auto el : dataInputs)
					delete[] el;
				return true;
			}

			bool escapeString(string& dest)
			{
				string sql = dest;
				dest = "";
				char escape;
				for (auto character : sql) {
					switch (character) {
					case 0: /* Must be escaped for 'mysql' */
						escape = '0';
						break;
					case '\n': /* Must be escaped for logs */
						escape = 'n';
						break;
					case '\r':
						escape = 'r';
						break;
					case '\\':
						escape = '\\';
						break;
					case '\'':
						escape = '\'';
						break;
					case '"': /* Better safe than sorry */
						escape = '"';
						break;
					case '\032': /* This gives problems on Win32 */
						escape = 'Z';
						break;
					default:
						escape = 0;
					}
					if (escape != 0) {
						dest += '\\';
						dest += escape;
					}
					else {
						dest += character;
					}
				}
				return true;
			}

		private:
			vector<Dm8Con*> pool;
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