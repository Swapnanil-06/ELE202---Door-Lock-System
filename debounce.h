#pragma once //prevents redefinition
#undef __ARM_FP
#include "mbed.h"


//define NO_KEY if not yet defined
#ifndef NO_KEY
#define NO_KEY 0
#endif




class Debounce {
  
   public:
       Debounce(int samples = 4); //number of samples needed for the key to be accepted


       char update(char signal); //pass the signal from the hardware and returns the validated key
      
   private:
       int counter = 0;
       int threshold = 5; //number of samples need to be reach (internal memory storing samples value)
       char last_signal = NO_KEY; //store the last signal (10ms ago) allow for current signal to be compared with
       char stable_state = NO_KEY; //store the character that has been entered successfully


};