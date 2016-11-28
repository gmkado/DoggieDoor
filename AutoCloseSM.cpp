#include "AutoCloseSM.h"
#include "DoorSM.h" // need doorstate to initialize state

// Constructor
AutoCloseSM::AutoCloseSM() {
  autoCloseTimer.reset(new Timer(HOLDING_TIME, &AutoCloseSM::onTimeout, *this, true));
}

void AutoCloseSM::init(DoorState_t doorState, std::bitset<SIZE_OF_FLAGS_ENUM> *flags){
  // initialize state
  switch(doorState) {
    case DOOR_CLOSING:
      transitionTo(WAITING_FOR_DOOR_CLOSED);
      break;
    case DOOR_CLOSED:
      transitionTo(WAITING_FOR_DOG);
      break;
    case DOOR_OPEN:
      if(flags->test(SENSOR_INRANGE_FLAG)) {
        // there's a dog present, so wait for the allclear
        transitionTo(WAITING_FOR_ALL_CLEAR);
      }else{
        flags->set(COMMAND_CLOSE_FLAG); // time to close the door (this avoids having to hold door open if no dog is present on init)
        transitionTo(WAITING_FOR_DOOR_CLOSED);
      }
      break;
    case DOOR_OPENING:
      transitionTo(WAITING_FOR_DOOR_OPEN);
      break;
  }
  // consume exit event so we don't do this the first time we enter runSM()
  isExit = false;
}


/*********************** STATE MACHINE **************************************/
void AutoCloseSM::runSM(std::bitset<SIZE_OF_FLAGS_ENUM> *flags) {
  switch(currentState) {
    case WAITING_FOR_DOG:
      if(flags->test(SENSOR_INRANGE_FLAG)) { // dog is present, so open door
        flags->set(COMMAND_OPEN_FLAG);
        transitionTo(WAITING_FOR_DOOR_OPEN);
      }
      break;
    case WAITING_FOR_DOOR_OPEN:
      if (isEntry) {
        isEntry = false;
      }
      else if(flags->test(SWITCH_UPPER_FLAG)) { // door opened, so go to waiting for all clear
        transitionTo(WAITING_FOR_ALL_CLEAR);
      }
      break;
    case WAITING_FOR_ALL_CLEAR:
      if(!flags->test(SENSOR_INRANGE_FLAG)) { // dog is not present, so go to holding door open
        transitionTo(HOLDING_DOOR_OPEN);
      }
      break;
    case HOLDING_DOOR_OPEN:
      if(isEntry) {
        isEntry = false;
        autoCloseTimer->changePeriod(HOLDING_TIME);
        autoCloseTimer->start();    // start the timer
      }
      if(flags->test(SENSOR_INRANGE_FLAG)) { // sensor was triggered again, so wait for all clear
        transitionTo(WAITING_FOR_ALL_CLEAR);
      }
      if(timeoutOccurred) {
        timeoutOccurred = false; // clear flag
        flags->set(COMMAND_CLOSE_FLAG); // time to close the door
        transitionTo(WAITING_FOR_DOOR_CLOSED);
      }
      if(isExit) {
        autoCloseTimer->stop();
      }
      break;
    case WAITING_FOR_DOOR_CLOSED:
      if(flags->test(SWITCH_LOWER_FLAG)) {
        transitionTo(WAITING_FOR_DOG);
      }else if(flags->test(SWITCH_UPPER_FLAG)) {
        transitionTo(WAITING_FOR_DOOR_OPEN);
      }else if(flags->test(SWITCH_BUMPER_FLAG)) {
        transitionTo(WAITING_FOR_DOOR_OPEN); // don't worry about sending a door open command, that's handled by DoorSM
      }else if(flags->test(SENSOR_INRANGE_FLAG)) { // dog triggered sensor again, so go back to opening
        flags->set(COMMAND_OPEN_FLAG);
        transitionTo(WAITING_FOR_DOOR_OPEN);
      }

      break;
  }

  // ensure that isExit is false, if there were exit actions they should have been resolved by here
  if(isExit) {
    isExit = false;
  }

}


AutoCloseState_t AutoCloseSM::getState() {
  return currentState;
}

void AutoCloseSM::transitionTo(AutoCloseState_t to) {
  Serial.printf("AutoCloseSM: Transitioning from %s to %s\n", AutoCloseStateNames[currentState].c_str(), AutoCloseStateNames[to].c_str());
  currentState = to;
  isEntry = true;
  isExit = true;
}

void AutoCloseSM::onTimeout() {
  timeoutOccurred = true;
}
