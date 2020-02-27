#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

#include <pins.h>

void mainMenu();
void speedMenu();
void slaveScreen();

void titleScreen();
void runMotor();
void fineAdjustment();
void getEncoder();
void speedCheck();
void drawControlScreen();

//sets up the lcd object
LiquidCrystal_I2C lcd(0x27, 16, 2);
//sets up the motor controller
Servo controller;
Servo controller2;

//keeps track for the encoder position and encoder state changes
int encoderValue = -1; //current encode value
int encoderPinALast = LOW; //previous pin A pulse
int n = LOW; //current pin A pulse
bool newValue = false; //encoder value change

//keeps track of the fine and course adjustment mode
bool fineAdjust = true; //state of fine adjustment
String adjustment = "Fine"; //text for the current adjustment mode

//keeps track of master and slave mode
bool slave = true;

//used to debounce the encoder button
const int buttonDelay = 225;

//version data
const float version = 1.00; //2 decimals
const String date = "Nov 2019"; //no decimal on the month

void setup() {
  //Serial.begin(115200);
  
  //sets the encoder pins to inputs
  pinMode(encoderPinA, INPUT);
  pinMode(encoderPinB, INPUT);

  //sets the button inputs with pullups
  pinMode(encoderButton, INPUT_PULLUP);
  pinMode(deadMan, INPUT_PULLUP);

  lcd.init(); //initializes the lcd object
  lcd.backlight(); //turns on the backlight

  //runs the title screen
  titleScreen();
}

void loop() {
  mainMenu();
}

//draws the title screen before starting the program
void titleScreen() {
  lcd.clear();
  //draws the team name
  lcd.setCursor(1, 0);
  lcd.print("4488 Shockwave");
  //draws the device name
  lcd.setCursor(0, 1);
  lcd.print("Speed Controller");
  //keeps everything on the display long enough to read then clears it
  delay(2500);
  lcd.clear();

  //draws the version number
  lcd.setCursor(4, 0);
  lcd.print("Ver ");
  lcd.print(version, 2);
  //draws the date of the version release
  lcd.setCursor(4, 1);
  lcd.print(date);

  //keeps everything on the display long enough to read then clears it
  delay(1500);
  lcd.clear();
}

//checks for encoder change
void getEncoder() {
  //stores the current encoder pin A value
  n = digitalRead(encoderPinA);
  
  //checks if the encoder changed position
  if ((encoderPinALast == LOW) && (n == HIGH)) {
    //checks the state of pin B
    //if low, the encoder turned CCW
    //if high, the encoder turned CW
    if (digitalRead(encoderPinB) == LOW) {
      if(fineAdjust == true) {
        encoderValue--;
      }
      else {
        encoderValue -= 10;
      }
    } 
    else {
      if(fineAdjust == true) {
        encoderValue++;
      }
      else {
        encoderValue += 10;
      }
    }
    //sets the new value flag for drawing use
    newValue = true;
  }

  //staores the new pin A value for comparison to the next encoder state check
  encoderPinALast = n;
}

//toggles fine and course adjustment
void fineAdjustment() {
  //checks if the encoder button was pressed
  if(digitalRead(encoderButton) == LOW) {
    //debounces the button
    delay(buttonDelay);

    //sets the draw position to the begining of the current adjustment mode
    lcd.setCursor(5, 1);
    
    //inverts the adjustment mode (i.e. course to fine and fine to course)
    if(fineAdjust == false) {
      fineAdjust = true;
      adjustment = "Fine";

      //prints the new adjstment mode
      lcd.print(adjustment);
      lcd.print("  ");
    }
    else {
      fineAdjust = false;
      adjustment = "Course";
      
      //prints the new adjstment mode
      lcd.print(adjustment);
    }
  }
}

//limits the range of the encoder adjustment from -100 to 100 and jumps over -10 to 10 because 
void speedCheck() {
  //keeps the motor speed under 101%
  if(encoderValue > 100) {
    encoderValue = 100;
  }

  //keeps the motor speed above 101%
  if(encoderValue < -100) {
    encoderValue = -100;
  }
}

//draws the speed if a change was made
void drawControlScreen() {
    //draws the speed text
    lcd.setCursor(0, 0);
    lcd.print("Speed: ");
    //clears the current speed value
    lcd.print("     ");
    //draws the enocder value with `%` at the end
    lcd.setCursor(7, 0);
    lcd.print(encoderValue);
    lcd.print("%");

    //draws the adjustment mode text
    lcd.setCursor(0, 1);
    lcd.print("Adj: ");
    //draws the adjustment mode
    lcd.print(adjustment);
}

//controlls the motro speed and runs the motor when the button is pressed
void controlScreen() {
  int i, PWMvalue, rPWMvalue;

  //resets the encoder value
  encoderValue = 0;
  //changes from fine adjustment to course adjustment
  fineAdjust = false;
  adjustment = "Course";

  //draws he initial control screen
  drawControlScreen();

  while(true) {
    //checks for adjustment mode change
    fineAdjustment();

    //gets any encoder changes
    getEncoder();

    //checks for encoder value in bounds
    speedCheck();

    //if an encoder change was made, update the control screen
    if(newValue == true) {
      drawControlScreen();
      newValue = false;
    }

    //starts the motor only if the the controller is the master and the deadman switch is pressed for safety
    while(slave == false && digitalRead(deadMan) == LOW) {
      controller.attach(PWMpin);
      controller2.attach(PWMpin2);

      //tells the user to wait while the motor ramps up to speed
      lcd.setCursor(0, 1);
      lcd.print("Wait       ");

      lcd.setCursor(7, 0);
      lcd.print("     ");

      //ramps the motor up to speed
      //2 cases for a postive and negative speed
      for(i = 0; i <= abs(encoderValue); i++) {
        if(digitalRead(deadMan) != LOW) {
          controller.detach();
          controller2.detach();
          break;
        }

        lcd.setCursor(7, 0);
          
        if(encoderValue < 0) {
          PWMvalue = -i * 5 + 1500;
          rPWMvalue = i * 5 + 1500;
          controller.writeMicroseconds(PWMvalue);
          controller2.writeMicroseconds(rPWMvalue);
          lcd.print(-i);
        }
        else {
          PWMvalue = i * 5 + 1500;
          rPWMvalue = -i * 5 + 1500;
          controller.writeMicroseconds(PWMvalue);
          controller2.writeMicroseconds(rPWMvalue);
          lcd.print(i);
        }
      
        lcd.print("%");
        delay(25);
      }


      if(digitalRead(deadMan) == LOW) {
        //tells the user the motor is at speed
        lcd.setCursor(12, 1);
        lcd.print("    ");
        lcd.setCursor(0, 1);
        lcd.print("Ready");
      }

      //runs the safety checks again then runs the motor if they are met
      while(slave == false && digitalRead(deadMan) == LOW) {
        PWMvalue = encoderValue * 5 + 1500;
        rPWMvalue = -encoderValue * 5 + 1500;
        controller.writeMicroseconds(PWMvalue);
        controller2.writeMicroseconds(rPWMvalue);
      }

      controller.detach();
      controller2.detach();
      
      //debounces the button
      delay(buttonDelay);

      //redraws the control screen fo the user to edit
      drawControlScreen();
    }
  }
}

//The screen that is drawn when slave mode is selected
void slaveScreen() {
  //tells the user to use the master controller for speed control
  lcd.setCursor(1, 0);
  lcd.print("Use the master");
  lcd.setCursor(0, 1);
  lcd.print("to control speed");

  //loops forever and sleeps for ~16.7 minutes to reduce processor load
  while(true) {
    delay(1000000);
  }
}

//lets the user select which mode to use
void mainMenu() {
  //draws the 2 menu options
  lcd.setCursor(1, 0);
  lcd.print("Master");
  lcd.setCursor(1, 1);
  lcd.print("Slave");

  //loops until the user makes a mode selection
  while(true) {
    //draws the cursor at the encoder position
    lcd.setCursor(0, encoderValue);
    lcd.print(">");

    //checks for an encoder value change
    getEncoder();

    //keeps the encoder value in the valid range for the menu
    if(encoderValue < 0) {
      encoderValue = 0;
    }
    else if(encoderValue > 1) {
      encoderValue = 1;
    }

    //moves the cursor if the encoder changes position
    if(newValue == true){
      if(encoderValue == 0){
        //moves the cursor to the first line
        lcd.setCursor(0, 1);
        lcd.print(" ");
        lcd.setCursor(0, 0);
        lcd.print(">");
      }
      else if(encoderValue == 1) {
        //movs the cursor the the seconds line
        lcd.setCursor(0, 0);
        lcd.print(" ");
        lcd.setCursor(0, 1);
        lcd.print(">");
      }

      newValue = false;
    }
    
    //selects the manu option that the cursor is on
    if(digitalRead(encoderButton) == LOW) {
      delay(buttonDelay);

      if(encoderValue == 0) {
        //changes the slave state
        slave = false;
        lcd.clear();
        //starts the motor controller program
        controlScreen();
      }
      else if(encoderValue == 1) {
        //sets the slave state just incase
        slave = true;
        lcd.clear();
        //draws the slave message
        slaveScreen();
      }
    }
  }
}