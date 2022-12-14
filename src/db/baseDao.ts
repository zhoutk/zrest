/* eslint-disable @typescript-eslint/no-unused-vars */
// import TransElement from '../common/transElement'
// import * as moment from 'moment'

export default class BaseDao {
    table: string
    constructor(table?: string) {
        this.table = table || ''
    }
    async retrieve(params = {}, fields = [], session = { userid: '' }): Promise<any> {
        let rs: object
        try {
            rs = JSON.parse(G.ORM.select(this.table, JSON.stringify(params), fields.join(',')))
        } catch (err) {
            rs = G.jsResponse(G.STCODES.EXCEPTIONERR, `data query fail: ${(err as Error).message}`)
        }
        // if (rs.status === G.STCODES.SUCCESS)
        //     return processDatum(rs)
        // else
        return rs
    }
    async create(params = {}, fields = [], session = { userid: '' }): Promise<any> {
        let keys = Object.keys(params)
        if (keys.length === 0 || params['id'] !== undefined && keys.length <= 1)
            return G.jsResponse(G.STCODES.PARAMERR, 'params is error.')
        else {
            let rs, id = params['id']
            try {
                if (!id) {
                    id = G.uuid()
                } 
                rs = JSON.parse(G.ORM.create(this.table, JSON.stringify(Object.assign(params, id ? { id } : {}))))
            } catch (err) {
                return G.jsResponse(G.STCODES.EXCEPTIONERR, `data create fail: ${(err as Error).message}`)
            }
            return G.L.extend(rs, {id})
        }
    }
    async update(params, fields = [], session = { userid: '' }): Promise<any> {
        params = params || {}
        let keys = Object.keys(params)
        if (params['id'] === undefined || keys.length <= 1)
            return G.jsResponse(G.STCODES.PARAMERR, 'params is error.')
        else {
            let rs
            try {
                rs = JSON.parse(G.ORM.update(this.table, JSON.stringify(params)))
            } catch (err) {
                return G.jsResponse(G.STCODES.EXCEPTIONERR, `data update fail: ${(err as Error).message}`)
            }
            return G.L.extend(rs, { id : params['id'] })
        }
    }
    async delete(params = {}, fields = [], session = { userid: '' }): Promise<any> {
        if (params['id'] === undefined)
            return G.jsResponse(G.STCODES.PARAMERR, 'params is error.')
        else {
            let id = params['id']
            let rs
            try {
                rs = JSON.parse(G.ORM.remove(this.table, JSON.stringify({id})))
            } catch (err) {
                return G.jsResponse(G.STCODES.EXCEPTIONERR, `data remove fail: ${(err as Error).message}`)
            }
            return G.L.extend(rs, { id })
        }
    }
    async querySql(sql: string, values = [], params = {}, fields = []): Promise<any> {
        let rs
        try {
            rs = JSON.parse(G.ORM.querySql(sql, values, params, fields))   
        } catch (err) {
            return G.jsResponse(G.STCODES.EXCEPTIONERR, `data querySql fail: ${(err as Error).message}`)
        }
        // if (rs.status === G.STCODES.SUCCESS)
        //     return processDatum(rs)
        // else
        return rs
    }
    async execSql(sql: string, values = []): Promise<any> {
        let rs
        try {
            rs = JSON.parse(G.ORM.execSql(sql))
        } catch (err) {
            return G.jsResponse(G.STCODES.EXCEPTIONERR, `data execSql fail: ${(err as Error).message}`)
        }
        return rs
    }
    async insertBatch(tablename: string, elements = []): Promise<any> {
        let rs
        try {
            rs = JSON.parse(G.ORM.insertBatch(tablename, JSON.stringify(elements)))
        } catch (err) {
            return G.jsResponse(G.STCODES.EXCEPTIONERR, `data batch fail: ${(err as Error).message}`)
        }
        return rs
    }
    async transGo(sqls = [], isAsync = true): Promise<any> {
        let rs
        try {
            rs = JSON.parse(G.ORM.transGo(JSON.stringify(sqls), isAsync))
        } catch (err) {
            return G.jsResponse(G.STCODES.EXCEPTIONERR, `data trans fail: ${(err as Error).message}`)
        }
        return rs
    }
}

// function processDatum(rs) {
//     rs.data && rs.data.forEach(element => {
//         let vs = Object.entries(element)
//         for (let [key, value] of vs) {
//             if (G.L.endsWith(key, '_time') && value) {
//                 element[key] = moment(value).format('YYYY-MM-DD hh:mm:ss')
//             } else if (G.L.endsWith(key, '_json')) {
//                 if (value && value.toString().trim().length === 0) {
//                     element[key] = null
//                 }
//             }
//         }
//     })
//     return rs
// }