import Koa from 'koa'

export default () => {
    return async (ctx: Koa.DefaultContext, next: () => any) => {
        try {
            await next()
        } catch (err) {
            ctx.body = G.jsResponse(ctx.ErrCode || G.STCODES.EXCEPTIONERR, (err as Error).message, { stack: (err as Error).stack })
        }
    }
}