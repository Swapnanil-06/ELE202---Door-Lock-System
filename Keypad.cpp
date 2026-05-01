#include "keypad.h"
#include "ThisThread.h"




// keypad constructor - allows the pins for the rows/columns to be specified
// notice the information after the single colon. This in member initialisation
// for objects defined in Keypad.h
// the keypad constructor is run when an object of this class is created. It performas
// any setup required
Keypad::Keypad(PinName row0,
              PinName row1,
              PinName row2,
              PinName row3,
              PinName col0,
              PinName col1,
              PinName col2): rows{row0, row1, row2, row3}, cols{col0, col1, col2}
{


   // create a 10ms scanning facility using a thread
   // the thread keyscan is also defined in Keypad.h
   // the callback() construct seems odd but is required
   // to ensure that the function holding the thread is
   // a function that is part of this specific class and object
   // KeyScanner scans the keys and assigns the instantaneous
   // key press to key
   keyscan.start(callback(this, &Keypad::KeyScanner));


   // initialise key to NO_KEY (nothing pressed)
   key = NO_KEY;


   // key_p is the value last time you interrogated the keypad
   key_p = NO_KEY;
}




// key scanning thread
// apply a 0 to one column read all the rows.
// repeat for all columns
// very simple - assumes only one key pressed
// could be improved
void Keypad::KeyScanner(void)
{ // use a local variable and only assign key at end
 char k;
   // do forever. This to ensure that the thread never finishes
   while (true) {


       // set key to NO_KEY
       k = NO_KEY;


       //loop through three columns
       for(int c = 0; c < 3; c++){
           cols[c] = 0; //set one column low
           for(int i = 0; i < 3; i++){
               //set other columns high
               if(i!=c){
                   cols[i] = 1;
               }
           }
           //loop through all rows
           for(int r = 0; r < 4; r++){
               if(rows[r] == 0){
                   k = mapping[c][r]; //store the current key pressed
                   break; //stop the loop once a key is found
               }
           }


           //stop looping other columns once a key is found
           if(k != NO_KEY){
               break;
           }
       }


       //pass the key found to be pressed to debounce
       key = debouncer.update(k);


       // go to sleep for 10ms then loop back and read the keys again
       // 10ms is a sweet spot: it's short enough so a uset thinks the
       // response is instantaneous but longer than the key bounce period
       // so we should not encounter any issues there
       ThisThread::sleep_for(10ms);
   }
}


// uses current value of key to determine if key pressed
// this is the only puplic function of the class and the
// only one that can be used - everything else happens under
// the hood
char Keypad::ReadKey(void)
{
   // only do something if the current value of the key
   // is different to the last time this function was called
   // this is the instant a key is pressed or de-pressed
   if (key != key_p) {


       // update the previous key value (for next time)
       key_p = key;


       // return key - the only time a value other than NO_KEY
       // is returned is when a key has just been pressed
       return key;
   }


   // by default return NO_KEY
   return NO_KEY;
}



