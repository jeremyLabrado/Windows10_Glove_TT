0) Setting the environment

0.0) Download the folder from https://github.com/jeremyLabrado/Windows10_Glove_TT 

0.1) Unzip it


1) Only first run or if you need to change the glove specs (number of channels, IMU, Frequency ...). 

1.0) Connect the TinyTILE to the PC with the USB mini cable

1.1) Open Arduino/Arduino.ino (if you don't have the Arduini IDE, you need to download and install it)

1.2) Select the good COM port in the Tools/Port tabs, you should see something like COM5(Arduino/Genuino101)

1.3) Upload the code into the TinyTILE by either clicking on the Upload button of Sketch/Upload

1.4) On the terminal, wait until seeing "SUCCESS: Sketch will execute in about 5 seconds."

1.5) You can check if all the sensors of the glove are working by opening the serial monitor from the tab Tools/Serial Monitor

1.6) Close the Arduino IDE


2) Only first-run - Pair the glove to the PC using the Bluetooth manager

2.0) Power either the glove with battery or USB cable

2.0) Open the windows settings

2.1) Open the Bluetooth & other devices settings

2.2) Click on the "+" to add a Bluetooth or other device

2.3) Click on the "Bluetooth" to add a Bluetooth device

2.4) Wait until you have "StretchSense_glove-0x" and click on it to pair and connect it

2.5) Close all the windows


3)  Play with the StretchSense Glove App

3.0) Open the .exe file Windows10_GloveTT-master\Unity\standalone.exe

3.1) Adapt the settings to your screen and press Play!

3.2) Click on "Add"  the glove you want to connect (0 by default)

3.3) Click on "Connect", the communication should be instantaneous. A blue circle next to the Reset button indicate the BLE connection.

3.4) Squeeze your hand a few times to do the calibration, it automatically detects the limits. to reset the calibration, press Reset

3.5) With the mouse, you can orbit the glove

3.6) Up to 4 gloves can be connected at the same time
