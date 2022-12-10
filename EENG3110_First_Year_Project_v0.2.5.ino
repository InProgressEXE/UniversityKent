// Importing libraries
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/sleep.h>

// Ultra sonic sensor
#define ult_echo_pin 4 //
#define ult_trig_pin 5 //

// OLED Diplay 128x64 pixels
#define screen_address 0x3C 
#define screen_W 128 // OLED display width, in pixels
#define screen_H 64 // OLED display height, in pixels
#define oled_reset -1
Adafruit_SSD1306 display(screen_W, screen_H, &Wire, oled_reset);

//Buttons
//Pins
#define button1_pin 3
#define button2_pin 2
//States
bool button1_active = false;
bool button2_active = false;
//Timing
unsigned long hold_time = 0;
const long extra_long_press_time = 1000;
const long long_press_time = 500; //can be changed to tune for comfort
bool long_press_active = false;

//LEDS
#define led1_pin 6
#define led2_pin 7

//Potentiometer
#define potentiometer_pin A3 // Change after getting your board

//Buzzer
#define buzzer_pin 9 // Change after getting your board

//Relay (Mosfet)
#define relay_pin 8 //Change after getting your board

// Menus
#define menu_item1 "Push To Measure"
#define menu_item2 "Storage/Upload"
#define menu_item3 "3 Measurements = Volume and S.A."
#define menu_item4 "Buzz and Relay At Distance"
#define menu_item5 "Clear Memory"
#define menu_item6 "About/Credit"
// Cursers
int current_item = 0;         // boot-up item from 0-6
int menu_frame = 1;
int menu_item_curser = 1;

//Push to measure
const int max_distance = 450; //cm
bool error_flag = false;

//saved values and frame for storage
int confirm_flag = false;
int current_value = 0;
int saved_values[50]; // = {290, 365, 385, 255, 273, 305, 190, 371, 382, 139, 253, 410, 171, 160, 140, 291, 268, 238, 298, 449, 336, 258, 108, 256, 187, 180, 303, 395, 113, 261, 379, 162, 104, 130, 386, 120, 419, 412, 228, 124, 233, 186, 158, 359, 136, 396, 148, 320, 265, 216};
int items_saved = 0; //50;
int last_storage_frame = 1;
int storage_frame = 1;

// 3 Measuremets values
// Curser
int curser_for_values = 1;
// State
bool value_selected = false;
// Values
int long width_value = 0;
int long hight_value = 0;
int long depth_value = 0;
int long volume_value = 0;
int long sa_value = 0;
int long calcu_offset = 0; // Needs to be positive

// Buzz and Relay
// States
bool active_relay = false; //The program is armed
bool buzz_item_selected = false;
bool relay_state = false; //Better to program this in (and be constant) rather then changing it by the user, but for demo its fine
bool buzzer_on_or_off = true; //Better to program this in (and be constant) rather then changing it by the user, but for demo its fine
bool reading_poten = false;
// Values
int hundreds_value = 0;
int tens_value = 0;
int ones_value = 0;
int actual_value = 0;
int symbol_selected = 0;
const char symbol_array[2] = {'<', '>'};
int buzz_item_curser = 1;
int poten_value = 0;

//Blink LED
//Timing
unsigned long led_current_time = 0;
unsigned long led_previous_time = 0;
const int long led_interval_time = 250;

void setup() {
  pinMode(ult_trig_pin, OUTPUT); 
  pinMode(ult_echo_pin, INPUT);
  pinMode(button1_pin, INPUT);
  pinMode(button2_pin, INPUT);
  pinMode(led1_pin, OUTPUT); // shows that relay is active
  pinMode(led2_pin, OUTPUT); // shows that microprocessor is active
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, screen_address);
  display.clearDisplay();
}


void loop() {
  //Serial.print("menu_item_curser: "); # for trouble shooting
  //Serial.println(menu_item_curser);
  //Serial.print("menu_frame: ");
  //Serial.println(menu_frame);
  //Serial.print("current_item: ");
  //Serial.println(current_item);
  
  led_blink(); // Blinking LED to show that program is running
  if (current_item == 0){ 
    menu_screen(menu_frame, menu_item_curser);
    delay(100);
    if (digitalRead(button1_pin)==HIGH && button1_active == false && digitalRead(button2_pin)==LOW){
      button1_active = true;
    }
    if (button1_active == true && digitalRead(button1_pin)==LOW){
        button1_active = false;
        if (menu_item_curser<2){
        ++menu_item_curser;
        }
        else if (menu_frame<3){
          menu_item_curser = 1;
          ++menu_frame;
        }
        else{
          menu_item_curser = 1;
          menu_frame = 1;
        }
    }
    if (digitalRead(button2_pin)==HIGH && button2_active == false && digitalRead(button1_pin)==LOW){
      button2_active = true;
    }
    if (button2_active == true && digitalRead(button2_pin)==LOW){
        if (menu_frame == 1){
          current_item = menu_item_curser;
        }
        if (menu_frame == 2){
          current_item = menu_frame + menu_item_curser;
        }
        if (menu_frame == 3){
          current_item = 4 + menu_item_curser;
        }
        button2_active = false;
      }
  }
  if (current_item == 1){ // FINISHED
    push_save_screen(0,false);
    if (digitalRead(button1_pin)==HIGH && digitalRead(button2_pin)==LOW){
      current_value = ult_sonic_measurement(false);
      push_save_screen(current_value, true);
      if (current_value == -1){
        current_value = 0;
      }
      delay(550);
      }
    if (digitalRead(button2_pin)==HIGH && button2_active == false && digitalRead(button1_pin)==LOW) {
      button2_active = true;
      hold_time = millis();
    }
    if (button2_active == true && digitalRead(button2_pin)==LOW){
        if ((millis()-hold_time)>=long_press_time){
          //Serial.print("Duration: ", (millis()-hold_time));
          //Serial.println();
          button2_active = false;
          current_item = 0;
          }
        if ((millis()-hold_time)<long_press_time){
          if(items_saved != 50 && error_flag == false){
            saved_values[items_saved] = current_value;
            items_saved++;
            display.fillRect(0, 10, 128, 59, SSD1306_WHITE);
            display.setTextSize(2);
            display.setCursor(3,13);
            display.setTextColor(SSD1306_BLACK);
            display.print(current_value); 
            display.println("cm");
            display.println("SAVED AT:");
            display.print("MEM: ");
            display.print(items_saved);
            display.display();
            delay(3200);
          }
          else if (error_flag == false){
            display.fillRect(0, 10, 128, 59, SSD1306_WHITE);
            display.setTextSize(2);
            display.setCursor(20,15); 
            display.setTextColor(SSD1306_BLACK);
            display.println("MEMORY");
            display.print("   FULL");
            display.display();
            delay(3200);
          }
          button2_active = false;
        }
    } 
  }
  if (current_item == 2){ // FINISHED
    display_memory_upload();
    if (storage_frame != 0){
      if (digitalRead(button1_pin)==HIGH && button1_active == false && digitalRead(button2_pin)==LOW){
        button1_active = true;
      }
      if (button1_active == true && digitalRead(button1_pin)==LOW){
          if (storage_frame<6){
            ++storage_frame;
            ++last_storage_frame;
          }
          else{
            storage_frame = 1;
            last_storage_frame = 1;
          }
          button1_active = false;
      }
      if (digitalRead(button2_pin)==HIGH && button2_active == false && digitalRead(button1_pin)==LOW) {
        button2_active = true;
        hold_time = millis();
      }
      if (button2_active == true && digitalRead(button2_pin)==LOW){
          if ((millis()-hold_time)>=long_press_time){
            button2_active = false;
            current_item = 0;
            }
          if ((millis()-hold_time)<long_press_time){
            button2_active = false;
            storage_frame = 0;
         }
      }
    }
    else{
      if (digitalRead(button1_pin)==HIGH && button1_active == false && digitalRead(button2_pin)==LOW){
        button1_active = true;
      }
      if (button1_active == true && digitalRead(button1_pin)==LOW){
        for (int i=0;i<50;++i){
          //Serial.println(i+1);
          Serial.println(saved_values[i]);
        }
        tone(buzzer_pin, 500, 500);
        storage_frame = last_storage_frame;
        button1_active = false;
      }
      if (digitalRead(button2_pin)==HIGH && button2_active == false && digitalRead(button1_pin)==LOW) {
        button2_active = true;
      }
      else if (button2_active == true && digitalRead(button2_pin)==LOW){
         storage_frame = last_storage_frame;
         button2_active = false; 
      }
    }
  }
  if (current_item == 3){ // FINISHED
    three_measurements();
    if (value_selected == false){
      if (digitalRead(button1_pin)==HIGH && button1_active == false && digitalRead(button2_pin)==LOW){
        button1_active = true;
      }
      if (button1_active == true && digitalRead(button1_pin)==LOW){
        value_selected = true;
        button1_active = false;
      }
    }
    else{
      if (digitalRead(button1_pin)==HIGH && button1_active == false && digitalRead(button2_pin)==LOW){
        button1_active = true;
      }
      if (button1_active == true && digitalRead(button1_pin)==LOW){
        if (curser_for_values == 1){
          hight_value = ult_sonic_measurement(false) + calcu_offset;
        }
        if (curser_for_values == 2){
          width_value = ult_sonic_measurement(false) + calcu_offset;
        }
        if (curser_for_values == 3){
          depth_value = ult_sonic_measurement(false) + calcu_offset;
        }
        value_selected = false;
        button1_active = false;
      }
    }
    if (digitalRead(button2_pin)==HIGH && button2_active == false && digitalRead(button1_pin)==LOW && value_selected == false) {
        button2_active = true;
        hold_time = millis();
      }
      else if (button2_active == true && digitalRead(button2_pin)==LOW){
          if ((millis()-hold_time)>=long_press_time){
            button2_active = false;
            current_item = 0;
            }
          if ((millis()-hold_time)<long_press_time){
            button2_active = false;
            if (curser_for_values<3){
              ++curser_for_values;
            }
            else{
              curser_for_values = 1;
            }
         }
      }
  }
  if (current_item == 4){ //FINISHED
    buzz_relay_at_distance();
    if (active_relay == false){
      noTone(buzzer_pin);
      if (reading_poten == true){
        poten_value = analogRead(potentiometer_pin);
        if (buzz_item_curser == 2){
          hundreds_value = map(poten_value, 0, 1015, 0, (max_distance/100));
        }
        if (buzz_item_curser == 3){
          tens_value = map(poten_value, 0, 1015, 0, 9);
        }
        if (buzz_item_curser == 4){
          ones_value = map(poten_value, 0, 1015, 0, 9);
        }
        if (digitalRead(button2_pin)==HIGH && button2_active == false && digitalRead(button1_pin)==LOW) {
          button2_active = true;
        }
        if (button2_active == true && digitalRead(button2_pin)==LOW){
          buzz_item_selected = false;
          reading_poten = false;
          button2_active = false;
          if (hundreds_value == (max_distance/100) && (tens_value>(max_distance%100)/10)){ // Checks if the the hundred's is at its maximum and if ten's is over the maximum distance then it sets it to maximum
            tens_value = (max_distance%100)/10;
          }
          else if (hundreds_value == (max_distance/100) && (tens_value==(max_distance%100)/10 && ones_value>(max_distance%10))){
            ones_value = (max_distance%10);
          }
          if (hundreds_value == 0 && tens_value == 0 && ones_value<5 && ones_value!=0){ // This gives me a safety zone for the ultrasonic sensor as it can only detect correctly until 2cm
            ones_value = 5;
          }
        }
      }
      if (buzz_item_selected == false){
        if (digitalRead(button1_pin)==HIGH && button1_active == false && digitalRead(button2_pin)==LOW){
          button1_active = true;
         }
        if (button1_active == true && digitalRead(button1_pin)==LOW){
          buzz_item_selected = true;
          button1_active = false;
        }
        if (digitalRead(button2_pin)==HIGH && button2_active == false && digitalRead(button1_pin)==LOW) {
          button2_active = true;
          hold_time = millis();
        }
        if (button2_active == true && digitalRead(button2_pin)==LOW){
            if ((millis()-hold_time)>=long_press_time){
              //Serial.print("Duration: ", (millis()-hold_time));
              //Serial.println();
              button2_active = false;
              current_item = 0;
              }
            if ((millis()-hold_time)<long_press_time){
              if(buzz_item_curser!=7){
                ++buzz_item_curser;
              }
              else{
                buzz_item_curser = 1;
              }
              button2_active = false;
            }
          }
       }
      else{
            if (buzz_item_curser == 1){
              if (digitalRead(button1_pin)==HIGH && button1_active == false && digitalRead(button2_pin)==LOW){
                button1_active = true;
              }
              if (button1_active == true && digitalRead(button1_pin)==LOW){
                if (symbol_selected == 0){
                  symbol_selected = 1;
                }
                else{
                  symbol_selected = 0;
               }
               button1_active = false;
              }
            }
            if (buzz_item_curser == 2||buzz_item_curser == 3||buzz_item_curser == 4){
              reading_poten = true;
            }
            if (buzz_item_curser == 5){ // Configuaring the Action Of Buzzer When Specified Distance Is Reached
              if (digitalRead(button1_pin)==HIGH && button1_active == false && digitalRead(button2_pin)==LOW){
                button1_active = true;
              }
              if (button1_active == true && digitalRead(button1_pin)==LOW){
                if (buzzer_on_or_off == false){
                  buzzer_on_or_off = true;
                }
                else{
                  buzzer_on_or_off = false;
               }
               button1_active = false;
              }
            }
            if (buzz_item_curser == 6){ // Configuaring the Action Of Relay When Specified Distance Is Reached
              if (digitalRead(button1_pin)==HIGH && button1_active == false && digitalRead(button2_pin)==LOW){
                button1_active = true;
              }
              if (button1_active == true && digitalRead(button1_pin)==LOW){
                if (relay_state == false){
                  relay_state = true; 
                }
                else{
                  relay_state = false;
               }
               button1_active = false;
              }
            }
            if (buzz_item_curser == 7){ // Activating the "Loop"
              if (digitalRead(button1_pin)==HIGH && button1_active == false && digitalRead(button2_pin)==LOW){
                button1_active = true;
              }
              if (button1_active == true && digitalRead(button1_pin)==LOW){
                button1_active = false;
                actual_value = (hundreds_value * 100)+(tens_value*10)+ones_value; // Putting together all the individual numbers together
                if (actual_value != 0){ // Dosent Activate the program if the distance is 0
                  buzz_item_selected = false; // Exiting the select state
                  active_relay = true; // Switiching States
                }
              }
            }
         }
          if (digitalRead(button2_pin)==HIGH && button2_active == false && digitalRead(button1_pin)==LOW) {
            button2_active = true;
          }
          if (button2_active == true && digitalRead(button2_pin)==LOW){
            buzz_item_selected = false;
            button2_active = false;
          }
      }
    else{
      if (digitalRead(button1_pin)==HIGH && button2_active == false && digitalRead(button2_pin)==LOW) {
            button1_active = true;
      }
      if (button1_active == true && digitalRead(button1_pin)==LOW){ // If the button is held for 1s or more then we exit the "Loop"
            if ((millis()-hold_time)>=extra_long_press_time){
              //Serial.print("Duration: ", (millis()-hold_time));
              //Serial.println();
              button1_active = false;
              active_relay = false;
            }
       }
      if (symbol_selected == 0){ // Depending on the selected symbol (more or less) a different action occures
        if (ult_sonic_measurement(true)<= actual_value || ult_sonic_measurement(true) == -1){ // When distance is reached or out of range it activates or dectivates relay or buzzer
          if (relay_state == true){
            digitalWrite(relay_pin, HIGH);
            digitalWrite(led1_pin, HIGH); //LED activates to provide visual evidance to show relay is active
          }
          else{
            digitalWrite(relay_pin, LOW);
            digitalWrite(led1_pin, LOW);
          }
          if (buzzer_on_or_off == true){
            tone(buzzer_pin, 100);
          }
        }
        else{
          if (relay_state == true){
            digitalWrite(relay_pin, LOW);
            digitalWrite(led1_pin, LOW);
          }
          else{
            digitalWrite(relay_pin, HIGH);
            digitalWrite(led1_pin, HIGH);
          }
          if (buzzer_on_or_off == true){
            noTone(buzzer_pin);
          }
        }
      }
      else{
        if (ult_sonic_measurement(true)>= actual_value || ult_sonic_measurement(true) == -1){
          if (relay_state == true){
            digitalWrite(relay_pin, HIGH);
            digitalWrite(led1_pin, HIGH);
          }
          else{
            digitalWrite(relay_pin, LOW);
            digitalWrite(led1_pin, LOW);
          }
          if (buzzer_on_or_off == true){
            tone(buzzer_pin, 100);
          }
        }
        else{
          if (relay_state == true){
            digitalWrite(relay_pin, LOW);
            digitalWrite(led1_pin, LOW);
          }
          else{
            digitalWrite(relay_pin, HIGH);
            digitalWrite(led1_pin, HIGH);
          }
          if (buzzer_on_or_off == true){
            noTone(buzzer_pin);
          }
        }
      }
    }
  }
  if (current_item == 5){ // FINISHED
    clear_mem_display();
    if (digitalRead(button1_pin)==HIGH && digitalRead(button2_pin)==LOW){
      button1_active = true;
      hold_time = millis();
    }
    if (button1_active == true && digitalRead(button1_pin)==LOW){
        if ((millis()-hold_time)>=extra_long_press_time){
          //Serial.print("Duration: ", (millis()-hold_time));
          //Serial.println();
          button1_active = false;
          for (int i=0;i<50;++i){
            saved_values[i] = 0;
          }
          tone(buzzer_pin, 500, 500);
        }
    }
    if (digitalRead(button2_pin)==HIGH && button2_active == false && digitalRead(button1_pin)==LOW) {
      button2_active = true;
      hold_time = millis();
    }
    if (button2_active == true && digitalRead(button2_pin)==LOW){
        if ((millis()-hold_time)>=long_press_time){
          //Serial.print("Duration: ", (millis()-hold_time));
          //Serial.println();
          button2_active = false;
          current_item = 0;
          //going to menu
          }
    }
  }
  if (current_item == 6){ // FINISHED
    about_display();
    if (digitalRead(button2_pin)==HIGH && button2_active == false && digitalRead(button1_pin)==LOW) {
      button2_active = true;
      hold_time = millis();
    }
    if (button2_active == true && digitalRead(button2_pin)==LOW){
        if ((millis()-hold_time)>=long_press_time){
          //Serial.print("Duration: ", (millis()-hold_time)); # for trouble shooting
          //Serial.println();
          button2_active = false;
          current_item = 0;
          //going to menu
          }
    }
  }
}
void about_display(){
  display.clearDisplay();
  // Top of the screen
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(27,0);
  display.print(F(menu_item6));
  display.drawLine(0,9,128,9,SSD1306_WHITE);
  // "Middle"
  display.setTextSize(1);
  display.setCursor(0,12);
  display.println(F("PCB Designed by:"));
  display.println(F("  -  Anton Dulia"));
  display.println(F("Programmed by:"));
  display.println(F("  -  Anton Dulia"));
  display.println(F("Inclosure Designed by"));
  display.print(F("  -  Anton Dulia"));
  //"Bottom"
  display.setCursor(103,56);
  display.drawRect(100, 54, 29, 11, SSD1306_WHITE);
  display.setTextSize(1);
  display.print(F("MENU"));
  display.display();
}

void clear_mem_display(){
  display.clearDisplay();
  // Top of the screen
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(27,0);
  display.print(F(menu_item5));
  display.drawLine(0,9,128,9,SSD1306_WHITE);
  // "Middle"
  display.setTextSize(1);
  display.setCursor(12,24);
  display.println(F("HOLD CLEAR FOR 1s"));
  display.print(F("   TO CLEAR MEMORY"));
  //"Bottom"
  display.setCursor(12,54);
  display.drawRect(9, 52, 35, 11, SSD1306_WHITE);
  display.setTextSize(1);
  display.print(F("CLEAR"));
  
  display.setCursor(92,54);
  display.drawRect(89, 52, 29, 11, SSD1306_WHITE);
  display.setTextSize(1);
  display.print(F("MENU"));
  display.display();
}

void buzz_relay_at_distance(){
  display.clearDisplay();
  // Top of the screen
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(2,0);
  display.println(F("   Buzz and Relay"));
  display.print(F("   At Set Distance"));
  display.drawLine(0,17,128,17,SSD1306_WHITE);
  // "Middle" of the screen
  display.setCursor(2,21);
  display.setTextSize(1); 
  display.print(F("Distance:"));
  if (active_relay == false){
    if (buzz_item_curser==1){
      display.setTextColor(SSD1306_BLACK);
      display.fillRect(55, 20, 8, 9, SSD1306_WHITE);
    }
    else{
      display.setTextColor(SSD1306_WHITE);
    }
    display.print(symbol_array[symbol_selected]);
    display.print(" ");
    if (buzz_item_curser==2){
      display.setTextColor(SSD1306_BLACK);
      display.fillRect(66, 20, 9, 9, SSD1306_WHITE);
    }
    else{
      display.setTextColor(SSD1306_WHITE);
    }
    display.print(hundreds_value);
    display.print(" ");
    if (buzz_item_curser==3){
      display.setTextColor(SSD1306_BLACK);
      display.fillRect(78, 20, 9, 9, SSD1306_WHITE);
    }
    else{
      display.setTextColor(SSD1306_WHITE);
    }
    display.print(tens_value);
    display.print(" ");
    if (buzz_item_curser==4){
      display.setTextColor(SSD1306_BLACK);
      display.fillRect(90, 20, 9, 9, SSD1306_WHITE);
    }
    else{
      display.setTextColor(SSD1306_WHITE);
    }
    display.print(ones_value);
    display.print(" ");
    display.setTextColor(SSD1306_WHITE);
    display.print("cm");
  }
  else{
    display.setTextColor(SSD1306_WHITE);
    if (ult_sonic_measurement(true)!= -1){
      display.print(ult_sonic_measurement(true));
      display.setTextColor(SSD1306_WHITE);
      display.print("cm");
    }
    else{
      display.print("OUT OF RANGE");
    }
  }
  display.drawLine(0,30,128,30,SSD1306_WHITE);
  
  //Buzzer state when distance reached
  display.setCursor(2,32);
  display.setTextColor(SSD1306_WHITE);
  display.print(F("Buzzer:"));
  if(buzz_item_curser==5){
    display.setTextColor(SSD1306_BLACK);
    display.fillRect(12, 41, 20, 9, SSD1306_WHITE);
  }
  else{
    display.setTextColor(SSD1306_WHITE);
  }
  if (buzzer_on_or_off == true){
    display.setCursor(16,42);
    display.print(F("ON"));
  }
  else{
    display.setCursor(14,42);
    display.print(F("OFF"));
  }
  display.drawLine(45,31,45,50,SSD1306_WHITE);
  
  //Relay state when distance reached
  display.setCursor(50,32);
  display.setTextColor(SSD1306_WHITE);
  display.print(F("Relay:"));
  if(buzz_item_curser==6){
    display.setTextColor(SSD1306_BLACK);
    display.fillRect(56, 41, 20, 9, SSD1306_WHITE);
  }
  else{
    display.setTextColor(SSD1306_WHITE);
  }
  if (relay_state == true){
    display.setCursor(60,42);
    display.print(F("ON"));
  }
  else{
    display.setCursor(58,42);
    display.print(F("OFF"));
  }
  display.drawLine(86,31,86,50,SSD1306_WHITE);
  
  //Can be activated?
  display.setCursor(90,32);
  display.setTextColor(SSD1306_WHITE);
  display.print(F("Activ:"));
  if(buzz_item_curser==7){
    display.setTextColor(SSD1306_BLACK);
    display.fillRect(97, 41, 19, 9, SSD1306_WHITE);
  }
  else{
    display.setTextColor(SSD1306_WHITE);
  }
  if (active_relay == true){
    display.setCursor(98,42);
    display.print(F("YES"));
  }
  else{
    display.setCursor(101,42);
    display.print(F("NO"));
  }
    
  //Bottom of screen
  display.setCursor(7,55);
  display.setTextColor(SSD1306_WHITE);
  if (buzz_item_selected == false && active_relay != true){
    display.drawRect(4, 53, 40, 11, SSD1306_WHITE);
    display.print(F("SELECT"));
  }
  else if (buzz_item_selected == true && (buzz_item_curser == 1 || buzz_item_curser == 5 || buzz_item_curser == 6)){
    display.drawRect(4, 53, 40, 11, SSD1306_WHITE);
    display.print(F("CHANGE"));
  }
  else if (buzz_item_selected == true && (buzz_item_curser == 2 || buzz_item_curser == 3 || buzz_item_curser == 4)){
    display.print(F("USE POTEN"));
  }
  else if(buzz_item_selected == true && buzz_item_curser == 7){
    display.drawRect(4, 53, 40, 11, SSD1306_WHITE);
    display.print("ACTIV?");
  }
  else if(active_relay == true){
    display.drawRect(4, 53, 53, 11, SSD1306_WHITE);
    display.print("KILL IT?");
  }
  
  display.setTextColor(SSD1306_WHITE);
  if (buzz_item_selected == true){
    display.setCursor(70,55);
    display.drawRect(68, 53, 52, 11, SSD1306_WHITE);
    display.setTextSize(1);
    display.print(F("UNSELECT"));
  }
  else if (active_relay != true){
    display.setCursor(60,55);
    display.drawRect(58, 53, 69, 11, SSD1306_WHITE);
    display.setTextSize(1);
    display.print(F("CHANGE/MENU"));
  }
  display.display();
}

void three_measurements(){
  // Some calculations
  if (error_flag != true){
    volume_value = hight_value*width_value*depth_value;
    sa_value = 2*(width_value*depth_value)+2*(hight_value*width_value)+2*(hight_value*depth_value);
  }
  else{
    volume_value = 0;
    sa_value = 0;
  }
  // Display output
  display.clearDisplay();
  // Top of the screen
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(2,0);
  display.println(F("  3 Measurements ="));
  display.print(F("   Volume and S.A."));
  display.drawLine(0,17,128,17,SSD1306_WHITE);
  //"Middle" of the screen
  display.setTextSize(1);
  if (curser_for_values == 1){
    display.drawRect(0, 20, 45, 11, SSD1306_WHITE);
  }
  display.setCursor(2,22);
  display.print("H:");
  if (hight_value != -1){
    display.print(hight_value);
    display.print("cm");
  }
  else{
    display.print("ERROR");
  }
  
  if (curser_for_values == 2){
    display.drawRect(0, 30, 45, 11, SSD1306_WHITE);
  }
  display.setCursor(2,32);
  display.print("W:");
  if (width_value != -1){
    display.print(width_value);
    display.print("cm");
  }
  else{
    display.print("ERROR");
  }
  if (curser_for_values == 3){
    display.drawRect(0, 40, 45, 11, SSD1306_WHITE);
  }
  display.setCursor(2,42);
  display.print("D:");
  if (depth_value != -1){
    display.print(depth_value);
    display.println("cm");
  }
  else{
    display.print("ERROR");
  }
  //Seperator line
  display.drawLine(46,18,46,50,SSD1306_WHITE);
  // Ans
  display.setCursor(49,19);
  display.print("Volume:");
  display.setCursor(49,28);
  display.print(volume_value);
  display.print("cm^3");
  
  display.setCursor(49,37);
  display.print("S.A.:");
  display.setCursor(49,46);
  display.print(sa_value);
  display.print("cm^2");
  // Bottom of the screen
  // Push to get distance
  display.setTextSize(1);
  if (value_selected == true){
    display.setCursor(5,56);
    display.drawRect(2, 54, 46, 11, SSD1306_WHITE);
    display.print(F("TAKE M."));
  }
  else{
    display.setCursor(7,56);
    display.drawRect(4, 54, 40, 11, SSD1306_WHITE);
    display.print(F("SELECT"));
  }
  // Save current distance/Menu
  if (value_selected == false){
    display.setCursor(60,56);
    display.drawRect(58, 54, 69, 11, SSD1306_WHITE);
    display.setTextSize(1);
    display.print(F("CHANGE/MENU"));
  }
  display.display();
}

void display_memory_upload(){
  display.clearDisplay();
  // Top of the screen
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(22,0);
  display.print(F(menu_item2));
  display.drawLine(0,9,128,9,SSD1306_WHITE);
  // "Middle" of the screen
  display.setTextSize(1);
  display.setCursor(0,15);
  if (storage_frame == 0){
    display.fillRect(0, 10, 128, 59, SSD1306_WHITE);
    display.setTextSize(1.5);
    display.setCursor(0,25); 
    display.setTextColor(SSD1306_BLACK);
    display.println("  HAVE YOU CONNCTED");
    display.print("  TO THE COMPUTER?");
  }
  if (storage_frame == 1){
    for (int i=1; i<=8;++i){
      if (i%2!=0){
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i-1]);
        display.print(F("cm"));
        display.print("   ");
      }
      else{
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i-1]);
        display.println(F("cm"));
      }
    }
  }
  if (storage_frame == 2){
    for (int i=9; i<=16;++i){
      if (i%2!=0){
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i-1]);
        display.print(F("cm"));
        display.print("   ");
      }
      else{
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i-1]);
        display.println(F("cm"));
      }
    }
  }
  if (storage_frame == 3){
    for (int i=17; i<=24;++i){
      if (i%2!=0){
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i]);
        display.print(F("cm"));
        display.print("   ");
      }
      else{
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i]);
        display.println(F("cm"));
      }
    }
  }
  if (storage_frame == 4){
    for (int i=31; i<=38;++i){
      if (i%2!=0){
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i-1]);
        display.print(F("cm"));
        display.print("   ");
      }
      else{
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i-1]);
        display.println(F("cm"));
      }
    }
  }
  if (storage_frame == 5){
    for (int i=39; i<=46;++i){
      if (i%2!=0){
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i-1]);
        display.print(F("cm"));
        display.print("   ");
      }
      else{
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i-1]);
        display.println(F("cm"));
      }
    }
  }
  if (storage_frame == 6){
    for (int i=47; i<=50;++i){
      if (i%2!=0){
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i-1]);
        display.print(F("cm"));
        display.print("   ");
      }
      else{
        display.print(i);
        display.print(F(")"));
        display.print(saved_values[i-1]);
        display.println(F("cm"));
      }
    }
  }
  if (storage_frame != 0){
    // Bottom of the screen
    // Push to scroll
    display.setCursor(5,54);
    display.drawRect(2, 52, 40, 11, SSD1306_WHITE);
    display.setTextSize(1);
    display.print(F("SCROLL"));
    // Upload the Memmory/Menu
    display.setCursor(56,54);
    display.drawRect(53, 52, 71, 11, SSD1306_WHITE);
    display.print(F("UPLOAD/MENU"));
    display.display();
  }
  else{
    display.setCursor(20,54);
    display.drawRect(17, 52, 23, 11, SSD1306_BLACK);
    display.setTextSize(1);
    display.print(F("YES"));
    // Upload the Memmory/Menu
    display.setCursor(90,54);
    display.drawRect(87, 52, 17, 11, SSD1306_BLACK);
    display.print(F("NO"));
    display.display();
  }
}

void push_save_screen(int distance, bool working){
  display.clearDisplay();
  // Top of the screen
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(15,0);
  display.print(F(menu_item1));
  display.drawLine(0,9,128,9,SSD1306_WHITE);
  // "Middle" of the screen
  display.setTextSize(2);
  display.setCursor(0,15);             // Start at top-left corner
  if (working == true && distance != -1){
    error_flag = false;
    display.println(F("Distance: "));
    display.print(distance);
    display.println(F(" cm"));
  }
  else if (working == true && distance == -1){
    error_flag = true;
    display.print(F("ERROR: OUT OF RANGE"));
  }
  else{
    display.println(F("Distance: "));
    display.print(current_value);
    display.println(F(" cm"));
  }
  // Bottom of the screen
  // Push to get distance
  display.setCursor(12,54);
  display.drawRect(9, 52, 29, 11, SSD1306_WHITE);
  display.setTextSize(1);
  display.print(F("PUSH"));
  // Save current distance/Menu
  display.setCursor(68,54);
  display.drawRect(66, 52, 58, 11, SSD1306_WHITE);
  display.setTextSize(1);
  display.print(F("SAVE/MENU"));
  display.display();
}

void menu_screen(int menu_frame, int menu_item_curser){
  display.clearDisplay();
  // Top of the screen
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(52,0);
  display.print(F("Menu"));
  display.drawLine(0,9,128,9,SSD1306_WHITE);
  display.setTextSize(2);
  // "Middle" of the screen
  display.setTextSize(1.5);
  if (menu_frame == 1){
      display.setCursor(1,15); 
      if (menu_item_curser==1){
        display.setTextColor(SSD1306_BLACK);
        display.fillRect(0, 12, 128, 14, SSD1306_WHITE);
        display.print(F("1) Distance And Save"));
      }
      else{
        display.setTextColor(SSD1306_WHITE);
        display.print(F("1) Distance And Save"));
      }
      display.setTextSize(1.5);
      display.setCursor(1,30);
      if (menu_item_curser==2){
        display.setTextColor(SSD1306_BLACK);
        display.fillRect(0, 28, 128, 18, SSD1306_WHITE);
        display.print(F("2)Saved Distances and  Upload To Computer"));
      }
      else{
        display.setTextColor(SSD1306_WHITE);
        display.print(F("2)Saved Distances and  Upload To Computer"));
      }
  }
  if (menu_frame == 2){
      display.setCursor(1,13);
      if (menu_item_curser==1){
        display.setTextColor(SSD1306_BLACK);
        display.fillRect(0, 12, 128, 18, SSD1306_WHITE);
        display.print(F("3) Measurments =\n Volume and S.A."));
      }
      else{
        display.setTextColor(SSD1306_WHITE);
        display.print(F("3) Measurments =\n Volume and S.A."));
      }
      display.setTextSize(1.5);
      display.setCursor(1,32);             // Start at top-left corner
      if (menu_item_curser==2){
        display.setTextColor(SSD1306_BLACK);
        display.fillRect(0, 31, 128, 17, SSD1306_WHITE);
        display.print(F("4) Buzz and Relay At  Distance"));
      }
      else{
        display.setTextColor(SSD1306_WHITE);
        display.print(F("4) Buzz and Relay At  Distance"));
      }
  }
  if (menu_frame == 3){
      display.setCursor(1,13);
      if (menu_item_curser==1){
        display.setTextColor(SSD1306_BLACK);
        display.fillRect(0, 12, 128, 11, SSD1306_WHITE);
        display.print(F("5) Clear Memory"));
      }
      else{
        display.setTextColor(SSD1306_WHITE);
        display.print(F("5) Clear Memory"));
      }
      display.setTextSize(1.5);
      display.setCursor(1,25);             // Start at top-left corner
      if (menu_item_curser==2){
        display.setTextColor(SSD1306_BLACK);
        display.fillRect(0, 23, 128, 11, SSD1306_WHITE);
        display.print(F("6) About"));
      }
      else{
        display.setTextColor(SSD1306_WHITE);
        display.print(F("6) About"));
      }
  }
  // Bottom of the screen
  // Push to scroll
  display.setCursor(12,54);
  display.setTextColor(SSD1306_WHITE);
  display.drawRect(9, 52, 41, 11, SSD1306_WHITE);
  display.setTextSize(1);
  display.print(F("SCROLL"));
  // Push to select
  display.setCursor(80,54);
  display.drawRect(76, 52, 43, 11, SSD1306_WHITE);
  display.setTextSize(1);
  display.print(F("SELECT"));
  display.display();
}


int ult_sonic_measurement(bool slower) {
  long duration;
  int distance, final_distance;
  // Clears the trigPin condition
  digitalWrite(ult_trig_pin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds (as instructed in the manual)
  digitalWrite(ult_trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(ult_trig_pin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(ult_echo_pin, HIGH);
  //Serial.print("Duration: ");
  //Serial.print(duration);
  //Serial.println(" ms");
  // Calculating the distance
  distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
  if (distance > max_distance){
    distance = -1;
    error_flag = true;
  }
  else{
    error_flag = false;
  }
  if (slower == true){
    delay(25);
  }
  // Displays the distance on the Serial Monitor
  //Serial.print("Distance: ");
  //Serial.print(distance);
  //Serial.println(" cm");
  return distance;
}

void led_blink(){
  // LED Blinking to show its working
  led_current_time = millis();
  if (led_current_time-led_previous_time>=led_interval_time){
    led_previous_time = led_current_time;
    if (digitalRead(led2_pin)==LOW){
      digitalWrite(led2_pin, HIGH);
    }
    else{
      digitalWrite(led2_pin, LOW);
    }
  }
}
