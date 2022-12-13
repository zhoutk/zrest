import serve from 'koa-static'
import path from 'path'
export default () => {
    return serve(path.join(G.ROOT_PATH, 'public'))
}