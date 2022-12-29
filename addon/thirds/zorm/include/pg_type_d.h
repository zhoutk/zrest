#pragma once

/* OIDS 1 - 99 */
//DESCR("boolean, 'true'/'false'");
#define BOOLOID			16
//DESCR("variable-length string, binary values escaped");
#define BYTEAOID		17
//DESCR("single character");
#define CHAROID			18
//DESCR("63-byte type for storing system identifiers");
#define NAMEOID			19
//DESCR("~18 digit integer, 8-byte storage");
#define INT8OID			20
//DESCR("-32 thousand to 32 thousand, 2-byte storage");
#define INT2OID			21
//DESCR("array of int2, used in system tables");
#define INT2VECTOROID	22
//DESCR("-2 billion to 2 billion integer, 4-byte storage");
#define INT4OID			23
//DESCR("registered procedure");
#define REGPROCOID		24
//DESCR("variable-length string, no limit specified");
#define TEXTOID			25
//DESCR("object identifier(oid), maximum 4 billion");
#define OIDOID			26
//DESCR("(block, offset), physical location of tuple");
#define TIDOID		27
//DESCR("transaction id");
#define XIDOID 28
//DESCR("command identifier type, sequence in transaction id");
#define CIDOID 29
//DESCR("array of oids, used in system tables");
#define OIDVECTOROID	30

/* OIDS 100 - 199 */
#define JSONOID 114
//DESCR("XML content");
#define XMLOID 142
//DESCR("string representing an internal node tree");
#define PGNODETREEOID	194
//DESCR("multivariate ndistinct coefficients");
#define PGNDISTINCTOID	3361
//DESCR("multivariate dependencies");
#define PGDEPENDENCIESOID	3402
//DESCR("internal type for passing CollectedCommand");
#define PGDDLCOMMANDOID 32

//DESCR("storage manager");
/* OIDS 300 - 399 */
/* OIDS 400 - 499 */
/* OIDS 500 - 599 */

/* OIDS 600 - 699 */
//DESCR("geometric point '(x, y)'");
#define POINTOID		600
//DESCR("geometric line segment '(pt1,pt2)'");
#define LSEGOID			601
//DESCR("geometric path '(pt1,...)'");
#define PATHOID			602
//DESCR("geometric box '(lower left,upper right)'");
#define BOXOID			603
//DESCR("geometric polygon '(pt1,...)'");
#define POLYGONOID		604
//DESCR("geometric line");
#define LINEOID			628

/* OIDS 700 - 799 */
//DESCR("single-precision floating point number, 4-byte storage");
#define FLOAT4OID 700
//DESCR("double-precision floating point number, 8-byte storage");
#define FLOAT8OID 701
//DESCR("absolute, limited-range date and time (Unix system time)");
#define ABSTIMEOID		702
//DESCR("relative, limited-range time interval (Unix delta time)");
#define RELTIMEOID		703
//DESCR("(abstime,abstime), time interval");
#define TINTERVALOID	704
//DESCR("");
#define UNKNOWNOID		705
//DESCR("geometric circle '(center,radius)'");
#define CIRCLEOID		718
//DESCR("monetary amounts, $d,ddd.cc");
#define CASHOID 790

/* OIDS 800 - 899 */
//DESCR("XX:XX:XX:XX:XX:XX, MAC address");
#define MACADDROID 829
//DESCR("IP address/netmask, host address, netmask optional");
#define INETOID 869
//DESCR("network IP address/netmask, network address");
#define CIDROID 650
//DESCR("XX:XX:XX:XX:XX:XX:XX:XX, MAC address");
#define MACADDR8OID 774

/* OIDS 900 - 999 */
/* OIDS 1000 - 1099 */
#define INT2ARRAYOID		1005
#define INT4ARRAYOID		1007
#define TEXTARRAYOID		1009
#define OIDARRAYOID			1028
#define FLOAT4ARRAYOID 1021
//DESCR("access control list");
#define ACLITEMOID		1033
#define CSTRINGARRAYOID		1263
//DESCR("char(length), blank-padded string, fixed storage length");
#define BPCHAROID		1042
//DESCR("varchar(length), non-blank-padded string, variable storage length");
#define VARCHAROID		1043
//DESCR("date");
#define DATEOID			1082
//DESCR("time of day");
#define TIMEOID			1083

/* OIDS 1100 - 1199 */
//DESCR("date and time");
#define TIMESTAMPOID	1114
//DESCR("date and time with time zone");
#define TIMESTAMPTZOID	1184
//DESCR("@ <number> <units>, time interval");
#define INTERVALOID		1186

/* OIDS 1200 - 1299 */
//DESCR("time of day with time zone");
#define TIMETZOID		1266

/* OIDS 1500 - 1599 */
//DESCR("fixed-length bit string");
#define BITOID	 1560
//DESCR("variable-length bit string");
#define VARBITOID	  1562

/* OIDS 1600 - 1699 */
/* OIDS 1700 - 1799 */
//DESCR("numeric(precision, decimal), arbitrary precision number");
#define NUMERICOID		1700
//DESCR("reference to cursor (portal name)");
#define REFCURSOROID	1790

/* OIDS 2200 - 2299 */
//DESCR("registered procedure (with args)");
#define REGPROCEDUREOID 2202
//DESCR("registered operator");
#define REGOPEROID		2203
//DESCR("registered operator (with args)");
#define REGOPERATOROID	2204
//DESCR("registered class");
#define REGCLASSOID		2205
//DESCR("registered type");
#define REGTYPEOID		2206
//DESCR("registered role");
#define REGROLEOID		4096
//DESCR("registered namespace");
#define REGNAMESPACEOID		4089
#define REGTYPEARRAYOID 2211

/* uuid */
//DESCR("UUID datatype");
#define UUIDOID 2950
//DESCR("PostgreSQL LSN datatype");
#define LSNOID			3220

/* text search */
//DESCR("text representation for text search");
#define TSVECTOROID		3614
//DESCR("GiST index internal text representation for text search");
#define GTSVECTOROID	3642
//DESCR("query representation for text search");
#define TSQUERYOID		3615
//DESCR("registered text search configuration");
#define REGCONFIGOID	3734
//DESCR("registered text search dictionary");
#define REGDICTIONARYOID	3769

/* jsonb */
//DESCR("Binary JSON");
#define JSONBOID 3802

/* range types */
//DESCR("range of integers");
#define INT4RANGEOID		3904
#define RECORDOID		2249
#define RECORDARRAYOID	2287
#define CSTRINGOID		2275
#define ANYOID			2276
#define ANYARRAYOID		2277
#define VOIDOID			2278
#define TRIGGEROID		2279
#define EVTTRIGGEROID		3838
#define LANGUAGE_HANDLEROID		2280
#define INTERNALOID		2281
#define OPAQUEOID		2282
#define ANYELEMENTOID	2283
#define ANYNONARRAYOID	2776
#define ANYENUMOID		3500
#define FDW_HANDLEROID	3115
#define INDEX_AM_HANDLEROID 325
#define TSM_HANDLEROID	3310
#define ANYRANGEOID		3831