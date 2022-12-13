import Router from 'koa-router'
//import BaseDao from '../db/baseDao'
const router = new Router()

// const METHODS = {
//     GET: 'retrieve',
//     POST: 'create',
//     PUT: 'update',
//     DELETE: 'delete'
// }

export default (() => {
    const process = async (ctx: any, next) => {
        ctx.body = `rs result -- ${JSON.stringify(ctx.params)}`
        const method: string = ctx.method.toUpperCase()
        //let tableName: string = ctx.params.table
        const id: string | number | undefined = ctx.params.id
        let params = method === 'POST' || method === 'PUT' ? ctx.request.body : ctx.request.query
        if (id != null)
            params.id = id
        // let {fields, ...restParams} = params
        // if (fields) {
        //     fields = G.arryParse(fields)
        //     if (!fields) {
        //         throw G.koaError(ctx, G.STCODES.PARAMERR, 'params fields is wrong.')
        //     }
        // }

        // let module = loadModule(`../dao/${tableName}`), is_module_exist = true
        // if (!module) {
        //     is_module_exist = false
        //     module = require('../db/baseDao')
        // }

        // if (method === 'GET' && !tableName.startsWith('v_') && (!is_module_exist || 
        //     is_module_exist && !Object.getOwnPropertyNames(module.default.prototype).some((al) => al === 'retrieve')) ) {
        //     let sqlFindView : string
        //     if(G.CONFIGS.db_dialect == 'sqlit3')
        //         sqlFindView = `SELECT name FROM sqlite_master WHERE type = 'view' AND name = 'v_${tableName}'`
        //     else if(G.CONFIGS.db_dialect == 'mysql')
        //         sqlFindView = `SELECT TABLE_NAME FROM INFORMATION_SCHEMA.VIEWS WHERE TABLE_SCHEMA= '${G.CONFIGS.db_cfg_name}' and TABLE_NAME= 'v_${tableName}' `
        //     else
        //         sqlFindView = ''
        //     if(sqlFindView.length > 0){
        //         let rs = await new BaseDao().querySql(sqlFindView)
        //         if (rs.status === 200)
        //             tableName = 'v_' + tableName
        //     }
        // }

        // let rs: any
        // try {
        //     let db = new module.default(tableName)
        //     rs = await db[METHODS[method]](restParams, fields, ctx.session)
        // } catch (err) {
        //    rs = G.jsResponse(G.STCODES.EXCEPTIONERR, (err as Error).message, {stack: (err as Error).stack})
        //}
        ctx.body = G.jsResponse(G.STCODES.EXCEPTIONERR, 'It\'s a test!', {id})
    }
    return router.all('/rs/:table', process).all('/rs/:table/:id', process)
})() 

// function loadModule(path: string) {
//     try {
//         return require(path)
//     } catch (err) {
//         if ((err as Error).message.indexOf('Cannot find module') < 0)
//             G.logger.error((err as Error).message)
//         return null
//     }
// }
