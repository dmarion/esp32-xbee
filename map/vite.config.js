import viteCompression from 'vite-plugin-compression';

export default {
    plugins: [viteCompression({ deleteOriginFile: true })],
    base: '/map/',
    build: {
        sourcemap: false,
        outDir: '../www/map',
        emptyOutDir: true,
        rollupOptions: {
            input: {
                app: 'index.html',
            },
            output: {
                assetFileNames: (assetInfo) => {
                    let extType = assetInfo.name.split('.').at(1);
                    if (/png|jpe?g|svg|gif|tiff|bmp|ico/i.test(extType)) {
                        extType = 'img';
                    }
                    return `[name][extname]`;
                },
                chunkFileNames: '[name]-[hash].js',
                entryFileNames: '[name].js',
            },
        },
    },
    server: {
        host: true,
        open: '/map/',
        proxy: {
            '/ubx/pos': {
                target: 'http://esp32xbee.local/',
                changeOrigin: true,
                secure: false,
                configure: (proxy, _options) => {
                    proxy.on('error', (err, _req, _res) => {
                        console.log('proxy error', err);
                    });
                    proxy.on('proxyReq', (proxyReq, req, _res) => {
                        console.log('Sending Request to the Target:', req.method, req.url);
                    });
                    proxy.on('proxyRes', (proxyRes, req, _res) => {
                        console.log('Received Response from the Target:', proxyRes.statusCode, req.url);
                    });
                },
            }
        },
    },
}
