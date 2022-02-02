#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

#include <pins.h>

//menus screens
void mainMenu();
void speedMenu();
void slaveScreen();
void titleScreen();

//operation functions
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
  //the main loop is run in a seperate funcion
  mainMenu();
}

//draws the title screen before starting the program
void titleScreen() {
  lcd.clear();
  //draws the team name centered on the screen
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

//checks for encoder change and direction of change
void getEncoder() {
  //stores the current encoder pin A value
  n = digitalRead(encoderPinA);
  
  //checks if the encoder changed position
  //encoder position starts at 0
  //a CW turn increases the postion count
  //a CCW turn decreases the position count
  if ((encoderPinALast == LOW) && (n == HIGH)) {
    //checks the state of pin B
    //if low, the encoder turned CCW
    //if high, the encoder turned CW
    if (digitalRead(encoderPinB) == LOW) {
      //fine adjustment allows for incraments of 1
      //course adjustment is in incraments of 10
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
    
    //changes the adjustment mode (i.e. course to fine and fine to course)
    if(fineAdjust == false) {
      fineAdjust = true;
      adjustment = "Fine";

      //prints the new adjstment mode
      lcd.print(adjustment);
      //draws 2 blank characters becasue "fine" is 2 characters shorter than "course" 
      //the "se" needs to be erased if it's there
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

//controlls the motor speed and runs the motor when the button is pressed
//many checks to make sure the 
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

    //starts tmothe or only if the the controller is the master and the deadman switch is pressed for safety
    while(slave == false && digitalRead(deadMan) == LOW) {
      controller.attach(PWMpin);
      controller2.attach(PWMpin2);

      //tells the operator to wait while the motor ramps up to speed
      lcd.setCursor(0, 1);
      lcd.print("Wait       ");

      //clears the currently displayed motor speed
      lcd.setCursor(7, 0);
      lcd.print("     ");

      //ramps the motor up to speed
      //2 cases for a postive and negative speed
      for(i = 0; i <= abs(encoderValue); i++) {
        //verifies that the deadman switch is pressed and the operator is still alive
        //if the operator releases the button, disable the PWM output
        if(digitalRead(deadMan) != LOW) {
          controller.detach();
          controller2.detach();
          break;
        }

        //moves the cursor to the location of the motor speed
        lcd.setCursor(7, 0);
        
        //if the motor is set to reverse
        if(encoderValue < 0) {
          PWMvalue = -i * 5 + 1500;
          rPWMvalue = i * 5 + 1500;
          controller.writeMicroseconds(PWMvalue);
          controller2.writeMicroseconds(rPWMvalue);
          lcd.print(-i);
        }
        //if the motor is ser to forward
        else if(encoderValue > 0) {
          PWMvalue = i * 5 + 1500;
          rPWMvalue = -i * 5 + 1500;
          controller.writeMicroseconds(PWMvalue);
          controller2.writeMicroseconds(rPWMvalue);
          lcd.print(i);
        }
        //something has gone terribly wrong, stop the motor
        else {
          controller.detach();
          controller2.detach();
          break;
        }
      
        //print the current motor speed
        lcd.print("%");
        delay(25);
      }


      if(digitalRead(deadMan) == LOW) {
        //tells the operator the motor is at speed
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

      //redraws the control screen fo the operator to edit
      drawControlScreen();
    }
  }
}

//The screen that is drawn when slave mode is selected
void slaveScreen() {
  //tells the operator to use the master controller for speed control
  lcd.setCursor(1, 0);
  lcd.print("Use the master");
  lcd.setCursor(0, 1);
  lcd.print("to control speed");

  //loops forever and sleeps for ~16.7 minutes to reduce processor load
  while(true) {
    delay(1000000);
  }
}

//lets the operator select which mode to use
void mainMenu() {
  //draws the 2 menu options
  lcd.setCursor(1, 0);
  lcd.print("Master");
  lcd.setCursor(1, 1);
  lcd.print("Slave");

  //loops until the operator makes a mode selection
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
        //moves the cursor the the second line
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

      //master
      if(encoderValue == 0) {
        //changes the slave state
        slave = false;
        lcd.clear();
        //starts the motor controller program
        controlScreen();
      }
      //slave
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