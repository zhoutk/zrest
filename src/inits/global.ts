import {STCODES} from './enums'
import lodash from 'lodash'
import {STMESSAGES} from './enums'
import { configure, getLogger} from 'log4js'
import logCfg from '../config/log4js'
import CONFIGS from '../config/configs'

const env = process.env.NODE_ENV || 'dev'            //dev - 开发; prod - 生产； test - 测试;
const GlobVar = {
    L: lodash,
    CONFIGS,
    PAGESIZE: 10,
    STCODES,
    ROOT_PATH: `${process.cwd()}${env === 'dev' ? '' : '/dist'}`,
    NODE_ENV: env,
    logger: (() => {
        configure(logCfg)
        return getLogger('default')
    })(),
    jsResponse(status: number, message = '', data?: object) {
        if (Array.isArray(data))
            return { status, message: message === '' ? (STMESSAGES[status.toString()] || '') : message, data }
        else
            return Object.assign({}, data, { status, message: message === '' ? (STMESSAGES[status.toString()] || '') : message })
    },
    koaError(ctx: any, status: number, message: string) {
        ctx.ErrCode = status
        return new KoaErr({ message, status })
    },
    isDev() {
        return G.NODE_ENV === 'dev'
    },
    arryParse(arr: any): Array<any> | null {
        try {
            if (Array.isArray(arr) || G.L.isNull(arr))
                return arr
            else if (typeof arr === 'string') {
                if (arr.startsWith('['))
                    arr = JSON.parse(arr)
                else
                    arr = arr.split(',')
            } else 
                return null
        } catch (err) {
            arr = null
        }
        return arr
    }
}

async function globInit() {
    Object.assign(global, { G: GlobVar })
}

class KoaErr extends Error {
    public status: number
    constructor({ message = 'Error', status = G.STCODES.EXCEPTIONERR } = {}, ...args: any[]) {
        super()
        this.message = message
        this.status = status
        if (args.length > 0) {
            Object.assign(this, args[0])
        }
    }
}

export { globInit, GlobVar }