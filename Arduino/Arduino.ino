#include <CurieIMU.h>
#include <MadgwickAHRS.h>
#include <CurieBLE.h>
#include <SPI.h>
#include <SD.h>

//HARDWARE SPECIFICATIONS
//frequency in Hz for updates to the BLE, IMU, and SD
#define   FREQUENCY_BLE_NEW_SAMPLE      12.5
//allowed values for IMU frequency are 12.5,25,50,100,200,400,800,1600
#define   FREQUENCY_IMU_NEW_SAMPLE      25
#define   FREQUENCY_SD_NEW_SAMPLE       40
//number of channels on the SPI circuit
#define   CHANNELS                      10
//flags to include certain sections of code
//6DoF inertial motion unit using Madgwick algorithm
//#define   ENABLE_IMU
//Bluetooth Low-Energy communication for interface with the C++ library
#define   ENABLE_BLE
//logging to Secure Digital card
//#define   ENABLE_SD
//reading StretchSense Serial Peripheral Interface circuit
#define   ENABLE_SSL_SPI
//printing to Arduino's serial monitor
#define   ENABLE_SERIAL_PRINT

// SD declarations
#ifdef ENABLE_SD
//timing
unsigned long microsPrevious_SD, microsPerReading_SD;
File myFile;
const int chipSelectPin_SD = 5;
String SD_FILE_NAME = "ssl_all.csv";
#endif

// BLE declarations
#ifdef ENABLE_BLE
//timing
unsigned long microsPrevious_BLE, microsPerReading_BLE;
BLEPeripheral blePeripheral;
//BLE service
BLEService stretchSenseService;
//BLE characteristics
BLECharacteristic stretchSenseChar;
BLECharacteristic imuChar;
#endif

// IMU declarations
#ifdef ENABLE_IMU
//timing
unsigned long microsPrevious_IMU, microsPerReading_IMU;
//madgwick
Madgwick filter;
float accelScale, gyroScale;
int RawDataIMU[20];
#endif

// SSL_SPI declarations
#ifdef ENABLE_SSL_SPI
const int InterruptPin   =    6;
const int chipSelectPin_SSL  =    10;

// ---- DEFINITIONS ----//

// Data package options
#define DATA    0x00  // 16FGV1.0 data packet
#define CONFIG  0x01  // 16FGV1.0 configuration command

// ODR - Output Data Rate
#define RATE_OFF             0x00
#define RATE_25HZ            0x01
#define RATE_50HZ            0x02
#define RATE_100HZ           0x03
#define RATE_166HZ           0x04
#define RATE_200HZ           0x05
#define RATE_250HZ           0x06
#define RATE_500HZ           0x07
#define RATE_1kHZ            0x08
#define RATE_2kHZ            0x09

// INT - Interrupt Mode
#define INTERRUPT_DISABLED    0x00
#define INTERRUPT_ENABLED     0x01

// TRG - Trigger Mode
#define TRIGGER_DISABLED     0x00
#define TRIGGER_ENABLED      0x01

// FILTER - Filter Mode
#define FILTER_1PT           0x01
#define FILTER_2PT           0x02
#define FILTER_4PT           0x04
#define FILTER_8PT           0x08
#define FILTER_16PT          0x10
#define FILTER_32PT          0x20
#define FILTER_64PT          0x40
#define FILTER_128PT         0x80
#define FILTER_255PT         0xFF

// RES - Resolution Mode
#define RESOLUTION_1pF       0x00 // 0 - 65535pf
#define RESOLUTION_100fF     0x01 // 0 - 6553.5pf
#define RESOLUTION_10fF      0x02 // 0 - 655.35pf
#define RESOLUTION_1fF       0x03 // 0 - 65.535pf

// Config Transfer
#define PADDING              0x00
/////////////////////////////////////////////////////////////////////
// Configuration Setup
// MODIFY THESE PARAMETERS TO CHANGE CIRCUIT FUNCTION
int   ODR_MODE                  =       RATE_500HZ;
int   INTERRUPT_MODE            =       INTERRUPT_DISABLED;
int   TRIGGER_MODE              =       TRIGGER_DISABLED;
int   FILTER_MODE               =       FILTER_32PT;
int   RESOLUTION_MODE           =       RESOLUTION_100fF;

//////////////////////////////////////////////////////////////////////


//SPI Configuration
SPISettings SPI_settings(2000000, MSBFIRST, SPI_MODE1);

//Default scaling factor
int   CapacitanceScalingFactor = 100; //Default value
int   RawDataCapacitance[20];
#endif

/// setup
/// <summary>Initializes the Arduino program</summary>
void setup() {
  Serial.begin(9600);
  //Serial.begin(115200);

  // Initialise the BLE ////////////////////////////////////////
#ifdef ENABLE_BLE
  microsPrevious_BLE = micros();
  microsPerReading_BLE = 1000000 / FREQUENCY_BLE_NEW_SAMPLE; //12.5 Hz

  Serial.print("b");
  blePeripheral.setLocalName("StretchSense_glove01"); //broadcast device name
  blePeripheral.setDeviceName("StretchSense_glove01");

  //UUID SPECIFICATIONS
  //first three digits: IMU (000 = none, 006 = 6DoF)
  //second three digits: sensor count (010 = 10-sensor, 005 = 5-sensor)
#ifdef ENABLE_IMU
  String imuString = "006";
#else
  String imuString = "000";
#endif
  String chanString = String(CHANNELS);
  while (chanString.length() < 3)
  {
    chanString = "0" + chanString;
  }
  String UUIDservice = imuString + chanString + "01" + "-7374-7265-7563-6873656e7365";
  stretchSenseService = BLEService(UUIDservice.c_str());
  stretchSenseChar = BLECharacteristic((imuString + chanString + "02" + "-7374-7265-7563-6873656e7365").c_str(), BLERead | BLENotify, CHANNELS * 2);
  imuChar = BLECharacteristic((imuString + chanString + "50" + "-7374-7265-7563-6873656e7365").c_str(), BLERead | BLENotify, 20);

  blePeripheral.setAdvertisedServiceUuid(stretchSenseService.uuid());  // add the service UUID
  blePeripheral.addAttribute(stretchSenseService);   // add your custom service
#ifdef ENABLE_SSL_SPI
  blePeripheral.addAttribute(stretchSenseChar); // add your custom characteristic
#endif
#ifdef ENABLE_IMU
  blePeripheral.addAttribute(imuChar); // add your custom characteristic
#endif

  blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);


  blePeripheral.begin();


  Serial.println("Bluetooth device active, waiting for connections...");
#endif

  //Initialise the IMU ////////////////////////////////////////
#ifdef ENABLE_IMU
  microsPrevious_IMU = micros();
  microsPerReading_IMU = 1000000 / FREQUENCY_IMU_NEW_SAMPLE;
  // start the IMU and filter
  CurieIMU.begin();
  CurieIMU.setGyroRate(FREQUENCY_IMU_NEW_SAMPLE);
  CurieIMU.setAccelerometerRate(FREQUENCY_IMU_NEW_SAMPLE);
  filter.begin(FREQUENCY_IMU_NEW_SAMPLE);

  // Set the accelerometer range to 2G
  CurieIMU.setAccelerometerRange(2);
  // Set the gyroscope range to 250 degrees/second
  CurieIMU.setGyroRange(250);

  CurieIMU.autoCalibrateGyroOffset();
  CurieIMU.autoCalibrateAccelerometerOffset(X_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Y_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Z_AXIS, 1);

  // give the circuit time to set up:
  delay(0.1);
#endif

  // Initialise the SD card  ///////////////////////////////////
#ifdef ENABLE_SD
  microsPrevious_SD = micros();

  microsPerReading_SD = 1000000 / FREQUENCY_SD_NEW_SAMPLE; //12.5 Hz

  pinMode(chipSelectPin_SD, OUTPUT);

  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);


  // if (!SD.begin(10)) {
  if (!SD.begin(chipSelectPin_SD)) {
    Serial.println("sd initialization failed!");
    return;
  }
  Serial.println("sd initialization done.");
  writeInitialiseSdCard();
  // give the circuit time to set up
  delay(0.1);
#endif

  // Initialise the SPI //////////////////////////////////////
#ifdef ENABLE_SSL_SPI
  //Initialise SPI port
  SPI.begin();
  SPI.beginTransaction(SPI_settings);

  // Initalize the  data ready and chip select pins:
  pinMode(InterruptPin, INPUT);
  pinMode(chipSelectPin_SSL, OUTPUT);

  //Configure 16FGV1.0:
  writeConfiguration();
  //Get capacitance scaling factor
  CapacitanceScalingFactor = getCapacitanceScalingFactor(RESOLUTION_MODE);

  // give the circuit time to set up:
  delay(0.1);
#endif

  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  delay(0.1);
}

/// loop
/// <summary>The main loop of the Arduino program</summary>
void loop() {

  int aix, aiy, aiz;
  int gix, giy, giz;
  float ax, ay, az;
  float gx, gy, gz;
  float roll, pitch, heading;
  int roll_int, pitch_int, heading_int;
  float accel_x, accel_y, accel_z;
  int accel_x_int, accel_y_int, accel_z_int;

  unsigned long microsNow;
  String str = "";
  String str2 = "";

  /////////////////////////////////////////////////////////////
  // IMU Mode /////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////
#ifdef ENABLE_IMU
  microsNow = micros();
  if (microsNow - microsPrevious_IMU >= microsPerReading_IMU) {
    // increment previous time, so we keep proper pace
    microsPrevious_IMU = microsPrevious_IMU + microsPerReading_IMU;
    // read raw data from CurieIMU
    CurieIMU.readMotionSensor(aix, aiy, aiz, gix, giy, giz);

    // convert from raw data to gravity and degrees/second units
    ax = convertRawAcceleration(aix);
    ay = convertRawAcceleration(aiy);
    az = convertRawAcceleration(aiz);
    gx = convertRawGyro(gix);
    gy = convertRawGyro(giy);
    gz = convertRawGyro(giz);

    // update the filter, which computes orientation
    filter.updateIMU(gx, gy, gz, ax, ay, az);

    // get the heading, pitch and roll
    heading = filter.getYaw();
    pitch = filter.getPitch();
    roll = filter.getRoll();

    // shift the value to be positive
    pitch = pitch + 90;
    roll = roll + 180;
    accel_x = ax + 2;
    accel_y = ay + 2;
    accel_z = az + 2;

    // shift the decimal place and cast into integer
    heading_int = heading * 100;
    pitch_int = pitch * 100;
    roll_int = roll * 100;
    accel_x_int = accel_x * 100;
    accel_y_int = accel_y * 100;
    accel_z_int = accel_z * 100;

    // separate each integer IMU into 2 headecimal values
    RawDataIMU[0] = heading_int / 256;
    RawDataIMU[1] = heading_int - ((heading_int / 256) << 8);
    RawDataIMU[2] = pitch_int / 256;
    RawDataIMU[3] = pitch_int - ((pitch_int / 256) << 8);
    RawDataIMU[4] = roll_int / 256;
    RawDataIMU[5] = roll_int - ((roll_int / 256) << 8);
    RawDataIMU[6] = accel_x_int / 256;
    RawDataIMU[7] = accel_x_int - ((accel_x_int / 256) << 8);
    RawDataIMU[8] = accel_y_int / 256;
    RawDataIMU[9] = accel_y_int - ((accel_y_int / 256) << 8);
    RawDataIMU[10] = accel_z_int / 256;
    RawDataIMU[11] = accel_z_int - ((accel_z_int / 256) << 8);
    RawDataIMU[12] = 0;
    RawDataIMU[13] = 0;
    RawDataIMU[14] = 0;
    RawDataIMU[15] = 0;
    RawDataIMU[16] = 0;
    RawDataIMU[17] = 0;
    RawDataIMU[18] = 0;
    RawDataIMU[19] = 0;
  }
#endif

  /////////////////////////////////////////////////////////////
  // StretchSense Mode ////////////////////////////////////////
  /////////////////////////////////////////////////////////////
#ifdef ENABLE_SSL_SPI

  //Check if interrupt mode is enabled (in configuration)
  if (INTERRUPT_MODE == INTERRUPT_ENABLED) {
    // don't do anything until the interupt pin goes low:
    while (digitalRead(InterruptPin) == HIGH);
  }

  //Read the sensor Data
#ifdef ENABLE_SD
  SPI.beginTransaction(SPI_settings);
#endif
  readCapacitance(RawDataCapacitance);

  //Wait for next data packet to start sampling
  if (INTERRUPT_MODE == INTERRUPT_ENABLED) {
    while (digitalRead(InterruptPin) == LOW);
  }
#endif

  /////////////////////////////////////////////////////////////
  // SD Mode //////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////
#ifdef ENABLE_SD
  // check if it's time to read data and update the filter
  microsNow = micros();
  if (microsNow - microsPrevious_SD >= microsPerReading_SD) {

    // increment previous time, so we keep proper pace
    microsPrevious_SD = microsPrevious_SD + microsPerReading_SD;
    Serial.println("Record");
    writeInSdCard();
  }
#endif

  /////////////////////////////////////////////////////////////
  // BLE Mode //////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////
#ifdef ENABLE_BLE
  // check if it's time to read data and update the filter
  microsNow = micros();
  if (microsNow - microsPrevious_BLE >= microsPerReading_BLE) {

    // increment previous time, so we keep proper pace
    microsPrevious_BLE = microsPrevious_BLE + microsPerReading_BLE;

    BLECentral central = blePeripheral.central();

#ifdef ENABLE_IMU
    if (central) { // if a central is connected to peripheral
      const unsigned char imuCharArray[20] = {
        RawDataIMU[0], RawDataIMU[1], RawDataIMU[2], RawDataIMU[3], RawDataIMU[4],
        RawDataIMU[5], RawDataIMU[6], RawDataIMU[7], RawDataIMU[8], RawDataIMU[9],
        RawDataIMU[10], RawDataIMU[11], RawDataIMU[12], RawDataIMU[13], RawDataIMU[14],
        RawDataIMU[15], RawDataIMU[16], RawDataIMU[17], RawDataIMU[18], RawDataIMU[19]
      };
      imuChar.setValue(imuCharArray, 20); //notify central with new data
    }
#endif
#ifdef ENABLE_SSL_SPI
    if (central) { // if a central is connected to peripheral
      const unsigned char capaCharArray[20] = {
        RawDataCapacitance[0], RawDataCapacitance[1], RawDataCapacitance[2], RawDataCapacitance[3], RawDataCapacitance[4],
        RawDataCapacitance[5], RawDataCapacitance[6], RawDataCapacitance[7], RawDataCapacitance[8], RawDataCapacitance[9],
        RawDataCapacitance[10], RawDataCapacitance[11], RawDataCapacitance[12], RawDataCapacitance[13], RawDataCapacitance[14],
        RawDataCapacitance[15], RawDataCapacitance[16], RawDataCapacitance[17], RawDataCapacitance[18], RawDataCapacitance[19]
      };
      stretchSenseChar.setValue(capaCharArray, 20); //notify central with new data
    }
#endif
  }
#endif


  /////////////////////////////////////////////////////////////
  // Serial print /////////////////////////////////////////////
  /////////////////////////////////////////////////////////////
#ifdef ENABLE_SERIAL_PRINT
#ifdef ENABLE_IMU
  //Convert the raw data to IMU:
  Serial.print(" Orientation: ");
  float imu_values = 0;
  for (int i = 0; i < 10; i++) {
    imu_values = extractCapacitance(RawDataIMU, i) / 100;
    Serial.print(imu_values);     //Output capacitance values
    Serial.print(", ");           //Output data as comma seperated values
  }
#endif

#ifdef ENABLE_SSL_SPI
  //Convert the raw data to capacitance:
  Serial.print(" StretchSense: ");
  float capacitance = 0;
  for (int i = 0; i < 10; i++) {
    capacitance = 10 * extractCapacitance(RawDataCapacitance, i);
    Serial.print(capacitance);  //Output capacitance values
    Serial.print(", ");          //Output data as comma seperated values
  }
#endif

  Serial.print(", ");          //Output data as comma seperated values
  Serial.print(", ");          //Output data as comma seperated values
  Serial.print(", ");          //Output data as comma seperated values
  unsigned long time = millis();
  //Serial.print(time);

  //Separate for next values
  Serial.print("\n");
#endif

}

/// convertRawAcceleration
/// <summary>Maps accelerometer values to the 2g range</summary>
/// <param name="aRaw">The raw acceleration from the accelerometer</param>
/// <returns>The converted acceleration</returns>
float convertRawAcceleration(int aRaw) {
  // since we are using 2G range
  // -2g maps to a raw value of -32768
  // +2g maps to a raw value of 32767

  float a = (aRaw * 2.0) / 32768.0;
  return a;
}

/// convertRawAcceleration
/// <summary>Maps gyroscope values to the 250deg/s range</summary>
/// <param name="gRaw">The raw angular speed from the gyroscope</param>
/// <returns>The converted angular speed</returns>
float convertRawGyro(int gRaw) {
  // since we are using 250 degrees/seconds range
  // -250 maps to a raw value of -32768
  // +250 maps to a raw value of 32767

  float g = (gRaw * 250.0) / 32768.0;
  return g;
}

/// writeConfiguration
/// <summary>Writes the specified configuration to the SPI circuit</summary>
void writeConfiguration() {

  // 16FGV1.0 requires a configuration package to start streaming data

  // Set the chip select low to select the device:
  digitalWrite(chipSelectPin_SSL, LOW);

  SPI.transfer(CONFIG);                 //  Select Config Package
  SPI.transfer(ODR_MODE);               //  Set output data rate
  SPI.transfer(INTERRUPT_MODE);         //  Set interrupt mode
  SPI.transfer(TRIGGER_MODE);           //  Set trigger mode
  SPI.transfer(FILTER_MODE);            //  Set filter
  SPI.transfer(RESOLUTION_MODE);        //  Set Resolution
  for (int i = 0; i < 16; i++) {
    SPI.transfer(PADDING);              //  Pad out the remaining configuration package
  }

  // take the chip select high to de-select:
  digitalWrite(chipSelectPin_SSL, HIGH);
}

/// readCapacitance
/// <summary>Reads capacitance values from the SPI circuit</summary>
/// <param name="raw">An array to write values from the SPI circuit</param>
void readCapacitance(int raw[]) {

  // 16FGV1.0 transmits data in the form of 10, 16bit capacitance values

  // Set the chip select low to select the device:
  digitalWrite(chipSelectPin_SSL, LOW);

  SPI.transfer(DATA);                   //  Select Data Package
  SPI.transfer(PADDING);                //  Get Sequence Number
  //Read capacitance
  for (int i = 0; i < 20; i++) {
    raw[i] =  SPI.transfer(PADDING);    //  Pad out the remaining configuration package
  }

  // take the chip select high to de-select:
  digitalWrite(chipSelectPin_SSL, HIGH);

}

/// getCapacitanceScalingFactor
/// <summary>Convert resolution to scaling factor</summary>
/// <param name="Resolution_Config">The resolution enum, in farads</param>
/// <returns>The factor to multiply the SPI value by</returns>
int getCapacitanceScalingFactor (int Resolution_Config) {

  switch (Resolution_Config) {
    case (RESOLUTION_1pF):
      return 1;
      break;

    case (RESOLUTION_100fF):
      return 10;
      break;

    case (RESOLUTION_10fF):
      return 100;
      break;

    case (RESOLUTION_1fF):
      return 1000;
      break;

  }
  return 1;

}

/// extractCapacitance
/// <summary>Reads the capacitance from a specific channel</summary>
/// <param name="raw">The capacitance values</param>
/// <param name="channel">The channel to read from</param>
/// <returns>The capacitance of the specified channel</returns>
float extractCapacitance(int raw[], int channel) {

  float Capacitance = 0;

  Capacitance = (raw[2 * channel]) * 256 + raw[2 * channel + 1];
  Capacitance /= CapacitanceScalingFactor * 10;

  return Capacitance;

}

#ifdef ENABLE_SD
/// writeInSdCard
/// <summary>Writes data to the connected SD card</summary>
void writeInSdCard() {

  // 16FGV1.0 requires a configuration package to start streaming data

  // Set the chip select low to select the device:
  digitalWrite(chipSelectPin_SD, LOW);

  //Convert the raw data to capacitance:
  float capacitance = 0;
  float imu = 0;
  unsigned long time = millis();

  myFile = SD.open(SD_FILE_NAME, FILE_WRITE);
  if (myFile) {

    myFile.print(time);
    myFile.print(", ");

#ifdef ENABLE_SSL_SPI
    for (int i = 0; i < 10; i++) {
      capacitance = extractCapacitance(RawDataCapacitance, i);
      myFile.print(capacitance);
      myFile.print(", ");
    }
#endif
#ifdef ENABLE_IMU
    for (int i = 0; i < 10; i++) {
      imu = extractCapacitance(RawDataIMU, i);
      myFile.print(imu);
      myFile.print(", ");
    }
#endif

    myFile.print("\n");
    myFile.close();
  }
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

  // take the chip select high to de-select:
  digitalWrite(chipSelectPin_SD, HIGH);
}

/// writeInitialiseSdCard
/// <summary>Sets up the SD card to be written to</summary>
void writeInitialiseSdCard() {
  digitalWrite(chipSelectPin_SD, LOW);

  myFile = SD.open(SD_FILE_NAME, FILE_WRITE);
  if (myFile) {
    myFile.print("time");
    myFile.print(", ");
#ifdef ENABLE_SSL_SPI
    for (int i = 0; i < 10; i++) {
      myFile.print("SSL ");
      myFile.print(i + 1);
      myFile.print(", ");
    }
#endif

#ifdef ENABLE_IMU
    for (int i = 0; i < 10; i++) {
      myFile.print("IMU ");
      myFile.print(i + 1);
      myFile.print(", ");
    }
#endif
    myFile.print("\n");

    myFile.close();
  }
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
  digitalWrite(chipSelectPin_SD, HIGH);

}
#endif

/// blePeripheralConnectHandler
/// <summary>Called when central connects, resets IMU</summary>
/// <param name="central">The BLE central that has been connected to</param>
void blePeripheralConnectHandler(BLECentral& central) {
  // central connected event handler
  Serial.print("\n");
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  /////////////////////////////////////////////////////

  //Initialise the IMU ////////////////////////////////////////
#ifdef ENABLE_IMU

  CurieIMU.autoCalibrateGyroOffset();
  CurieIMU.autoCalibrateAccelerometerOffset(X_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Y_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Z_AXIS, 1);

  // give the circuit time to set up:
  delay(0.1);
#endif
  /////////////////////////////////////////////////////
}

/// blePeripheralDisconnectHandler
/// <summary>Called when central disconnects</summary>
/// <param name="central">The central device that has been disconnected from</param>
void blePeripheralDisconnectHandler(BLECentral& central) {
  // central disconnected event handler
  Serial.print("\n");

  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}


