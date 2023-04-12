#include <Wire.h> 
//#include<LiquidCrystal_I2C.h>
#include <LiquidCrystal_PCF8574.h>
#include <MPU6050_tockn.h>

#define NORMAL_STATE LOW
#define PRESSED_STATE HIGH

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

#define ROT_CLK 9
#define ROT_DT 10
#define ROT_SW 11

LiquidCrystal_PCF8574 lcd(0X27);  //SCL A5 SDA A4
MPU6050 mpu6050(Wire);

int mode = 1;
float length;   //stored distance (mode 2), path (mode 3) and no of revolution (mode 5)
float offset = 0.0f;
long duration;

float x,y, Angle;
float x_error = -0.6f, y_error = -3.5f;

int prevMode = 0;
int prev_length = 0;

int lcdCount = 0;
int hcrState = HCR_INIT;

uint32_t hcrTimer = 0;
uint32_t lcdTimer = 0;

bool buttonState = LOW;

int lcdDelay = 500;

int rot_state, rot_last_state, rot_pos = 0;
long rev_time = 0, ROT_SW_time = 0;

void lcdUpdate(float x, float y, float Angle, float length, int mode){
  lcd.clear();
  lcd.setCursor(0, 0);
  if(mode < 6) lcd.print("Mode " + String(mode));
  lcd.setCursor(0, 1);
  switch(mode){
    case 1:
      lcd.print("X:" + String(x+x_error) + " Y:"+String(y+y_error));
      break;
    case 2:
      lcd.print("Distance:" + String(length));
      break;
    case 3:
      lcd.print("Curved path:" + String(length));
      break;
    case 4:
      lcd.print("Angle:" + String(1.2f*Angle));
      break;
    case 5:
      lcd.print("No of rev:" + String(length));
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
		keyReg0[index] = !digitalRead(Button[index]);
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
    mode = 1;
    lcd.begin(16, 2);
    lcd.setBacklight(255);
    pinMode(TRIGGER_PIN,OUTPUT);   // chân trig sẽ phát tín hiệu
    pinMode(ECHO_PIN,INPUT);
    pinMode(LAZER_PIN, OUTPUT);
    pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(ROT_DT, INPUT);
    pinMode(ROT_SW, INPUT);
    pinMode(ROT_CLK, INPUT);
    //lastButtonState = digitalRead(MODE_BUTTON_PIN);
    digitalWrite(TRIGGER_PIN,0);
    mpu6050.begin();
    mpu6050.calcGyroOffsets(true);
    setTimer(0, 10);
    rot_last_state = digitalRead(ROT_CLK);
    rev_time = millis();
  // put your setup code here, to run once:
}

void loop() {
    // if(Serial.available()){
    //    mode = Serial.parseInt();
    // }
    switch(mode){
      case 1:
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
        duration = pulseIn(ECHO_PIN, HIGH);
        length = (float)duration * 0.017f;
        if(isButtonPressed(0) == 1) mode = 3;
        break;
      }
      case 3: //  not tested with wheel yet
        if(digitalRead(ROT_SW) == LOW) {
          if(millis() - ROT_SW_time > 25) {
            rot_pos = 0;
            length = 0;
          }
          ROT_SW_time = millis();
        }
        rot_state = digitalRead(ROT_CLK);
        if(rot_state != rot_last_state && rot_state == HIGH) {
          if(digitalRead(ROT_DT) != rot_state) rot_pos -= 1;
          else rot_pos += 1;
        }
        rot_last_state = rot_state;
        length = (float)rot_pos * 0.19949f; // cm
        if(isButtonPressed(0) == 1) mode = 4;
        break;
      case 4: //  todo: angle still give weird value
        //hcrState = HCR_INIT;
        mpu6050.update();
        Angle = mpu6050.getAngleZ();
        if(isButtonPressed(0) == 1) mode = 5;
        break;
      case 5: //  not tested with wheel yet
        if(digitalRead(ROT_SW) == LOW) {
          if(millis() - ROT_SW_time > 25) {
            rot_pos = 0;
            rev_time = millis();  //  take the current time again
          }
          ROT_SW_time = millis();
        }
        rot_state = digitalRead(ROT_CLK);
        if(rot_state != rot_last_state && rot_state == HIGH) {
          if(digitalRead(ROT_DT) != rot_state) rot_pos -= 1;
          else rot_pos += 1;
        }
        rot_last_state = rot_state;
        if(millis() - rev_time >= 1000) {
          length = rot_pos / 20;
          length *= 60;
        }
        if(isButtonPressed(0) == 1) mode = 6;
        break;
      case 6:
        digitalWrite(LAZER_PIN, ON);
        if(isButtonPressed(0) == 1) {
          digitalWrite(LAZER_PIN, OFF);
          mode = 1;
        }
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
