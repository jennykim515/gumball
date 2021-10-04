/*
initialState: https://groker.init.st/api/events?accessKey=ist_3Y4exZqlwezqaeRaBAmeBbQkyDrKTvpS&bucketKey=LSLT9HDY8RLG
*/
//Link to bucket: https://go.init.st/2vdubnt
#include "pitches.h"
#include "SparkFunMicroOLED.h" // Include MicroOLED library
#include "math.h"
#include "bitmaps.h"
#include <blynk.h>
#include "SparkFunMMA8452Q.h"

//button
bool keepGoing = true;
int lastButtonState = LOW;

//piezzo buzzer
int melody[] = {NOTE_G5, NOTE_G5, NOTE_G5, NOTE_E5, NOTE_G5, NOTE_C6};
int duration[] = {2, 4, 4, 4, 4, 4};
const int PIN_SERVO = D4;
const int PIN_BUZZER = D2;
const int PIN_TRIGGER = D8;
const int PIN_ECHO = D3;
const int PIN_BUTTON = A5;

//for blynk
int menuItem;
int buttonPushed;

//for distance sensor
double SPEED = 0.03444;
int MAX_RANGE = 78;
int MIN_RANGE = 0;
double distance;
const double distanceToGND = 17;

//initial state
unsigned long prevStudyTime = 0;
int data = 0;
bool restart = false;

//accelerometer
int taps = 0;

//OLED bitmaps
enum Pics
{
  GUMBALL,
  STUDY,
  BREAK,
  MUSIC
};
//objects
Servo servo;
MicroOLED oled;
MMA8452Q accel;

//Blynk app
unsigned long prevMillis = 0;
unsigned long blynkDelay = 1000;
unsigned long timerLength = 0;

void setup()
{
  //for accelerometer
  accel.begin(SCALE_2G, ODR_1);
  byte threshold = 1;
  //time elapsed between pulses (in increments of 0.0625 ms)
  byte pulseTimeLimit = 200; //255 * 0.0625
  //after a pulse is detected, how long does it wait before another pulse is detected (like a delay)
  byte pulseLatency = 20;
  accel.setupTap(threshold, threshold, threshold, pulseTimeLimit, pulseLatency);

  //for button
  pinMode(PIN_BUTTON, INPUT);

  //for distance sensor
  pinMode(PIN_TRIGGER, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  //for piezzo buzzer
  pinMode(PIN_BUZZER, OUTPUT);

  //for servo
  pinMode(PIN_SERVO, OUTPUT);
  Serial.begin(9600);
  servo.attach(PIN_SERVO);

  //for OLED
  oled.begin();    // Initialize the OLED
  oled.clear(ALL); // Clear the display's internal memory
  delay(1000);     // Delay 1000 ms

  //for Blynk
  Serial.begin(9600);
  delay(5000); // Allow board to settle
  Blynk.begin("qnh7VDp-oZr-rsl2GK9aA4KySEWSqYym");
  Blynk.syncAll(); //optional if you want the argon to be sent all app values when argon resets
}

//OLED display action based on state
//switch LED based on state
void action()
{
  //if blynk button is pushed
  if (buttonPushed == 1)
  {
    //serve gumball
    playMusic();
    serveGum();
  }
  //otherwise, use distance sensor to trigger gumball
  else
  {
    motionSensor();
  }
}

//display welcome image before using machine
void displayWelcome()
{
  oled.clear(PAGE);
  switch (menuItem)
  {
  case 1:
    oled.drawBitmap(gumMachine);
    break;
  case 2:
    oled.drawBitmap(book);
    break;
  case 3:
    oled.drawBitmap(music);
    break;
  }
  oled.display();
}

//activates servo to dispense gum
void serveGum()
{
  for (int i = 108; i >= 13; i -= 2)
  {
    servo.write(i);
    delay(10);
  }
  delay(1000);

  for (int i = 13; i <= 110; i += 2)
  {
    servo.write(i);
    delay(10);
  }
  delay(1000);
}

//play the piezzo buzzer when detect motion
void playMusic()
{
  for (int i = 0; i < arraySize(melody); i++)
  {
    int noteTime = 1000 / duration[i];
    tone(PIN_BUZZER, melody[i], noteTime);

    int pauseTime = noteTime * 1.3;
    delay(pauseTime);
  }
}

void motionSensor()
{
  // start trigger
  digitalWrite(PIN_TRIGGER, LOW); // prepare
  delayMicroseconds(2);
  digitalWrite(PIN_TRIGGER, HIGH); // prepare
  delayMicroseconds(10);
  digitalWrite(PIN_TRIGGER, LOW); // prepare

  int timeRoundTrip = pulseIn(PIN_ECHO, HIGH); // wait for round trip time
  distance = timeRoundTrip * SPEED / 2;

  if (distance >= MAX_RANGE ||
      distance <= MIN_RANGE)
  {
    Serial.println("out of range");
    // digitalWrite(PIN_BLUE, LOW);
  }
  else if (distance <= distanceToGND)
  {
    Serial.println(String(distance) + " cm");
    // digitalWrite(PIN_BLUE, HIGH);
    playMusic();
    serveGum();
  }
  else
  {
    // digitalWrite(PIN_BLUE, LOW);
  }
  delay(500);
}

//get menu item from blynk
BLYNK_WRITE(V0)
{
  //assign incoming value from pin V0 to a variable
  int pinValue = param.asInt(); //or param.asStr() or .asDouble()
  menuItem = pinValue;
}

//check if button is pushed in blynk
BLYNK_WRITE(V2)
{
  int pinValue = param.asInt();
  buttonPushed = pinValue;
}

//check if blynk wants to restart timer
BLYNK_WRITE(V3)
{
  int pinValue = param.asInt();
  if (pinValue == 1)
  {
    restart = true;
  }
  else
  {
    restart = false;
  }
}

void readButton()
{
  //button (using analogRead, ran out of Digital pins)
  int button = analogRead(PIN_BUTTON);
  Serial.println(button);
  //if button pushed, switch off or on
  if (button < 100 && lastButtonState > 2000 && keepGoing == true)
  {
    //turns off the machine
    keepGoing = false;
  }
  else if (button < 100 && lastButtonState > 2000 && keepGoing == false)
  {
    keepGoing = true;
  }
  lastButtonState = button;
  delay(250);
}

void loop()
{
  Blynk.run();
  readButton();
  if (keepGoing)
  {
    displayWelcome();
    //if user wants to do study mode
    if (menuItem == 2)
    {
      //restart study timer
      if (restart)
      {
        data = 0;
      }
      unsigned long studyTime = millis();
      //publish every 1 minute
      if (studyTime - prevStudyTime > 60000)
      {
        //minutes
        //publish minutes studied to initial state
        Particle.publish("studyTime", String(data) + " min", PRIVATE);
        data += 1;
        prevStudyTime = studyTime;
      }
    }
    //regular gumball mode
    else if (menuItem == 1)
    {
      action();
    }
    //knocking mode (3 knocks to dispense gum)
    else
    {
      useAccel();
    }
  }
  else
  {
    Serial.println("Turned off");
  }
}

void useAccel()
{
  //CHECK IF DATA available
  if (accel.available())
  {
    accel.read(); //read the values of the acceleration

    if (accel.readTap() > 0)
    {
      //count as a bump
      Serial.println("Tap!");
      taps++;
      if (taps == 3)
      {
        playMusic();
        serveGum();
        taps = 0;
      }
    }
  }
}
