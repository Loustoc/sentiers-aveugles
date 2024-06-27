varying vec2 vUv;

// uniforms
uniform float posValues[9];
uniform float subdivs;
uniform float aspectRatio;
uniform float time;

void main()
{
    vec2 uv = vUv;
    float ratioX = floor(uv.x / (1./subdivs)) >= subdivs ? subdivs - 1. : floor(uv.x / (1./subdivs));
    float ratioY = floor(uv.y / (1./subdivs)) >= subdivs ? subdivs - 1. : floor(uv.y / (1./subdivs));
    gl_FragColor = vec4(1., 1., 1.,1.);
    gl_FragColor = mix(gl_FragColor, vec4(1., 0., 0., 1.), posValues[int(((2. - ratioY) * subdivs + ratioX))]);
}

