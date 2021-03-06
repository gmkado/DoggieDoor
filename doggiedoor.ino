#include "DoorSM.h"
#include "SupervisorSM.h"
#include "AutoCloseSM.h"
#include "FlagDefines.h"
#include "PinDefines.h"
#include "HX711.h"
#include <bitset>

std::bitset<SIZE_OF_FLAGS_ENUM> myFlags; // initialize all flags false

const bool DEBUG = false;
bool status = false;
int calibration_factor = 23000;


//const unsigned int MAX_SENSOR_PULSEWIDTH = 23200;
//const unsigned int INRANGE_THRESH_INCHES = 20;

//unsigned long t1;
//unsigned long t2;
//unsigned long pulse_width;
//float inches;

//Timer sensorTriggerTimer(1000, sendTrigger, true);
Timer sendRSSITimer(15*60*1000, publishRSSI);
DoorSM myDoor;
SupervisorSM mySupervisor(6, 22); // default to start at 6AM, end at 10PM
AutoCloseSM myAutoClose;

HX711 *scale = NULL;

void setup()
{
  Time.zone(-8); // use PST

  Particle.function("getSupState", getSupState);
  Particle.function("setIsAuto", setIsAuto);
  Particle.function("getDoorState", getDoorState);
  Particle.function("openDoor", openDoor);
  Particle.function("closeDoor", closeDoor);

  //initialize pins
  pinMode(UPPER_LIMIT_PIN, INPUT);
  pinMode(LOWER_LIMIT_PIN, INPUT);
  pinMode(BUMPER_LIMIT_PIN, INPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_DIR1_PIN, OUTPUT);
  pinMode(MOTOR_DIR2_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);

  Serial.begin(9600);


  // attach interrupts for sensor
/*  attachInterrupt(SENSOR_ECHO_PIN, echoChange, CHANGE);
  digitalWrite(SENSOR_TRIGGER_PIN, LOW);
  sendTrigger(); // start the sensor */

  digitalWrite(STATUS_LED_PIN, LOW);

  if(!DEBUG) checkInputs(); // check all switches so we set the flags before we call init on statemachines

  // initialize state machines
  Particle.publish("Log", "************INITIALIZING DOOR ***************");
  mySupervisor.init();
  myDoor.init(mySupervisor.getState(), &myFlags);


  // publish the RSSI signal strength on start and every x minutes after
  publishRSSI();
  sendRSSITimer.start();
}

void loop()
{
  digitalWrite(STATUS_LED_PIN, status);
  myFlags.reset(SENSOR_INRANGE_FLAG); // TODO: For now, sensor is always low until we find a fix

  // run the state machines
  myDoor.runSM(&myFlags);
  mySupervisor.runSM(&myFlags);

  // Only run AutoClose state machine if supervisor is in the right state
   if(mySupervisor.getState() == AUTO_CLOSED){
    // Check if we need to initialize the state machine
    if(myFlags.test(INIT_AUTOCLOSE_FLAG)) {
      myAutoClose.init(myDoor.getState(), &myFlags);
      myFlags.reset(INIT_AUTOCLOSE_FLAG); // clear flag
    }
    myAutoClose.runSM(&myFlags);
  }

  // print out events and statements
  if(myFlags.test(EVENT_CLOSED_FLAG)) {
    if(DEBUG) Serial.println("Main: Door closed event");

    if(mySupervisor.getState() == MANUAL_MODE){
      Particle.publish("Log", "Event: Door closed manually");
    }else{
      Particle.publish("Log", "Event: Door closed automatically");
    }
    myFlags.reset(EVENT_CLOSED_FLAG);
  }
  if(myFlags.test(EVENT_OPENED_FLAG)) {
    if(DEBUG) Serial.println("Main: Door open event");

    switch(mySupervisor.getState()){
      case MANUAL_MODE:
        Particle.publish("Log", "Event: Door opened manually");
        break;
      case AUTO_OPEN:
        Particle.publish("Log", "Event: Door opened automatically");
        break;
      case AUTO_CLOSED:
        Particle.publish("Log", "Event: Door opened automatically");
        Particle.publish("Error", "Door opened during autoclose");  // TODO: take this out when we fix sensor
        break;
    }

    myFlags.reset(EVENT_OPENED_FLAG);
  }
  if(myFlags.test(FAULT_OPENING_FLAG)) {
    if(DEBUG) Serial.println("Main: Door opening fault");
    Particle.publish("Log", "Fault: Door timed out on open");
    myFlags.reset(FAULT_OPENING_FLAG);
  }
  if(myFlags.test(FAULT_CLOSING_FLAG)) {
    if(DEBUG) Serial.println("Main: Door closing timeout");
    Particle.publish("Log", "Fault: Door timed out on close");
    myFlags.reset(FAULT_CLOSING_FLAG);
  }
  if(myFlags.test(FAULT_MISSEDCLOSE_FLAG)) {
    if(DEBUG) Serial.println("Main: Missed the lower limit switch during close");
    Particle.publish("Log", "Fault: Missed the lower limit switch");
    myFlags.reset(FAULT_MISSEDCLOSE_FLAG);
  }
  if(myFlags.test(FAULT_DOGSQUISHED_FLAG)) {
    if(DEBUG) Serial.println("Main: Dog squished");
    Particle.publish("Log", "Fault: Dog squished");
    myFlags.reset(FAULT_DOGSQUISHED_FLAG);
  }
  if(myFlags.test(FAULT_NO_RELEASE_ON_CLOSE_FLAG)) {
    if(DEBUG) Serial.println("Main: Upper limit switch not released during open, possibly wrong direction.");
    Particle.publish("Log", "Fault: Upper limit switch not released during open, possibly wrong direction.");
    myFlags.reset(FAULT_NO_RELEASE_ON_CLOSE_FLAG);
  }
  if(myFlags.test(FAULT_NONRECOVER_FLAG)) { // NOTE this fault goes last, no others will report after it since it enters a while(true) statement
    // nonrecoverable error, caused by too many oscillations of the door
    String message = "NONRECOVERABLE!!! Door tried to open and close too many times.  Check door and restart";
    if(DEBUG) Serial.println("Main: Nonrecoverable, door tried to open and close too many times.  Check door and restart");
    Particle.publish("Log", "Fault: NONRECOVERABLE!!! Door tried to open and close too many times.  Check door and restart");
    Particle.publish("Error", "NONRECOVERABLE!!! Door tried to open and close too many times.  Check door and restart");  // TODO: take this out when we fix sensor
    while(true) Particle.process(); // maintain cloud connection but don't allow any other commands
  }

  if(!DEBUG)checkInputs(); // check inputs at end so that serial events override it

}

void serialEvent()
{
  // only look at serial inputs if we're debugging
  if(DEBUG) {
    // this is called after loop() so we can change flags without worrying about "thread safety"
    char c = Serial.read();
    // this is just for debugging (i.e. stepping through code) so lets make sure we reset the sw at a time by resetting the switch flags
    //myFlags.reset(SWITCH_UPPER_FLAG);
    myFlags.reset(SWITCH_LOWER_FLAG);
    myFlags.reset(SWITCH_BUMPER_FLAG);

    switch(c) {
      case 'u': // upper limit switch
        myFlags.flip(SWITCH_UPPER_FLAG);
        Serial.printf("Main: upper now set to %s\n", myFlags.test(SWITCH_UPPER_FLAG)?"true":"false");
        break;
      case 'l': // lower limit switch
        myFlags.set(SWITCH_LOWER_FLAG);
        break;
      case 'b': // bumper limit switch
        myFlags.set(SWITCH_BUMPER_FLAG);
        break;
      case 'p': //PIR sensor
        myFlags.set(SENSOR_INRANGE_FLAG);
        break;
      case 'o': // open command
        myFlags.set(COMMAND_OPEN_FLAG);
        break;
      case 'c': // closed command
        myFlags.set(COMMAND_CLOSE_FLAG);
        break;
      case 'q': // query state:
        Serial.printf("Door state: %s\n", DoorStateNames[myDoor.getState()].c_str());
        Serial.printf("Supervisor state: %s\n", SupervisorStateNames[mySupervisor.getState()].c_str());
        if(mySupervisor.getState() == AUTO_CLOSED) {
          Serial.printf("AutoClose state: %s\n", AutoCloseStateNames[myAutoClose.getState()].c_str());
        }
        break;
      case 's': // set start time
        mySupervisor.setStartHour(Serial.parseInt());
        break;
      case 'e': // set end time
        mySupervisor.setEndHour(Serial.parseInt());
        break;
      case 'm': // toggle manual mode
        mySupervisor.setIsAuto(!mySupervisor.getIsAuto());
        break;
      case 'f': // print out bit from bitset
        Serial.printf("Bitset = %s\n", myFlags.to_string().c_str());
        break;
      case ' ': // AUX
        //Serial.printf("Current hour = %i, Start time = %i, End time = %i\n",Time.hour(), mySupervisor.getStartHour(), mySupervisor.getEndHour());
        break;
      break;
    }
  }
}

void checkInputs() {
  // Check if the switch flags are HIGH
  digitalRead(LOWER_LIMIT_PIN)?myFlags.set(SWITCH_LOWER_FLAG):myFlags.reset(SWITCH_LOWER_FLAG);
  digitalRead(UPPER_LIMIT_PIN)?myFlags.set(SWITCH_UPPER_FLAG):myFlags.reset(SWITCH_UPPER_FLAG);
  digitalRead(BUMPER_LIMIT_PIN)?myFlags.set(SWITCH_BUMPER_FLAG):myFlags.reset(SWITCH_BUMPER_FLAG);



}

/*********************Particle functions ************************************/
int getSupState(String command) {
  return mySupervisor.getState();
}

int setIsAuto(String command) {
  mySupervisor.setIsAuto(strcmp(command,"true")==0); // strcmp returns 0 if they match!!
  return 0;
}

int openDoor(String command) {
  myFlags.set(COMMAND_OPEN_FLAG);
  return 0;
}

int closeDoor(String command) {
  myFlags.set(COMMAND_CLOSE_FLAG);
  return 0;
}

int getDoorState(String command) {
  return myDoor.getState();
}

/**************** sensor interrupts ****************/
/*void echoChange() {
  if (digitalRead(SENSOR_ECHO_PIN)) {
    t1 = micros();
  }else{
    status = !status;
    // stop timing and calculate distance
    t2 = micros();
    pulse_width = t2 - t1;
    inches = pulse_width / 148.0;

    // set sensor flag
    inches < INRANGE_THRESH_INCHES ? myFlags.set(SENSOR_INRANGE_FLAG) : myFlags.reset(SENSOR_INRANGE_FLAG);
    // start timer for sending trigger
    sensorTriggerTimer.startFromISR();
  }
}

void sendTrigger() {
  // send a trigger to the ultrasonic sensor
  digitalWrite(SENSOR_TRIGGER_PIN, HIGH);
  delayMicroseconds(10); // Hold the trigger pin high for at least 10 us
  digitalWrite(SENSOR_TRIGGER_PIN, LOW);
}*/

/****************** util ******************/
void publishRSSI() {
  // send rssi value to the logs
  char message[10];
  sprintf(message, "RSSI =  %d dB\n", WiFi.RSSI());
  Particle.publish("RSSI", message);
}
