import { resolve } from 'path'
import { defineConfig, externalizeDepsPlugin } from 'electron-vite'
import react from '@vitejs/plugin-react'
import glsl from 'vite-plugin-glsl'

export default defineConfig({
  main: {
    plugins: [externalizeDepsPlugin()],
    build: {
      lib: {
        entry: './electron/main.js'
      },
      outDir: './out'
    }
  },
  preload: {
    build: {
      lib: {
        entry: './electron/preload.ts'
      }
    },
    plugins: [externalizeDepsPlugin()]
  },
  renderer: {
    appType: 'mpa',
    build: {
      rollupOptions: {
        input: {
          main: resolve(__dirname, 'src/renderer/index.html')
        }
      }
    },
    resolve: {
      alias: {
        '@renderer': resolve('src/renderer')
      }
    },
    plugins: [react(), glsl()]
  }
})
