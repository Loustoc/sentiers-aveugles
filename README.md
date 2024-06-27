# Sentiers Aveugles <span style="font-size:0.5em;">(Blind Paths)</span>

Final Year Project / POC of my 2nd Year at Gobelins - Paris. <br/>
**Theme :** `The 5 Senses and the 4 Elements`. <br/>

---

A defined area is subdivided in a (by default 3x3) grid, each subdivision then having a random sound mapped onto it. <br/>
The user's position in that space is calculated by weighting out the number of points in each subdivision, by depth and position on the X axis.
The gyroscope allows for the app to take full advantage of the Web Audio Spatialization API.

**1. Building the Kinect App:** <br/>

```
cd ./kinect
cmake .
cmake --build .
```

**2. Building and running the Electron app:** <br/>

```
cd ../app
npm i
npm run start
```

You should then see (by default) 9 spheres in a grid, along with a Sphere Mesh. The red section of the latter represents the eyes of the user.<br/><br/>
**3. Setting up and Connecting the M5StickC-Plus:** <br/><br/>
You'll need the following libs :

- [EasyButton](https:/github.com/evert-arias/EasyButton)
- [OSC](https:/github.com/CNMAT/OSC)
- [M5StickCPlus](https:/www.github.com/m5stack/M5StickC-Plus.git) <br/>

Keep in mind you'll need to add the M5Stack boards to your board manager, by using the following link :
[Boards JSON](https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json)
<br/><br/>
After uploading the firmware to the board, you'll need to connect it to **the same WiFi network as the computer the apps run on**.
<br/>
To do this, install the BlueFruit Connect app: [Android](https:/play.google.com/store/apps/details?id=com.adafruit.bluefruit.le.connect) / [iOS](https:/apps.apple.com/us/app/bluefruit-connect/id830125974).<br/><br/>
Then, connect to the M5Stick. It should be called "SentiersAveugles-Gyro-XX".
You can now send your network info in that order (Check the M5Stick's screen for any error) :

- SSID
- PWD
- Local IP Address of the computer (you can scan the QRCode on the electron app to get it)
- OSC Port (default is 9419)
  <br/><br/>

**4. Calibration:** <br/>

- Press `E`
  on the Kinect App to calibrate the depth. This will allow the program to detect pixels that have changed depth (thus if something moved at that specific position), compared to a reference frame.
- After ensuring the red section on the Sphere Mesh moves when rotating the gyroscope along the Z axis, click on the `A` button when the red section faces the left side. Then, strap the gyroscope onto the headphones, face the kinect, and press the `A` button again. You're now all set !

## Additionnal Information

I'm using the M5StickC-Plus solely for the fact that it packs a gyroscope/accelerometer, WiFi, Bluetooth and a battery, without the need to plug anything else in. Since it's based off a widely used chip, the ESP32, and that I'm not using any specific chip reliant functionnality, it should be fairly easy to adapt.

## Shortcuts

- Kinect App
  - `E` : Calibration
  - `O` and `P`: Decrease / Increase Depth Threshold
  - `W` and `S`: Zoom in/out
  - `V`: Reset Calibration and deactivate OSC connection
- M5StickC-Plus
  - `A`(Short-Press): Toggle OSC connection
  - `A`(Long-Press): Reset Conections (Both OSC and WiFi, Bluetooth will remain)
  - `B`(Long-Press): Turn ON/OFF
