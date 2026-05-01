#include "debounce.h"


//Constructor - sets up the initial values
Debounce::Debounce(int samples){
   threshold = samples;
   counter = 0;
   last_signal = NO_KEY;
   stable_state = NO_KEY;
}


char Debounce::update(char signal){
   if (signal != last_signal){
       counter = 0; //reset when the signal is unstable
       last_signal = signal; //update the signal
   }
   else{
       counter++; //increment when signal is consistent
   }
   if(counter == threshold){
       stable_state = last_signal; //accept a new key when threshold is reached
   }
   return stable_state; //return the current stable state


}
