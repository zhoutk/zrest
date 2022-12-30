export default {
    inits: {
        directory: {
            run: false,
            dirs: ['public/upload', 'public/temp']
        },
        socket: {
            run: false
        }
    },
    port: 3000,
    db_dialect: 'mysql',
    db_log_close: false,
    db_options: {
        db_host: '192.168.0.136',
        db_port: 3306,
        db_name: 'encweb',
        db_user: 'root',
        db_pass: '123456',
        db_char: 'utf8mb4',
        db_conn: 5,
    },
    jwt: {
        secret: 'zh-123456SFU>a4bh_$3#46d0e85W10aGMkE5xKQ',
        expires_max: 36000      
    },
}