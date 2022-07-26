//Importing necessary external files
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <EEPROM.h>

//Defining Macros
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define WHITE 0x7
#define UPARROW 0
#define DOWNARROW 1
//Const
const char STUDENTID[] = "F129477";
//Creating the states
typedef enum state_e {SYNCHRONISATION, MAIN_PHASE} state_t;


Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

//Create byte uparrow
byte upArrow[] = { B00100, B01110, B10101, B00100, B00100, B00100, B00100, B00100 };
//Create byte downarrow
byte downArrow[] = { B00100, B00100, B00100, B00100, B00100, B10101, B01110, B00100 };

//Create pointers for scroll
byte topPointer = 0;
byte bottomPointer = 1;

//Creating the Structure for the channels
struct channel {
  char iD;
  char descr[16];
  byte minVal;
  byte maxVal;
  byte value;
  bool userCreated;
  int count;
  int total;
  int AVG;
};

//Setup
void setup() {
  lcd.begin(16, 2);
  lcd.setBacklight(5);
  lcd.createChar(UPARROW, upArrow);
  lcd.createChar(DOWNARROW, downArrow);
  Serial.begin(9600);
}

//FREERAM CODE - Taken from lab task
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else // __ARM__
extern char *__brkval;
#endif // __arm__
int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
}

//Bubblesort to alphabetically sort the channels on Arduino
void bubbleSort(struct channel *c)
{
  int i, j;
  for (i = 0; i < channelsMade(c) - 1; ++i) {
    bool noSwaps = true;
    // if last element greater than other element swap, otherwise turn swap false and repeat
    for (j = 0; j < channelsMade(c) - i - 1; ++j) {
      if (c[j].iD > c[j + 1].iD) {
        struct channel temp = c[j];
        c[j] = c[j + 1];
        c[j + 1] = temp;
        noSwaps = false;
      }
    }
    if (noSwaps) return;
  }
}

//Checks whether values are in range (0 - 255), otherwise return true
bool validation_check(struct channel _array[], char descr[]) {
  if (atoi(descr) > 255) {

    return false;
  }
  if (atoi(descr) < 0 ) {

    return false;
  }
  return true;
}

//FUNCTION FOR DEBUGGING - displays entire array
void showChannels(struct channel c[]) {
  for (byte i = 0 ; i < 26; ++i) {
    Serial.print("ID: ");
    Serial.print(c[i].iD);
    Serial.print(", ");
    Serial.print("Descr: ");
    Serial.print(c[i].descr);
    Serial.print(", ");
    Serial.print("Minval: ");
    Serial.print(c[i].minVal);
    Serial.print(", ");
    Serial.print("Maxval: ");
    Serial.print(c[i].maxVal);
    Serial.print(" Value: ");
    Serial.print(c[i].value);
    Serial.print("AVG: ");
    Serial.print(c[i].AVG);
    Serial.print(", In use: ");
    Serial.print(c[i].userCreated);
    Serial.println();
  }
}
//Gives each channel an index
int channelIndex(struct channel array_[], char _operator) {
  for (byte i = 0; i < 26; ++i) {
    if (array_[i].iD == _operator) {
      //Create index of channels
      return i;
    }
  }
  return -1;
}

//Goes through struct in a for loop and creates an array of channels
int newChannel(struct channel _array[] , char iD, char descr[]) {
  int i  = channelIndex(_array, iD);
  if (i == -1) {
    //doesn't exist so create channel
    for (byte i = 0 ; i < 26; ++i) {
      if (_array[i].iD == '*') {
        _array[i].iD = iD;
        strcpy(_array[i].descr, descr);
        _array[i].userCreated = true;
        return descr;
      }
    }
  }
  else {
    strcpy(_array[i].descr, descr);
    return;
  }
}

//Changes value based on user input
byte changeValue(struct channel _array[], char iD , char descr[]) {
  int i = channelIndex(_array, iD);
  if (i == -1) {
    return;
  }
  _array[i].value = atoi(descr);
  _array[i].total += atoi(descr);
  _array[i].count++;
  _array[i].AVG = _array[i].total / _array[i].count;
  //  showChannels(_array);
  return _array[i].value;
}

//Determines if value size bigger than max val, smaller than min val or both then changes the backlight
void value_size(struct channel _array[]) {
  bool big_value = false;
  bool small_value = false;
  for (byte i = 0; i < 26; ++i) {
    if (_array[i].value > _array[i].maxVal) {
      big_value = true;
    }
    if (_array[i].value < _array[i].minVal) {
      small_value = true;
    }
  }
  if (small_value == true & big_value == true) {
    lcd.setBacklight(YELLOW);
  }
  else if (big_value == true) {
    lcd.setBacklight(RED);
  }
  else if (small_value == true) {
    lcd.setBacklight(GREEN);
  }
  else {
    lcd.setBacklight(WHITE);
  }
}

//Changes min value depending on user input
byte changeMinValue(struct channel _array[], char iD , char descr[]) {
  byte i = channelIndex(_array, iD);
  if (i == -1) {
    return;
  }
  //equates minval to the description given in the serial monitor. ATOI gives it a int value
  _array[i].minVal = atoi(descr);
  return _array[i].minVal;
}

//changes max value based on user input
void changeMaxValue(struct channel _array[], char iD , char descr[]) {
  int i = channelIndex(_array, iD);
  if (i == -1) {
    return;
  }
  _array[i].maxVal = atoi(descr);
}

//Main function used to alter lcd
void updateScreen(struct channel _array[], bool showStudentId) {

  lcd.clear();
  //changes display based on select button being held
  if (showStudentId == true) {
    lcd.print(STUDENTID);
    lcd.setBacklight(5);
    lcd.setCursor(0, 1);
    lcd.print("FREE SRAM: ");
    lcd.print(freeMemory());
    return;
  } else {
    value_size(_array);
  }
  for (byte i = 0 ; i < 2; ++i) {

    int pointer = 0;
    if (i == 0) {
      pointer = topPointer;
    } else {
      pointer = bottomPointer;
    }
    if (_array[pointer].userCreated == false) continue;
    //Displaying up arrow
    lcd.setCursor(0, i);
    if (i == 0) {
      if (pointer == 0) {
        lcd.print(" ");
      } else {
        lcd.write(UPARROW);
      }
    } else {
      //displaying down arrow
      if (pointer == channelsMade(_array) - 1) {
        lcd.print(" ");
      } else {
        lcd.write(DOWNARROW);
      }
    }

    lcd.print(_array[pointer].iD);
    if (_array[pointer].value < 10) {
      lcd.print(" ");
    }
    if (_array[pointer].value < 100) {
      lcd.print(" ");
    }
    if (_array[pointer].value == 0) {
      lcd.print(" ");
      lcd.print(", ");
    }
    else {
      lcd.print(_array[pointer].value);
      lcd.print(",");
    }
    if (_array[pointer].AVG < 10) {
      lcd.print(" ");
    }
    if (_array[pointer].AVG < 100) {
      lcd.print(" ");
    }
    if (_array[pointer].AVG == 0) {
      lcd.print(" ");
    }
    else {
      lcd.print(_array[pointer].AVG);
      lcd.print(" ");
    }
    lcd.print(_array[pointer].descr);

  }

}

void scroll(struct channel _array[], bool showStudentId) {
  //this is to skip scroll if select button held
  if (showStudentId == true) {
    return;
  }
  char scroller1[16] = "               ";
  char scroller2[16] = "               ";
  strcpy(scroller1, _array[topPointer].descr);
  strcpy(scroller2, _array[bottomPointer].descr);
  static unsigned int scrollpos1 = 0;
  static unsigned int scrollpos2 = 0;
  static unsigned long now = millis();

  if (millis() - now > 500) {
    now = millis();
    //increment scroll
    scrollpos1 ++;
    scrollpos2++;
    if (scrollpos1 > strlen(scroller1) - 6) {
      scrollpos1 = 0;
    }
    if (scrollpos2 > strlen(scroller2) - 6) {
      scrollpos2 = 0;
    }
  }
  if (strlen(scroller1) > 6) {
    lcd.setCursor(10, 0);
    for (int i = 0; i < strlen(scroller1); i++) {
      lcd.print(scroller1[scrollpos1 + i]);
    }
  }
  if (strlen(scroller2) > 6) {
    lcd.setCursor(10, 1);
    for (int i = 0; i < strlen(scroller2); i++) {
      lcd.print(scroller2[scrollpos2 + i]);
    }
  }
}
//Counts amount of made channels
byte channelsMade(struct channel _array[]) {
  int count = 0;
  for (byte i = 0 ; i < 26; ++i) {
    if (_array[i].userCreated) {
      count++;
    }
  }
  return count;
}

void loop() {
  //Main variables
  static state_t state = SYNCHRONISATION;
  static struct channel array_[26];
  static int previousButtons = 0;
  static long savedTime = 0;
  static bool showStudentId = false;
  static bool buttonHold = false;
  //Clearing memory
  static bool x = false;
  if (!x) {
    for (int i = 0 ; i < 26 ; ++i) {
      struct channel c;
      c.iD = '*';
      c.descr[0] = '\0';
      c.minVal = 0;
      c.maxVal = 255;
      c.value = 0;
      c.AVG = 0;
      c.count = 0;
      c. total = 0;
      c.userCreated = false;

      array_[i] = c;//loop through this (array to see if channel has been made)
    }
  }
  x = true;

  scroll(array_, showStudentId);
//Synchronisation phase
  if (state == SYNCHRONISATION) {
    String input = "";
    while (input != "X") {
      Serial.print('Q');
      input = Serial.readString();
    }
    //Shows all extensions done
    Serial.print("BASIC,UDCHARS,FREERAM,RECENT,NAMES,SCROLL\n");
    lcd.setBacklight(7);
    state = MAIN_PHASE;
  }
  //Main phase triggered
  else if (state == MAIN_PHASE) {
    if (Serial.available() > 0) {
      String channelInput = Serial.readString();
      char operator_ = channelInput[0];
      char iD = channelInput[1];
      char descr[16] = "               ";
      for (byte i = 0 ; i < strlen(descr); ++i) {
        descr[i] = channelInput[i + 2];
        if (descr[i] == '\0') break;
      }

      //Reads user input
      switch (operator_) {
        //if C: append and create new channel with values typed
        case 'C':
          //Validation
          if (descr[0] == '\0') {
            Serial.print("ERROR: ");
            Serial.print(channelInput);
            Serial.print('\n');
            break;
          }
          else {
            newChannel(array_, iD, descr);
            bubbleSort(array_);
            updateScreen(array_, showStudentId);
            value_size(array_);
          }
          break;
        //Update value parameter
        case 'V':
          //Validation
          if (validation_check(array_, descr) == false) {
            Serial.print("ERROR: ");
            Serial.print(channelInput);
            Serial.print('\n');
            break;
          }
          if (atoi(descr) == '\0') {
            Serial.print("ERROR: ");
            Serial.print(channelInput);
            Serial.print('\n');
            break;
          }
          else {
            //if all validations pass
            changeValue(array_, iD, descr);
            updateScreen(array_, showStudentId);
            value_size(array_);


            break;
          }

        //update min value parameter
        case 'N':
          //Validation
          if (validation_check(array_, descr) == false) {
            Serial.print("ERROR: ");
            Serial.print(channelInput);
            Serial.print('\n');
            break;
          }
          if (atoi(descr) == '\0') {
            Serial.print("ERROR: ");
            Serial.print(channelInput);
            Serial.print('\n');
            break;
          }
          else {
            changeMinValue(array_, iD, descr);
            updateScreen(array_, showStudentId);
            value_size(array_);

            break;
          }

        //update max value parameter
        case 'X':
          //Validation
          if (validation_check(array_, descr) == false) {
            Serial.print("ERROR: ");
            Serial.print(channelInput);
            Serial.print('\n');
            break;
          }
          if (atoi(descr) == '\0') {
            Serial.print("ERROR: ");
            Serial.print(channelInput);
            Serial.print('\n');
            break;
          }
          else {
            changeMaxValue(array_, iD, descr);

            updateScreen(array_, showStudentId);
            value_size(array_);

            break;
          }
        default:
          //If input is unrecognised
          Serial.print("ERROR: ");
          Serial.print(channelInput);
          Serial.print("\n");
          break;
      }
    }
  }
  else {
    //If nothing is recognised
    Serial.print("ERROR");
  }
  //Buttons
  int buttons = lcd.readButtons();
  if (buttons != previousButtons) {
    if (buttons & BUTTON_UP) {
      //For scrolling and displaying updated screen
      if (channelsMade(array_) > 2) {
        topPointer--;
        bottomPointer--;
        if (topPointer < 0 || bottomPointer < 1 ) {
          topPointer = 0;
          bottomPointer = 1;
        }
        updateScreen(array_, showStudentId);
      }

    }
    //for scrolling and then updates screen based off scroll
    if (buttons & BUTTON_DOWN) {
      if (channelsMade(array_) > 2) {
        //Increments pointers up by one
        topPointer++;
        bottomPointer++;
        if (topPointer >= channelsMade(array_) - 2) {
          topPointer = channelsMade(array_) - 2;
          bottomPointer = channelsMade(array_) - 1;
        }
        updateScreen(array_, showStudentId);
      }
    }
    //updates previous buttons to the new buttons that have been pressed
    previousButtons = buttons;
  }
  //Checks to see if user has started holding the select button
  if (buttons & BUTTON_SELECT) {
    if (buttonHold == false) {
      buttonHold = true;
      savedTime = millis();
    }
    else {
      //Sees if user is holding select for 1 second
      if (showStudentId == false and savedTime + 1000 < millis())
      {
        showStudentId = true;
        updateScreen(array_, showStudentId);
      }
    }
  }
  //Checks to see if user has stopped pressing button - if so calls function and updates screen back to channels
  if (showStudentId & (buttons & BUTTON_SELECT) == false) {
    showStudentId = false;
    buttonHold = false;
    savedTime = millis();
    //    Serial.println("no longer holding select");
    updateScreen(array_, showStudentId);
  }
}
