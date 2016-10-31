#include "DoorSM.h"
#include "SupervisorSM.h"
#include "AutoCloseSM.h"
#include "FlagDefines.h"
#include <bitset>

std::bitset<SIZE_OF_FLAGS_ENUM> myFlags; // initialize all flags false

int led1 = D7;
DoorSM myDoor;
SupervisorSM mySupervisor(9, 23); // default to start at 7AM, end at 10PM
AutoCloseSM myAutoClose;

void setup()
{
  checkInputs(); // check all switches so we set the flags before we call init on statemachines

  Time.zone(-7); // use PST
  Particle.function("toggleLED", toggleLED);

  //pinMode(led1, OUTPUT);
  //digitalWrite(led1, LOW);

  myDoor.init(&myFlags);
  mySupervisor.init();

  Serial.begin(9600);
}

void loop()
{
  myDoor.runSM(&myFlags);
  mySupervisor.runSM(&myFlags);
  if(mySupervisor.getState() == AUTO_CLOSED){
    if(myFlags.test(INIT_AUTOCLOSE_FLAG)) {
      myAutoClose.init(myDoor.getState(), &myFlags);
      myFlags.reset(INIT_AUTOCLOSE_FLAG); // clear flag
    }
    myAutoClose.runSM(&myFlags);
  }

  // print out events and statements
  if(myFlags.test(EVENT_CLOSED_FLAG)) {
    Serial.println("Main: Door closed event");
    myFlags.reset(EVENT_CLOSED_FLAG);
  }
  if(myFlags.test(EVENT_OPENED_FLAG)) {
    Serial.println("Main: Door open event");
    myFlags.reset(EVENT_OPENED_FLAG);
  }
  if(myFlags.test(FAULT_OPENING_FLAG)) {
    Serial.println("Main: Door opening fault");
    myFlags.reset(FAULT_OPENING_FLAG);
  }
  if(myFlags.test(FAULT_CLOSING_FLAG)) {
    Serial.println("Main: Door closing fault");
    myFlags.reset(FAULT_CLOSING_FLAG);
  }
  if(myFlags.test(FAULT_DOGSQUISHED_FLAG)) {
    Serial.println("Main: Dog squished");
    myFlags.reset(FAULT_DOGSQUISHED_FLAG);
  }
}

void serialEvent()
{
  // this is called after loop() so we can change flags without worrying about "thread safety"
  char c = Serial.read();
  // this is just for debugging (i.e. stepping through code) so lets make sure we reset the sw at a time by resetting the switch flags
  myFlags.reset(SWITCH_UPPER_FLAG);
  myFlags.reset(SWITCH_LOWER_FLAG);
  myFlags.reset(SWITCH_BUMPER_FLAG);

  switch(c) {
    case 'u': // upper limit switch
      myFlags.set(SWITCH_UPPER_FLAG);
      break;
    case 'l': // lower limit switch
      myFlags.set(SWITCH_LOWER_FLAG);
      break;
    case 'b': // bumper limit switch
      myFlags.set(SWITCH_BUMPER_FLAG);
      break;
    case 'p': //PIR sensor
      myFlags.set(SENSOR_PIR_FLAG);
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
      Serial.printf("AutoClose state: %s\n", AutoCloseStateNames[myAutoClose.getState()].c_str());
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
      Serial.printf("Current hour = %i, Start time = %i, End time = %i\n",Time.hour(), mySupervisor.getStartHour(), mySupervisor.getEndHour());

      break;
    break;

  }
}

int toggleLED(String command) {
  if(digitalRead(led1)) {
    digitalWrite(led1, LOW);
  }else{
    digitalWrite(led1, HIGH);
  }
  return 0;
}


void checkInputs() {
  //TODO: implement this
}
