export default () => {
    return async (ctx: any, next: () => any) => {
        try {
            await next()
        } catch (err) {
            ctx.body = G.jsResponse(ctx.ErrCode || G.STCODES.EXCEPTIONERR, (err as Error).message, { stack: (err as Error).stack })
        }
    }
}