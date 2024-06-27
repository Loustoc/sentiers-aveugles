const { app, BrowserWindow, dialog } = require('electron')
const express = require('express')
const { createServer } = require('node:http')
const { join } = require('node:path')
const path = require('path')
import { electronApp, optimizer } from '@electron-toolkit/utils'
import icon from '../resources/icon.png?asset'
import ip from 'ip'
import { Server as _oscServer } from 'node-osc'
import child_process from 'child_process'
let kinectStarted = false
let restartLock = false
let startTimeout

function run_script(command, args, callback) {
  var child = child_process.spawn(command, args, {
    encoding: 'utf8',
    shell: true
  })
  child.on('error', (error) => {
    dialog.showMessageBox({
      title: 'Title',
      type: 'warning',
      message: 'Error occured.\r\n' + error
    })
  })

  child.stdout.setEncoding('utf8')
  child.stdout.on('data', (data) => {
    data = data.toString()
    console.log(data)
  })

  child.stderr.setEncoding('utf8')
  child.stderr.on('data', (data) => {
    mainWindow.webContents.send('mainprocess-response', data)
    console.log(data)
  })

  // child.on('close', (code) => {
  //   switch (code) {
  //     case 0:
  //       dialog.showMessageBox({
  //         title: 'Title',
  //         type: 'info',
  //         message: 'End process.\r\n'
  //       })
  //       break
  //   }
  // })
  if (typeof callback === 'function') callback()
}

var oscServer = new _oscServer(9419, '0.0.0.0', () => {
  console.log('OSC Server is listening')
})

oscServer.on('message', function (msg) {
  if (msg[0] === '/start') {
    kinectStarted = true
    clearInterval(startTimeout)
    console.log('kinect started')
  } else {
    BrowserWindow.getAllWindows()[0]?.webContents.send('oscMsg', msg)
  }
})

//Check if compiled version or dev
const isDev = process.mainModule.filename.indexOf('app.asar') === -1
console.log('isDev: ', isDev)
let resources_dir
if (!isDev) {
  resources_dir = path.join(__dirname, '../../')
  resources_dir = path.join(resources_dir, '/out')
} else {
  resources_dir = __dirname
}

let ipaddress

const getLocalIp = () => {
  ipaddress = ip.address()
  console.log('ip address: ' + ipaddress)
  BrowserWindow.getAllWindows()[0]?.webContents.send('ipAddress', ipaddress)
}

const express_app = express()
const server = createServer(express_app)

express_app.use(express.static(join(resources_dir, isDev ? './renderer' : '../renderer')))
const render_path = path.join(resources_dir, isDev ? '.' : '..', '/renderer')
console.log(render_path)

express_app.get('/', (req, res) => {
  res.sendFile(join(render_path, '/main/index.html'))
})

server.listen(3000, () => {
  console.log('server running at http://localhost:3000')
})

let mainWindow

function createWindow() {
  // Create the browser window.
  mainWindow = new BrowserWindow({
    width: 900,
    height: 670,
    show: false,
    fullscreenable: true,
    fullscreen: false,
    skipTaskbar: true,
    autoHideMenuBar: true,
    ...(process.platform === 'linux' ? { icon } : {}),
    webPreferences: {
      preload: join(__dirname, 'preload.js'),
      sandbox: false
    }
  })

  mainWindow.on('ready-to-show', () => {
    mainWindow.show()
    getLocalIp()
    run_script(`../kinect/sentiers-aveugles`)
    startTimeout = setInterval(() => {
      if (!kinectStarted && !restartLock) {
        restartLock = true
        console.log('kinect not started')
        run_script(`pkill sentiers-aveugles`)
        setTimeout(() => {
          run_script(`../kinect/sentiers-aveugles`)
          restartLock = false
        }, 3000)
      }
    }, 5000)
  })

  mainWindow.loadURL('http://localhost:3000')
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(() => {
  // Set app user model id for windows
  electronApp.setAppUserModelId('com.electron')

  // Default open or close DevTools by F12 in development
  // and ignore CommandOrControl + R in production.
  // see https://github.com/alex8088/electron-toolkit/tree/master/packages/utils
  app.on('browser-window-created', (_, window) => {
    optimizer.watchWindowShortcuts(window)
  })

  createWindow()

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('before-quit', () => {
  if (process.platform !== 'darwin') {
    oscServer.close()
    run_script(`pkill sentiers-aveugles`)
    app.quit()
  }
})
