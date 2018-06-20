#include <Wire.h>
#include <SFE_BMP180.h>
#include <EEPROM.h>

SFE_BMP180 pressure;

const int dawn = 50;
const int sun = 251;
const int light_time = 840;
const int night_time = 300;
const int added_time = 14;
const long int bright_check = 20000;
const long int one_sec = 1000;
const long int one_min = 60000;
const long int nine_min = one_min * 9;
const long int rest_of_min = one_min - bright_check - (one_sec*4);

int daycount,b1,b2,b3,b4,b5,brightness = 0;
int moisture,m1,m2,m3,m4,m5 = 0;
int maxmoisture = 0;
int light_test = 0;
boolean dayflag = false;  // flag for day and night
int power = 0;
int temp;
int temphi = 0;
int templow = 255;
int addr = 0;
int mins = night_time;
  
void setup() {
  Serial.begin(9600); // open serial port, set the baud rate as 9600 bps
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  digitalWrite(2,HIGH);
  digitalWrite(3,HIGH);
  if (pressure.begin())
    Serial.println("BMP180 init success");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.
    Serial.println("BMP180 init fail\n\n");
  }
}


//*****************************************************************************************************************************

void loop() {
  
  char status;
  double T,P,p0,a;
  Serial.println();
  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      // Print out the measurement:
      temp = (int)((9.0/5.0)*T + 32.5);
      Serial.print("temperature: ");
      Serial.print((9.0/5.0)*T+32.0,2);
      Serial.println(" deg F");
    }
  }
  b5 = b4;
  b4 = b3;
  b3 = b2;
  b2 = b1;
  b1 = 255 - (analogRead(0)/4);
  brightness = (b1+b2+b3+b4+b5)/5; //utilize a rolling average of light readings
  power = 255 - analogRead(1)/4; 
  m5 = m4;
  m4 = m3;
  m3 = m2;
  m2 = m1;
  m1 = 255 - (analogRead(2)/4);
  moisture = (m1+m2+m3+m4+m5)/5;  //utilize a rolling average of moisture readings
  if (temp < templow) {templow = temp;}
  if (temp > temphi) {temphi = temp;}
  if (moisture > maxmoisture) {maxmoisture = moisture;}
  Serial.print(dayflag);
  Serial.print(" minutes: ");
  Serial.println(mins);
  Serial.print("brightness: ");
  Serial.print(brightness); //print values to serial port
  Serial.print(" soil moisture: ");
  Serial.print(moisture);
  Serial.print(" soil max moisture: ");
  Serial.print(maxmoisture);
  Serial.print(" Low temp: ");
  Serial.print(templow);
  Serial.print(" High temp: ");
  Serial.println(temphi);

  if (dayflag == false)  //if it's night time 
  {
    if (brightness >= dawn) //and brightness is higher than dawn
    {
      dayflag = true; //set the system to day
      addr = addr + 4; //increment the memory pointer to the next record
      if (addr == 1024) {addr = 0;} //if the memory is full, move the pointer back to 0 and start over
      daycount++; //increment the day
      mins = light_time; //reset the minute counter
      temphi = 0; //reset the high temp
      templow = 255; //reset the low temp
      digitalWrite(2,LOW); //turn on the lights
      Serial.println("*daytime*");
    }
  }
  else //it's daytime 
  {
    if (brightness < sun)
    {
      digitalWrite(2,LOW);  //turn on the lights
      Serial.println("*lights on*");
    }
    else  //we're in the shade
    {      
      digitalWrite(2,HIGH); //turn the lights off
      Serial.println("*lights off*");
    }
  }
//  Serial.println("*passed day/night*");
  if (mins <= 0) //if the timer expires, test the ambient light to see if it's dark out
  {
    digitalWrite(2,HIGH); //turn the lights off
    Serial.println("*lights off*");
    delay(bright_check); //wait 20 seconds
    light_test = 0;
    light_test = light_test + 255 - (analogRead(0)/4);  //read the ambient light
    delay(one_sec);
    light_test = light_test + 255 - (analogRead(0)/4);  //read the ambient light
    delay(one_sec);
    light_test = light_test + 255 - (analogRead(0)/4);  //read the ambient light
    delay(one_sec);
    light_test = light_test + 255 - (analogRead(0)/4);  //read the ambient light
    delay(one_sec);
    light_test = light_test + 255 - (analogRead(0)/4);  //read the ambient light
    light_test = light_test / 5;  //take an averaged  light reading
    Serial.print(" current brightness: ");
    Serial.println(light_test);
    if (light_test > dawn)  //if it's not totally dark
    {
      digitalWrite(2,LOW); //turn the lights on
      mins = mins + added_time; //add time
      Serial.println("*added time*"); //notify condition via serial
      delay(rest_of_min);
    }
    else //it's dark out
    {
      dayflag = false; //set the system to night
      digitalWrite(2,HIGH); //turn off the lights and water
      digitalWrite(3,HIGH);
      EEPROM.write(addr,daycount);  //write the day of the month in EEPROM
      EEPROM.write(addr+1,temphi); //write the high temp of the day
      EEPROM.write(addr+2,templow); //write the low temp of the day
      EEPROM.write(addr+3,maxmoisture); //write the moisture of the day     
      Serial.println("*nighttime*");
      mins = night_time;
      delay(nine_min);  //delay 9 min
    }  
  }
  delay(one_min); //delay 1 min
  mins = mins - 1; //decrement the timer
}
