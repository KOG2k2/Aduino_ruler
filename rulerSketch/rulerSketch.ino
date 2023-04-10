#include <Wire.h> 
//#include<LiquidCrystal_I2C.h>
#include <LiquidCrystal_PCF8574.h>
#include <MPU6050_tockn.h>

#define NORMAL_STATE HIGH
#define PRESSED_STATE LOW

#define NUM_OF_BUTTON 1

#define ECHO_PIN 7
#define TRIGGER_PIN 8
#define MODE_BUTTON_PIN 2
#define LAZER_PIN 13

#define ON HIGH
#define OFF LOW

#define HCR_INIT 0
#define HCR_EMIT 1
#define HCR_DONE 2

LiquidCrystal_PCF8574 lcd(0X27);  //SCL A5 SDA A4
MPU6050 mpu6050(Wire);

int mode = 1;
float length;
long duration;

float x,y, Angle;

int prevMode = 0;
int prev_length = 0;

int lcdCount = 0;
int hcrState = HCR_INIT;

uint32_t hcrTimer = 0;
uint32_t lcdTimer = 0;

bool buttonState = LOW;

int lcdDelay = 500;

void lcdUpdate(float x, float y, float Angle, float length, int mode){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode " + String(mode));
  lcd.setCursor(0,1);
  switch(mode){
    case 1:
      lcd.print("X:" + String(x) + " Y:"+String(y));
      break;
    case 2:
      lcd.print("Distance:" + String(length));
      break;
    case 3:
      lcd.print("Angle:" + String(Angle) + " deg");
      break;
    case 5:
      lcd.print("Lazer ON");
      break;
  }

}

//SoftwareTimer-----------------------------------------
int timerArray[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool timerArrayFlag[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void timerRun()
{
  for (int i = 0; i < 10; i++)
  {
    if (timerArray[i] > 0)
    {
      timerArray[i]--;
      if (timerArray[i] == 0)
      {
        timerArrayFlag[i] = 1;
      }
    }
  }
}

bool isTimerTimeout(int index)
{
  if (timerArrayFlag[index] == 1) 
  {
    timerArrayFlag[index] = 0;
    return true;
  }
  return false;
}

void setTimer(int index, int dur)
{
  if (dur > 0 && index >=0)
  {
    timerArray[index] = dur / 10;
    timerArrayFlag[index] = 0;
  }
}
//------------------------------------------------------

//Button Debounce---------------------------------------
int keyReg0[NUM_OF_BUTTON] = {NORMAL_STATE};
int keyReg1[NUM_OF_BUTTON] = {NORMAL_STATE};
int keyReg2[NUM_OF_BUTTON] = {NORMAL_STATE};
int keyReg3[NUM_OF_BUTTON] = {NORMAL_STATE};

int button_flag[NUM_OF_BUTTON] = {0};

int Button[NUM_OF_BUTTON] = {MODE_BUTTON_PIN};

int isButtonPressed(int index){
  if(button_flag[index] == 1){
    button_flag[index] = 0;
    return 1;
  }
  return 0;
}

void subKeyProcess(int index){
  button_flag[index] = 1;
}

void getKeyInput(){
  for(int index = 0; index < NUM_OF_BUTTON; index++){
		keyReg2[index] = keyReg1[index];
		keyReg1[index] = keyReg0[index];
		keyReg0[index] = digitalRead(Button[index]);
		if ((keyReg1[index] == keyReg0[index]) && (keyReg1[index] == keyReg2[index])){
			if (keyReg2[index] != keyReg3[index]){
				keyReg3[index] = keyReg2[index];

				if (keyReg3[index] == PRESSED_STATE) subKeyProcess(index);
			}
		}
	}
}

//------------------------------------------------------

void setup() {
    lcd.begin(16, 2);
    lcd.setBacklight(255);
    pinMode(TRIGGER_PIN,OUTPUT);   // chân trig sẽ phát tín hiệu
    pinMode(ECHO_PIN,INPUT);
    pinMode(LAZER_PIN, OUTPUT);
    pinMode(MODE_BUTTON_PIN, INPUT);
    //lastButtonState = digitalRead(MODE_BUTTON_PIN);
    digitalWrite(TRIGGER_PIN,0);
    Serial.begin(9600);
    Wire.begin();
    mpu6050.begin();
    mpu6050.calcGyroOffsets(true);
    setTimer(0, 10);
  // put your setup code here, to run once:
}

void loop() {
    // if(Serial.available()){
    //    mode = Serial.parseInt();
    // }
    switch(mode){
      case 1:
        digitalWrite(LAZER_PIN, OFF);
        mpu6050.update();
        x = mpu6050.getAngleX();
        y = mpu6050.getAngleY();
        if(isButtonPressed(0) == 1) mode = 2;
        break;
      case 2: {
        /*switch(hcrState){
          case HCR_INIT:
            hcrTimer = micros();
            digitalWrite(TRIGGER_PIN,0);
            hcrState = HCR_EMIT;
            break;
          case HCR_EMIT:
            if(micros() - hcrTimer >= 2){
              digitalWrite(TRIGGER_PIN,1);
              hcrTimer = micros();
              hcrState = HCR_DONE;
            }
            break;
          case HCR_DONE:
            if(micros() - hcrTimer >= 10){
              digitalWrite(TRIGGER_PIN,0);   // tắt chân trig
              duration = pulseIn(ECHO_PIN,HIGH); 
              length = int((float)(duration/2)/29.412);
              hcrState = HCR_INIT;
            }
            break;
          break;*/
            int hcrTimer = micros();
            digitalWrite(TRIGGER_PIN, 0);
            if(micros() - hcrTimer >= 2) digitalWrite(TRIGGER_PIN, HIGH);
            if(micros() - hcrTimer >= 12) digitalWrite(TRIGGER_PIN, LOW);   //  pull the trigger low after 10us
            duration = pulseIn(ECHO_PIN, HIHH);
            length = (float)duration * 0.017f;
            break;
      }
      case 3:
        hcrState = HCR_INIT;
        //todo: mode 3,4
        mpu6050.update();
        Angle = mpu6050.getAngleZ()/2;
        break;
      case 4:
        break;
      case 5:
        digitalWrite(LAZER_PIN, ON);
        if(isButtonPressed(0) == 1) mode = 1;
        break;
      }
    if(isTimerTimeout(0)){
      lcdUpdate(x,y,Angle,length,mode);  
      setTimer(0, 500);
    }
  getKeyInput();
  timerRun();
  delay(10);
}
