import { useRef, useEffect } from 'react'
import {
  Clock,
  Scene,
  OrthographicCamera,
  ShaderMaterial,
  Mesh,
  PlaneGeometry,
  WebGLRenderer,
  PCFShadowMap,
  ACESFilmicToneMapping
} from 'three'
import * as tweakpane from 'tweakpane'
import fragmentShader from '../../webgl/shaders/fragment.glsl'
import vertexShader from '../../webgl/shaders/vertex.glsl'
import AudioScene from './audioScene'
import qrcode from 'qrcode-generator'

function App() {
  const canvas = useRef(null)
  const qrcodeCont = useRef(null)
  const PARAMS = (() => {
    let _params = {
      aspectRatio: 0,
      subdivs: {
        value: 3
      },
      time: {
        value: 0
      },
      currScene: {
        value: 'audio'
      },
      angleZ: {
        value: 0
      },
      posValues: {
        type: 'vec2',
        value: new Array(9).fill(0)
      }
    }
    return _params
  })()
  const updateData = (data) => {
    if (data[0] === '/data') {
      const index = data[2] * PARAMS.subdivs.value + data[1]
      PARAMS.posValues.value[index] = data[3]
    } else if (data[0] === '/gyro') {
      PARAMS.angleZ.value = data[1]
    }
  }

  const init = () => {
    const pane = new tweakpane.Pane()
    const scene = new Scene()

    PARAMS.aspectRatio = window.innerWidth / window.innerHeight

    const material = new ShaderMaterial({ uniforms: PARAMS, vertexShader, fragmentShader })
    const plane = new Mesh(new PlaneGeometry(2, 2), material)
    plane.position.y = 0
    plane.position.z = 0
    scene.add(plane)
    const sizes = {
      width: window.innerWidth,
      height: window.innerHeight
    }

    window.addEventListener('resize', () => {
      sizes.width = window.innerWidth
      sizes.height = window.innerHeight

      camera.aspect = PARAMS.aspectRatio.value = sizes.width / sizes.height
      camera.updateProjectionMatrix()

      renderer.setSize(sizes.width, sizes.height)
      renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))
    })

    const camera = new OrthographicCamera(-1, 1, 1, -1, 0, 1)
    scene.add(camera)
    camera.lookAt(plane.position)

    const renderer = new WebGLRenderer({
      canvas: canvas.current,
      antialias: true,
      alpha: true,
      powerPreference: 'high-performance'
    })

    let audioScene = new AudioScene(renderer, pane, PARAMS)

    renderer.shadowMap.enabled = true
    renderer.shadowMap.type = PCFShadowMap
    renderer.toneMapping = ACESFilmicToneMapping
    renderer.toneMappingExposure = 1
    renderer.setSize(sizes.width, sizes.height)
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))

    const shaderFolder = pane.addFolder({ title: 'Shader', expanded: false })
    for (let i = 0; i < 9; i++) {
      shaderFolder.addBinding(PARAMS.posValues.value, i.toString(), { min: 0, max: 1 })
    }

    // Scene changer
    pane
      .addBinding(PARAMS.currScene, 'value', {
        options: {
          shader: 'shader',
          audio: 'audio'
        }
      })
      .on('change', (value) => {
        console.log('value : ', value)
      })

    const clock = new Clock()

    const tick = () => {
      const elapsedTime = clock.getElapsedTime()
      PARAMS.time = elapsedTime
      if (PARAMS.currScene.value === 'shader') {
        renderer.render(scene, camera)
      } else if (PARAMS.currScene.value === 'audio') {
        let delta = clock.getDelta(clock.getDelta())
        renderer.render(audioScene, audioScene.camera)
        audioScene.controls?.update(delta)
      }
      window.requestAnimationFrame(tick)
    }
    tick()
  }
  useEffect(() => {
    init()
  }, [])

  window.api.oscMsg((evt, data) => {
    if (['/data', '/gyro'].includes(data[0])) {
      updateData(data)
    }
  })
  window.api.ipAddress((evt, data) => {
    const qr = qrcode(4, 'L')
    let ipaddress = `${data}:9419`
    qr.addData(ipaddress)
    qr.make()
    qrcodeCont.current.innerHTML = qr.createImgTag()
  })

  return (
    <div>
      <div ref={qrcodeCont} className="qrcode" />
      <canvas ref={canvas} className="test" />
    </div>
  )
}

export default App
