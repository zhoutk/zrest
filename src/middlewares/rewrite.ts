import rewrite from 'koa-rewrite'
export default () => {
    return [
        rewrite('/index.html', '/')
    ]
}