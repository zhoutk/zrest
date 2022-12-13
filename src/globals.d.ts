import { GlobVar } from './inits/global'

type GLOB = typeof GlobVar

declare global {
    let G: GLOB
}