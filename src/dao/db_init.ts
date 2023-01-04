/* eslint-disable @typescript-eslint/no-unused-vars */
import BaseDao from '../db/baseDao'

export default class DbInit extends BaseDao {
    constructor(table: string) {
        super(table)
    }
    async retrieve(params = {}, fields = [], session = { userid: '' }): Promise<any> {
        return new Promise((resolve, reject) => {
            reject(G.jsResponse(G.STCODES.NOTFOUNDERR))
        })
    }
    async create(params = {}, fields = [], session = { userid: '' }): Promise<any> {
        if(G.CONFIGS.db_dialect == 'sqlite3'){
            G.ORM.execSql('DROP TABLE IF EXISTS "table_for_test";')
            G.ORM.execSql('CREATE TABLE "table_for_test" (\
                        "id" char(64) NOT NULL,\
                        "name" TEXT DEFAULT "",\
                        "age" integer DEFAULT 0,\
                        "score" real DEFAULT 0.0,\
                        PRIMARY KEY ("id"));')
        }else if(G.CONFIGS.db_dialect == 'postgres'){
            G.ORM.execSql('DROP TABLE IF EXISTS "public"."table_for_test";')
            G.ORM.execSql('CREATE TABLE "public"."table_for_test" (\
                "id" char(64) NOT NULL,\
                "name" varchar(128) DEFAULT \'\'::character varying,\
                "age" int4 DEFAULT 0,\
                "score" numeric DEFAULT 0.0)')
            G.ORM.execSql('ALTER TABLE "public"."table_for_test" ADD CONSTRAINT "table_for_test_pkey" PRIMARY KEY ("id");')
        }else if(G.CONFIGS.db_dialect == 'mysql'){
            G.ORM.execSql('DROP TABLE IF EXISTS `table_for_test`;')
            G.ORM.execSql('CREATE TABLE `table_for_test` (\
                `id` char(64) CHARACTER SET utf8mb4 NOT NULL,\
                `name` varchar(128) CHARACTER SET utf8mb4 NULL DEFAULT \'\',\
                `age` int(0) NULL DEFAULT 0,\
                `score` double NULL DEFAULT 0,\
                PRIMARY KEY (`id`) USING BTREE\
                ) ENGINE = InnoDB CHARACTER SET = utf8mb4 ROW_FORMAT = Dynamic;')
        }else{
            return new Promise((resolve, reject) => {
                reject(G.jsResponse(G.STCODES.DBDIALECTNOTSUPPORT))
            })
        }
        
        let cObj: object = {
            id: 'a1b2c3d4',
            name: 'Kevin 凯文',
            age: 18,
            score: 99.99
        }
        return JSON.parse(G.ORM.create('table_for_test', JSON.stringify(cObj)))
    }
    async update(params = {}, fields = [], session = { userid: '' }): Promise<any> {
        return new Promise((resolve, reject) => {
            reject(G.jsResponse(G.STCODES.NOTFOUNDERR))
        })
    }
    async delete(params = {}, fields = [], session = { userid: '' }): Promise<any> {
        return new Promise((resolve, reject) => {
            reject(G.jsResponse(G.STCODES.NOTFOUNDERR))
        })
    }
}