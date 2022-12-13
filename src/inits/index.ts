//初始化在app定义好后运行，以init开的头文件都会被处理，传输参数app并要求返回一个带有init方法的对象。
// eslint-disable-next-line @typescript-eslint/no-var-requires
let requireDir = require('require-dir')
import Koa from 'koa'

export default class Startup {
    async init(app: Koa) {
        const inits = []
        const dirData = requireDir(__dirname)
        G.L.each(dirData, (item, name) => {
            const initOp = name.length > 4 && name.substring(4).toLowerCase()
            if (initOp && G.CONFIGS.inits[initOp] && G.CONFIGS.inits[initOp].run && 
                    name.match(/^init/) && item && item.default && item.default.init) {
                inits.push(item.default)
            }
        })
        for (const item of inits) {
            await item.init(app)
        }
    }
}