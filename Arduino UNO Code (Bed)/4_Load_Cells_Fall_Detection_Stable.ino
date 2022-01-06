#include "HX711.h"

HX711 scale_1(5, 6);
HX711 scale_2(7, 8);
HX711 scale_3(9, 10);
HX711 scale_4(11, 12);

float calibration_factor_1 = 40000;  //Load cells are analog devices that have unique calibration coefficients
float calibration_factor_2 = 39500;
float calibration_factor_3 = 40000;
float calibration_factor_4 = 40000;

long zero_factor_1;
long zero_factor_2;
long zero_factor_3;
long zero_factor_4;

float units_1;
float units_2;
float units_3;
float units_4;

float fall_threshold_1 = 1000; //Default values do not mean anything, are recalculated when calibration is done
float fall_threshold_2 = 1000;
float fall_threshold_3 = 1000;
float fall_threshold_4 = 1000;

float fall_factor_top = 1.7; //This coefficient is multiplied with the ocfs to determine when the user has fallen from the bed
float fall_factor_bot = 1.4;

float ocf1 = 0; //Occupied Calibration Factor [calculated once someone is on the bed]
float ocf2 = 0;
float ocf3 = 0;
float ocf4 = 0;

unsigned int sample = 100; //Time elapsed per iteration in milliseconds (0.1s for 10Hz)
unsigned long Previous_Time = 0; //Used for finding time taken per iteration
unsigned long Initial_Time = 0;
unsigned long Current_Time = 0;  //Used to check whether calibration time has been reached [main loop duration]
unsigned int calibration_time = 20000; //Time in milliseconds that bed calibrates ocfs

const int buzzer = 3; //We will use Digital Pin 3 for the Buzzer

int division_constant = 1; //Starts with a stable value for division constant
bool person_has_gotten_up = 0;
bool person_is_falling_from_left = 0;
bool person_is_falling_from_right = 0;
bool person_on = 0; //Relates to the initial sending of occupied thresholds to Python
bool calibration_done = 0;

unsigned long time_of_getting_up = 0; //Starts of at an abritraily low value
int movement_tolerance_time = 20000; //If the user does any sudden movements, the variables should not be reset as long as they only take 20s
unsigned long time_of_load_below_threshold = 0;   //The instant the load cells stop detecting the expected load, this time is taken to check if 3s have elapsed
int getting_up_tolerance = 12000; //If the subject gets up on the bed, alarm will not trigger if they get off as long as it occurs within 12 seconds
int unoccupied_threshold = 2; //Unoccupied threshold is arbitrarily assigned as 2kg detected per weight

void setup() {
  Serial.begin(9600);

  //Load Cell Calibration, please wait 5 seconds

  pinMode(buzzer, OUTPUT); //We set up the buzzer at Pin 3

  scale_1.set_scale();
  scale_1.tare();  //Reset the scale to 0, to calculate the zero factor
  zero_factor_1 = scale_1.read_average();
  scale_1.set_scale(calibration_factor_1); //Re-enter the calibration factor once the zero factor has been made
  scale_1.set_offset(zero_factor_1);

  scale_2.set_scale();
  scale_2.tare();  //Reset the scale to 0, to calculate the zero factor
  zero_factor_2 = scale_2.read_average();
  scale_2.set_scale(calibration_factor_2); //Re-enter the calibration factor once the zero factor has been made
  scale_2.set_offset(zero_factor_2);

  scale_3.set_scale();
  scale_3.tare();  //Reset the scale to 0, to calculate the zero factor
  zero_factor_3 = scale_3.read_average();
  scale_3.set_scale(calibration_factor_3); //Re-enter the calibration factor once the zero factor has been made
  scale_3.set_offset(zero_factor_3);

  scale_4.set_scale();
  scale_4.tare();  //Reset the scale to 0, to calculate the zero factor
  zero_factor_4 = scale_4.read_average();
  scale_4.set_scale(calibration_factor_4); //Re-enter the calibration factor once the zero factor has been made
  scale_4.set_offset(zero_factor_4);

  Initial_Time = millis();
  
}

void loop() {

  units_1 = scale_1.get_units(), 10;
  units_2 = scale_2.get_units(), 10;
  units_3 = scale_3.get_units(), 10;
  units_4 = scale_4.get_units(), 10;

  Current_Time = millis() - Initial_Time; //Will be used to check if calibration time has elapsed

  //Part of the Code that calculates an occupied threshold once a person has climbed on the bed

  if ((units_1 > unoccupied_threshold && units_3 > unoccupied_threshold && Current_Time < calibration_time) || (units_2 > unoccupied_threshold && units_4 > unoccupied_threshold && Current_Time < calibration_time)) {

    ocf1 = ocf1 + units_1;
    ocf2 = ocf2 + units_2;
    ocf3 = ocf3 + units_3;
    ocf4 = ocf4 + units_4;
    
    calibration_done = 1;
  }

  // If the previous part of the calibration is completed successfully, we can move on to fall detection [Only difference in conditions is time]

  else if ((units_1 > unoccupied_threshold && units_3 > unoccupied_threshold && Current_Time >= calibration_time && calibration_done == 1) || (units_2 > unoccupied_threshold && units_4 > unoccupied_threshold && Current_Time >= calibration_time && calibration_done == 1)) {

    //Since the occupied calibration factors are made from the average over 10000 ms, we need to divide by the time of a single sample (100 ms)
    //At this stage, the occupied calibration factors are still the sum of all the values over a space of 20s, so we have to find the average

    if (person_on == 0) { //Send all the calculated occupied thresholds to python once, after a person finishes their 30 second calibration time
      division_constant = calibration_time / sample;
      ocf1 = ocf1 / division_constant;
      ocf2 = ocf2 / division_constant;
      ocf3 = ocf3 / division_constant;
      ocf4 = ocf4 / division_constant;

      Serial.print(" ");Serial.print(ocf1); Serial.print(" "); Serial.print(ocf2); Serial.print(" "); Serial.print(ocf3); Serial.print(" "); Serial.print(ocf4);Serial.println();
      
      fall_threshold_1 = ocf1 * fall_factor_top;
      fall_threshold_2 = ocf2 * fall_factor_top;
      fall_threshold_3 = ocf3 * fall_factor_bot;
      fall_threshold_4 = ocf4 * fall_factor_bot;
      person_on = 1;
    }

    //Each fall detection algorithm includes a condition that checks whether a person has recently gotten up (within 10 secs), to avoid false positives
    
    if (units_1 > fall_threshold_1 && units_3 > fall_threshold_3 && units_2 < ocf2 && units_4 < ocf4 && (millis()-time_of_getting_up)>getting_up_tolerance) { //Falling from the Right Side of the bed
      person_is_falling_from_right = 1;
      tone(buzzer, 1500); //Send a 1000 Hz signal when a fall is detected
      Serial.print(" "); Serial.print(units_1); Serial.print(" "); Serial.print(units_2); Serial.print(" "); Serial.print(units_3); Serial.print(" "); Serial.print(units_4);
      Serial.println();
      Serial.println(time_of_getting_up);
      delay(10);
      noTone(buzzer); //Turn off the buzzer
    }

    if (units_2 > fall_threshold_2 && units_4 > fall_threshold_4 && units_1 < ocf1 && units_3 < ocf3 && (millis()-time_of_getting_up)>getting_up_tolerance) { //Falling from the Left Side of the bed
      person_is_falling_from_left = 1;
      tone(buzzer, 1500); //Send a 1000 Hz signal when a fall is detected
      Serial.print(" "); Serial.print(units_1); Serial.print(" "); Serial.print(units_2); Serial.print(" "); Serial.print(units_3); Serial.print(" "); Serial.print(units_4);
      Serial.println();
      Serial.println(time_of_getting_up);
      delay(10);
      noTone(buzzer); //Turn off the buzzer
    }

    //If neither fall detection or getting up is detected, then we assume person is sleeping safely
      person_is_falling_from_left = 0;
      person_is_falling_from_right = 0;
      person_has_gotten_up = 0;
      Serial.print(" "); Serial.print(units_1); Serial.print(" "); Serial.print(units_2); Serial.print(" "); Serial.print(units_3); Serial.print(" "); Serial.print(units_4);
      Serial.println();

      time_of_load_below_threshold = millis(); //If person moves slightly, then the last time normal readings were recorded are stored

      delay(5);
  }

  else { //If a person has gotten up and left the bed before the end of initial calibration, variables reset to be ready for next period of calibration
     if (units_1 <= unoccupied_threshold && units_2 <= unoccupied_threshold && units_3 >= unoccupied_threshold && units_4 >= unoccupied_threshold) { //If a person sits up, the time is recorded
          person_has_gotten_up = 1;
          time_of_getting_up = millis();
      }
    
      if ((millis() - time_of_load_below_threshold) > movement_tolerance_time) {
        Initial_Time = millis();
        ocf1 = 0; ocf2 = 0; ocf3 = 0; ocf4 = 0;
        person_on = 0;
        calibration_done = 0;
      }
  }

  while ((millis() - Previous_Time) < sample) { //Condition that standardizes execution times [currently 100 ms]
    delay(1);
  }

  Previous_Time = millis();
}
