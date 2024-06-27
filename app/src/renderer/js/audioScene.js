import {
  SphereGeometry,
  ShaderMaterial,
  Scene,
  BoxGeometry,
  Mesh,
  Color,
  MeshBasicMaterial,
  PerspectiveCamera,
  AudioLoader,
  AudioListener,
  PositionalAudio
} from 'three'
import audio1 from '../../../assets/sounds/city.mp3'
import audio2 from '../../../assets/sounds/leaves.mp3'
import audio3 from '../../../assets/sounds/crickets.mp3'
import audio4 from '../../../assets/sounds/waterflow.mp3'
import audio5 from '../../../assets/sounds/thunder.mp3'
import audio6 from '../../../assets/sounds/wave.mp3'
import audio7 from '../../../assets/sounds/wind_howling.mp3'
import audio8 from '../../../assets/sounds/swamp.mp3'
import audio9 from '../../../assets/sounds/quiet_crowd.mp3'
import audio10 from '../../../assets/sounds/crows.mp3'
import audio11 from '../../../assets/sounds/campfire.mp3'
import { PositionalAudioHelper } from 'three/addons/helpers/PositionalAudioHelper.js'

export default class AudioScene extends Scene {
  constructor(rend, pane, _PARAMS) {
    super()
    this.subdivs = 3
    this._PARAMS = _PARAMS
    this.meshes = []
    this.meshesPos = []
    let geometry = new SphereGeometry(0.3, 100, 100)
    let _mat = new ShaderMaterial({
      vertexShader: `varying vec2 vUv;uniform float uTime;void main(){gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);vUv = uv;}`,
      fragmentShader: `varying vec2 vUv;void main()
      {
          vec2 uv = vUv;
          gl_FragColor = vec4(uv.x > 0.8 && uv.x < 0.9 ? 1.0 : 0.0, 0.0, 0.0, 1.0);
      }`
    })
    this._mesh = new Mesh(geometry, _mat)
    this.add(this._mesh)
    for (let i = 0; i < this.subdivs; i++) {
      for (let j = 0; j < this.subdivs; j++) {
        let geometry = new BoxGeometry(0.2, 0.2, 0.2)
        let color = i == 0 && j == 0 ? new Color(0x00ff00) : new Color(0x0000ff)
        let material = new MeshBasicMaterial({ color })
        let mesh = new Mesh(geometry, material)
        mesh.position.set(i * this.subdivs, 0, j * this.subdivs)
        this.add(mesh)
        this.meshes.push(mesh)
        this.meshesPos.push([i * this.subdivs, j * this.subdivs])
      }
    }
    this.sounds = []
    this.background = new Color(0x222222)

    this.aboveCamera = new PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 100)
    this.aboveCamera.position.set(3, 12, 3)
    this.aboveCamera.rotation.x = -Math.PI / 2

    this.PARAMS = { ignoreOsc: false }

    document.addEventListener('keydown', (event) => {
      console.log(this.PARAMS.ignoreOsc)
      if (!this.PARAMS.ignoreOsc) return
      console.log('couocu')
      if (event.key === 'z') {
        this._mesh.position.z += -0.2
      }
      if (event.key === 's') {
        this._mesh.position.z += 0.2
      }
      if (event.key === 'q') {
        this._mesh.position.x += -0.2
      }
      if (event.key === 'd') {
        this._mesh.position.x += 0.2
      }
      if (event.key === 'ArrowLeft') {
        this._mesh.rotation.y += 0.2
      }
      if (event.key === 'ArrowRight') {
        this._mesh.rotation.y -= 0.2
      }
      if (event.key === 'u') {
        this._mesh.lookAt(this.meshes[0].position)
      }
    })

    this.camera = this.aboveCamera
    const audioSceneFolder = pane.addFolder({ title: 'Audio Scene' })
    audioSceneFolder.addBinding(this.PARAMS, 'ignoreOsc', { label: 'Ignore OSC' })
    this.loadAudio()
    this.tick()
  }
  loadAudio() {
    let _sounds = [
      audio1,
      audio2,
      audio3,
      audio4,
      audio5,
      audio6,
      audio7,
      audio8,
      audio9,
      audio10,
      audio11
    ]
    let audioLoader = new AudioLoader()
    shuffle(_sounds)
    for (let i = 0; i < this.subdivs * this.subdivs; i++) {
      let listener = new AudioListener()
      this._mesh.add(listener)
      let sound = new PositionalAudio(listener)
      const helper = new PositionalAudioHelper(sound)
      audioLoader.load(
        _sounds[i],
        function (buffer) {
          sound.setBuffer(buffer)
          sound.setRefDistance(0.5)
          sound.setLoop(true)
          sound.play()
          sound.setDirectionalCone(360, 360, 0.1)
          sound.add(helper)
        },
        function (xhr) {
          console.log((xhr.loaded / xhr.total) * 100 + '% loaded')
        },
        // onError callback
        function (err) {
          console.log('An error happened')
        }
      )
      this.meshes[i].add(sound)
      this.sounds.push(sound)
      sound.add(helper)
      console.log('sound', sound)
    }
    // Fisher-Yates shuffle
    function shuffle(array) {
      for (let i = array.length - 1; i > 0; i--) {
        let j = Math.floor(Math.random() * (i + 1))
        ;[array[i], array[j]] = [array[j], array[i]]
      }
    }
    let _meshes = [...this.meshes]

    for (let i = 0; i < this.subdivs * this.subdivs; i++) {
      let audio_mesh = _meshes[i]
      audio_mesh.add(this.sounds[i])
    }
  }
  tick() {
    let _balance = [0, 0]
    let globDivider = 1
    this.meshesPos.forEach((pos, i) => {
      if (this._PARAMS.posValues.value[i] > 0) {
        _balance[0] += pos[0] * this._PARAMS.posValues.value[i] * 10
        _balance[1] += pos[1] * this._PARAMS.posValues.value[i] * 10
        globDivider += this._PARAMS.posValues.value[i] * 10
      }
    })
    let _x = (_balance[0] /= globDivider)
    let _y = (_balance[1] /= globDivider)
    if (!this.PARAMS.ignoreOsc) {
      this._mesh.position.x = _x
      this._mesh.position.z = _y
      console.log(_x, _y)
      this._mesh.rotation.y = (this._PARAMS.angleZ.value * 2 * Math.PI) / 360
    }
    window.requestAnimationFrame(this.tick.bind(this))
  }
}
