import bodyParser from 'koa-body'
export default () => {
    return bodyParser({jsonLimit: '500mb', formLimit: '500mb'})
}