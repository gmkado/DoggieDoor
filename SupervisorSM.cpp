#include "SupervisorSM.h"
#include <string>

// Constructor
SupervisorSM::SupervisorSM(int start, int end) {
  startHour = start;
  endHour = end;
}

void SupervisorSM::init(){
    // initialize state

    if(isDayTime()) {
        transitionTo(AUTO_OPEN);
    }else {
      transitionTo(AUTO_CLOSED);
    }

    // consume exit event so we don't do this the first time we enter runSM()
    isExit = false;

    // default is auto mode
    isAuto = true;

}


/*********************** STATE MACHINE **************************************/
void SupervisorSM::runSM(std::bitset<SIZE_OF_FLAGS_ENUM> *flags) {
    switch(currentState) {
        case MANUAL_MODE:
            if(isAuto) {
              if(isDayTime()) {
                  transitionTo(AUTO_OPEN);
              }else {
                 transitionTo(AUTO_CLOSED);
              }
            }
            break;
        case AUTO_OPEN:
            if(isEntry) {
                flags->set(COMMAND_OPEN_FLAG); // send an open door command
                isEntry = false;
            }
            if(!isDayTime()){
              transitionTo(AUTO_CLOSED);
            } else if(!isAuto) {
              transitionTo(MANUAL_MODE);
            }
            break;
        case AUTO_CLOSED:
            if(isEntry) {
                flags->set(INIT_AUTOCLOSE_FLAG);
                isEntry = false;
            }
            if(isDayTime()) {
              transitionTo(AUTO_OPEN);
            }else if(!isAuto) {
              transitionTo(MANUAL_MODE);
            }
            break;

    }

    // ensure that isExit is false, if there were exit actions they should have been resolved by here
    isExit = false;


}


SupervisorState_t SupervisorSM::getState() {
    return currentState;
}

void SupervisorSM::transitionTo(SupervisorState_t to) {
    Serial.printf("SupervisorSM: Transitioning from %s to %s\n", SupervisorStateNames[currentState].c_str(), SupervisorStateNames[to].c_str());
    currentState = to;
    isEntry = true;
    isExit = true;

    char publishString[10]; //TODO: is this a memory leak?
    sprintf(publishString, "%d", to);
    // publish the event
    Particle.publish("SupervisorState", publishString);
}


/*************** TIMER AND INTERRUPT CALLBACKS*********************/

/*************** GETTERS AND SETTERS *****************/
void SupervisorSM::setStartHour(int hour) {
  if(hour >= endHour) {
    Serial.printf("SupervisorSM: %i is not less than current end hour %i\n", hour, endHour);
  }else if(hour<0 || hour>23) {
    Serial.println("SupervisorSM: invalid hour");
  } else{
    startHour = hour;
  }
}

void SupervisorSM::setEndHour(int hour) {
  if(hour <= startHour) {
    Serial.printf("SupervisorSM: %i is not greater than current start hour %i\n", hour, startHour);
  }else if(hour<0 || hour>23) {
    Serial.println("SupervisorSM: invalid hour");
  } else{
    endHour = hour;
  }
}

int SupervisorSM::getStartHour() {
  return startHour;
}

int SupervisorSM::getEndHour() {
  return endHour;
}

bool SupervisorSM::isDayTime() {
  return Time.hour() >= startHour && Time.hour() < endHour;
}

void SupervisorSM::setIsAuto(bool isauto) {
  isAuto = isauto;
}

bool SupervisorSM::getIsAuto() {
  return isAuto;
}
