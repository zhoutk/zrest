import * as jwt from 'jsonwebtoken'
import Koa from 'koa'
const config = G.CONFIGS.jwt
const AUTHURL = ['rs']

export default () => {
    return async (ctx: Koa.DefaultContext, next: () => any) => {
        const { header: { token } } = ctx
        const urlStrs = ctx && ctx.url && ctx.url.split('/')
        const isAuth: boolean = AUTHURL.some((url) => { return urlStrs[1] === url })
        if (token) {
            try {
                const decoded = jwt.verify(token, config.secret)
                ctx.session = decoded
                await next()
            } catch (err) {
                if (ctx.method === 'GET' || !isAuth) {
                    return await next()
                }
                if ((err as Error).name === 'TokenExpiredError') {
                    ctx.body = G.jsResponse(G.STCODES.JWTAUTHERR, 'Token Expired.')
                } else if ((err as Error).name === 'JsonWebTokenError') {
                    ctx.body = G.jsResponse(G.STCODES.JWTAUTHERR, 'Invalid Token.')
                } else {
                    ctx.body = G.jsResponse(G.STCODES.JWTAUTHERR, (err as Error).message)
                }
            }
        } else {
            if (ctx.method !== 'GET' && isAuth) {
                ctx.body = G.jsResponse(G.STCODES.JWTAUTHERR, 'Missing Auth Token.')
            } else {
                await next()
            }
        }
    }
}
