import middles from '../../routers'
import Koa from 'koa'

export default async () => {
    middles.push(async (ctx: Koa.DefaultContext) => {
        ctx.body = G.jsResponse(G.STCODES.NOTFOUNDERR, 'What you request is not found.')
    })
    return middles
}
