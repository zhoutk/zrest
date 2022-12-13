// eslint-disable-next-line @typescript-eslint/no-var-requires
let requireDir = require('require-dir')
export default (() => {
    const inits = []
    const dirData = requireDir(__dirname)
    G.L.each(dirData, (item, name) => {
        const initOp = name.length > 7 && name.substring(7).toLowerCase()
        if (initOp && name.match(/^router/) && item && item.default) {
            inits.push(item.default)
        }
    })
    const middles = []
    for (const item of inits) {
        middles.push(item.routes())
        middles.push(item.allowedMethods())
    }
    return middles
})()