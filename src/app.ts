import Koa from 'koa'
import Startup from './inits'

//加载中间件
export default {
    async init() {
        const app = new Koa()
        //支持 X-Forwarded-Host
        app.proxy = true
        const middlewares = [
            'cors',
            'logger', //记录所用方式与时间
            //'session',
            'globalError', // 全局错误处理
            'conditional', //配合etag
            'etag', //etag 客户端缓存处理
            'bodyParser', //body解析
            'rewrite', //url重写
            'static',
            'router',
        ]
        for (const n of middlewares) {
            if (n) {
                const middleware = await this.loadMiddleware.apply(null, [].concat(n))
                if (middleware) {
                    //考虑返回多个中间件
                    for (const m of [].concat(middleware)) {
                        // eslint-disable-next-line prefer-spread
                        m && (app.use.apply(app, [].concat(m)))
                    }
                }
            }
        }
        //其他初始化处理  directory socket schedule ...
        await new Startup().init(app)
        return app
    },
    async loadMiddleware(name: string, ...args: any[]) {
        // eslint-disable-next-line @typescript-eslint/no-var-requires
        const middleware = require('./middlewares/' + name).default
        // eslint-disable-next-line prefer-spread
        return (middleware && await middleware(args)) || async function (ctx: Koa.DefaultContext, next: () => any) { await next() }
    }
}
