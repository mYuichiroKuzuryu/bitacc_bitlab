// please set modes set 1-2
// #1 = slop mode and taptap mode
// #2 = revolve mode and E=mgh mode
#define ACCELEBIT_MODES_SET 2

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;
//MPU6050 accelgyro(0x69); // <-- use for AD0 high

int16_t ax, ay, az;
int16_t gx, gy, gz;

int16_t pre_ax, pre_ay, pre_az;
int16_t pre_gx, pre_gy, pre_gz;

#include "motion.h"

#define ANALOG_OUT_PIN PIN_C6
#define SWITCH_PIN PIN_B3

#if ACCELEBIT_MODES_SET == 1  // slope mode and taptap mode
/////////////////////////////////////////////
////        slope mode
/////////////////////////////////////////////
#define SHAKE_MODE HIGH
  

/////////////////////////////////////////////
////        taptap mode
/////////////////////////////////////////////
#define TAPTAP_MODE LOW


#endif

// uncomment "OUTPUT_READABLE_ACCELGYRO" if you want to see a tab-separated
// list of the accel X/Y/Z and then gyro X/Y/Z values in decimal. Easy to read,
// not so easy to parse, and slow(er) over UART.
//#define OUTPUT_READABLE_ACCELGYRO

// uncomment "OUTPUT_BINARY_ACCELGYRO" to send all 6 axes of data as 16-bit
// binary, one right after the other. This is very fast (as fast as possible
// without compression or data loss), and easy to parse, but impossible to read
// for a human.
//#define OUTPUT_BINARY_ACCELGYRO


#define LED_PIN 13
bool blinkState = false;

#define THD 8000

#define MINIMUM_DECAY 100 //msec : no sound after over THD


#if ACCELEBIT_MODES_SET == 2  // revolve mode and height mode
/////////////////////////////////////////////
////        revolve mode
/////////////////////////////////////////////
int16_t revolve_default_val = 0;
int16_t revolve_sum_val = 0;

#define REVOLVE_MAX 10000
#define REVOLVE_MIN 0

#define REVOLVE_MODE HIGH

/////////////////////////////////////////////
////        height mode
/////////////////////////////////////////////
int16_t height_default_val = 0;
int16_t height_sum_val = 0;

#define HEIGHT_MAX 10000
#define HEIGHT_MIN 0

#define HEIGHT_MODE LOW

#endif


void setup() {
  // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif

    // initialize serial communication
    // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
    // it's really up to you depending on your project)
//    Serial.begin(38400);
    Serial.begin(9600);
    
    delay(3000);
    
    // initialize device
    Serial.println("Initializing I2C devices...");
    accelgyro.initialize();

    // verify connection
    Serial.println("Testing device connections...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

    // configure Arduino LED for
    pinMode(LED_PIN, OUTPUT);
    
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    pre_ax = ax;
    pre_ay = ay;    
    pre_az = az;
    pre_gx = gx;
    pre_gy = gy;    
    pre_gz = gz;
    
//    double vec[3] = {0.0, 0.0, 0.0};
    double vec[3];
    

    digitalWrite(LED_PIN, LOW);
    
    for(int j = 0; j < CALIB_VEC_TIMES; j++){
      accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
      get_cos_two_vector((double*)vec, ax, ay, az, pre_ax, pre_ay, pre_az);
      
      calib_vec_length += vec[1];

      pre_ax = ax;
      pre_ay = ay;    
      pre_az = az;
      
      delay(CALIB_VEC_DELAY);
    }

    digitalWrite(LED_PIN, HIGH);    
    
    calib_vec_length /= CALIB_VEC_TIMES;
    
    Serial.print("CALIBRATED VECTOR LENGTH = "); Serial.println(calib_vec_length);


  revolve_default_val = gx; 
  
  height_default_val = az;

  pinMode(ANALOG_OUT_PIN, OUTPUT);
  analogWrite(ANALOG_OUT_PIN, 128);
  
  pinMode(SWITCH_PIN, INPUT);
  
}

void loop() {

#if ACCELEBIT_MODES_SET == 1  // slope mode and taptap mode
/////////////////////////////////////////////
////        slope mode
/////////////////////////////////////////////
  #define SLOPE_MAX 17000
  #define ANALOG_OUT_MAX 255
  #define AVERAGE_NUM 10
  #define AVERAGE_DELAY 0
  
  double sum_ax = 0;
  double sum_ay = 0;
  double sum_az = 0;
  double sum_gx = 0;
  double sum_gy = 0;
  double sum_gz = 0;
  
  for(int i = 0; i < AVERAGE_NUM; i++){
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    sum_ax += ax;
    sum_ay += ay;
    sum_az += az;
    sum_gx += gx;
    sum_gy += gy;
    sum_gz += gz;
  }
    
  double slope_x = ((double)sum_ax/AVERAGE_NUM + SLOPE_MAX)/2/SLOPE_MAX * ANALOG_OUT_MAX;
  Serial.print("slope_x: "); Serial.print(slope_x); Serial.print("("); Serial.println(ax);
  analogWrite(ANALOG_OUT_PIN, (int)slope_x);

/////////////////////////////////////////////
////        shake mode
/////////////////////////////////////////////


  double sum_power = 0;
  #define DECREASE 100
  #define POWER_MAX 10000
  
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  pre_ax = ax;
  pre_ay = ay;    
  pre_az = az;

  while(digitalRead(SWITCH_PIN) == SHAKE_MODE){
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    sum_power = sum_power + abs(pre_ax - ax)/100 + abs(pre_ay - ay)/100 + abs(pre_az - az)/100;

    if(sum_power > POWER_MAX){
      sum_power = POWER_MAX;
    }
    
    Serial.print("power = "); Serial.println(sum_power);
    analogWrite(ANALOG_OUT_PIN, (int)(sum_power/POWER_MAX * 255));
    
    pre_ax = ax;
    pre_ay = ay;    
    pre_az = az;
    
    
    sum_power -= DECREASE;
    if(sum_power < 0){
      sum_power = 0;
    }

  }
  
#endif


#if ACCELEBIT_MODES_SET == 2  // revolve mode and height mode
/////////////////////////////////////////////
////        revolve mode
/////////////////////////////////////////////
  if(digitalRead(SWITCH_PIN) == REVOLVE_MODE){
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    revolve_sum_val += (gx - revolve_default_val);
    if(revolve_sum_val < REVOLVE_MIN){revolve_sum_val = REVOLVE_MIN;}
    if(revolve_sum_val > REVOLVE_MAX){revolve_sum_val = REVOLVE_MAX;}
    
    analogWrite(ANALOG_OUT_PIN, (int)((double)revolve_sum_val/(REVOLVE_MAX - REVOLVE_MIN) * 255));
  }

/////////////////////////////////////////////
////        height mode
/////////////////////////////////////////////
  if(digitalRead(SWITCH_PIN) == HEIGHT_MODE){
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    revolve_sum_val += (az - height_default_val);
    if(height_sum_val < HEIGHT_MIN){height_sum_val = HEIGHT_MIN;}
    if(height_sum_val > HEIGHT_MAX){height_sum_val = HEIGHT_MAX;}
    
    analogWrite(ANALOG_OUT_PIN, (int)((double)height_sum_val/(HEIGHT_MAX - HEIGHT_MIN) * 255));
  }

#endif


    // these methods (and a few others) are also available
    //accelgyro.getAcceleration(&ax, &ay, &az);
    //accelgyro.getRotation(&gx, &gy, &gz);

    #ifdef OUTPUT_READABLE_ACCELGYRO
        // display tab-separated accel/gyro x/y/z values
        Serial.print("a/g:\t");
        Serial.print(ax); Serial.print("\t");
        Serial.print(ay); Serial.print("\t");
        Serial.print(az); Serial.print("\t");
        Serial.print(gx); Serial.print("\t");
        Serial.print(gy); Serial.print("\t");
        Serial.println(gz);
    #endif

    #ifdef OUTPUT_BINARY_ACCELGYRO
        Serial.write((uint8_t)(ax >> 8)); Serial.write((uint8_t)(ax & 0xFF));
        Serial.write((uint8_t)(ay >> 8)); Serial.write((uint8_t)(ay & 0xFF));
        Serial.write((uint8_t)(az >> 8)); Serial.write((uint8_t)(az & 0xFF));
        Serial.write((uint8_t)(gx >> 8)); Serial.write((uint8_t)(gx & 0xFF));
        Serial.write((uint8_t)(gy >> 8)); Serial.write((uint8_t)(gy & 0xFF));
        Serial.write((uint8_t)(gz >> 8)); Serial.write((uint8_t)(gz & 0xFF));
    #endif

    // blink LED to indicate activity
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
    
}

int16_t diff_acc(int16_t x, int16_t y, int16_t z)
{
  int16_t val;

  long diff_ax = x - pre_ax;
  long diff_ay = y - pre_ay;
  long diff_az = z - pre_az;

  val = diff_ax * diff_ax + diff_ay * diff_ay + diff_az * diff_az; 
  
  pre_ax = x;
  pre_ay = y;    
  pre_az = z;
  pre_gx = gx;
  pre_gy = gy;    
  pre_gz = gz;  
  
  return val;
}

int16_t diff_gyro(int16_t x, int16_t y, int16_t z)
{
  int16_t val;

  int16_t diff_gx = abs(x - pre_gx);
  int16_t diff_gy = abs(y - pre_gy);
  int16_t diff_gz = abs(z - pre_gz);

//  val = diff_gx * diff_gx + diff_gy * diff_gy + diff_gz * diff_gz; 
  val = diff_gx + diff_gy + diff_gz;
    
  pre_gx = x;
  pre_gy = y;    
  pre_gz = z;  
  
  return val;  
  
}


