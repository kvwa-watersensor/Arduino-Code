/*Includes*/

#include <SPI.h>
#include <SD.h>
#include <math.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>
#include <Time.h>

/* Global Variable Section */
String dataString = "";
const int chipSelect = 10;
float batt_v = 0;
float panel_v = 0;
double flow = 0;
double depth = 0;
int height = 0;

/*Configure Screen*/
// set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27,20,4); 

void setup() {
  //All Time Taken as of the start of this year
   setTime(00,00,00,01,01,2018);  
  //Initialize Screen
  lcd.init();            
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  visible("Hello","Initialising  SD");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    visible("Card Failed!","Please Reset");
    // don't do anything more:
    while (1);
  }
  visible("End Setup","Card Initialised");
}


void visible(String string_1,String string_2){
  int len_1 = (16- string_1.length())/2;
  int len_2 = (16- string_2.length())/2;
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(len_1,0);
  lcd.print(string_1);
  lcd.setCursor(len_2,1);
  lcd.print(string_2);
}

void height_search(){  
  double heights[17] = {0,0.01,0.065,0.1225,0.18,0.20,0.21,0.22,0.23,0.24,0.25,0.26,0.27,0.28,0.29,0.30,0.35};
  height = 0;
  uint8_t a_0 = 0;
  uint8_t a_1 = 0;
  int i = 0;
  analogWrite(3,0); //Strobe Low
  analogWrite(5,a_0); //Address 0
  analogWrite(6,a_1); //Address 1  
     //Check Mid
     if (digitalRead(7)==1){
      //Check for Upper Range
      height = 16; 
      for (i = 0; i < 4; i++){
        //Change Address
        if (i == 0){
          a_0 = 0;
          a_1 = 0;
          }
        if (i == 1){
          a_0 = 255;
          a_1 = 0;
          }
         if (i == 2){
          a_0 = 0;
          a_1 = 255;
          }
         if (i == 3){
          a_0 = 255;
          a_1 = 255;
         }
         analogWrite(5,a_0);
         analogWrite(6,a_1);
         if (digitalRead(7) ==0){
          height--;
          }
          if (digitalRead(8)==0){
            height--;
          }
         delay(1000); 
         }
     }
     else {
        //Check for Lower Range
        height = 8; 
        for (i = 0; i < 4; i++){
          //Change Address
          if (i == 0){
            a_0 = 0;
            a_1 = 0;
          }
          if (i == 1){
           a_0 = 255;
           a_1 = 0;
          }
          if (i == 2){
           a_0 = 0;
           a_1 = 255;
          }
           if (i == 3){
           a_0 = 255;
           a_1 = 255;
          }
          analogWrite(5,a_0);
          analogWrite(6,a_1);
          if (digitalRead(2) ==0){
            height--;
          }
          if (digitalRead(4)==0){
            height--;
         }
         delay(1000);  
        }

          
     }
     depth = heights[height];   
     flow = gvf_algorithm(depth);
     Serial.println(height);
     Serial.println("Water Height");
     Serial.println(depth);
     Serial.println("Water Depth /m");
     Serial.println(flow,8);
     Serial.println("Flow");
     //remember to pull strobe low
     String out_1 = String(height);
     out_1 += " level";
     String out_2 = String(flow,9);
     out_2 += " m3/s";
     visible(out_1,out_2);
}

void volt_mon(){
  //Get Panel Voltage
  int panel = analogRead(0);
  //Get Battery Voltage
  int batt = analogRead(1);
  //Calibrate of ADC range of 0-1000
  panel_v = panel/57.89;
  batt_v = batt/57.67;
  //Charge Cutout
  if (batt >= 778){
    analogWrite(9,255); //Set Zener to zero
  }
  else{
    analogWrite(9,0); //Leave on Charge
  }
}

double gvf_algorithm(double depth){
/*Channel Parameters in cm*/
double width = 0.076; //std 36.50 test 7.6
double n = 0.009; //std 0.0155 test 0.009 
double sin_theta = 0.005061; //std 0.003734 test 0.005061
double weir_height = 0.0;
depth = depth - weir_height;//std 0

/*Get Hydraulic Radius*/
double D_h = ((width*depth)/(width + (2*depth)));
//Serial.println(D_h);
/*Manning's Equation*/
double chezy = (pow(D_h,1/6)/n);
/*Get cross sectional area*/
double A = (width*depth);
//Serial.println(A);
/*Get Velocity*/
double V = (chezy*sqrt(D_h*sin_theta));
//Serial.println(V);
/*Get Approximate Flow*/
flow = V*A;
return flow;
}

void card_write(){
  /*
  SD card datalogger
  
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

 created  24 Nov 2010
 modified 9 Apr 2012
 by Tom Igoe
 */
/*Compile the data string*/
  dataString += String(month());
  dataString += String(" ");
  dataString += String(day());
  dataString += String(" ");
  dataString += String(hour());
  dataString += String(" ");
  dataString += String(minute());
  dataString += String(" ");
  dataString += String(second());
  dataString += String(" Time mm dd hr min sec ");
  dataString += String(depth);
  dataString += String(" Water Depth/m ");
  dataString += String(flow,16);
  dataString += String(" Water Flow/m^3/s^-1 ");
  dataString += String(panel_v,3);
  dataString += String(" Panel Voltage/V ");
  dataString += String(batt_v,3);
  dataString += String(" Battery Voltage/V ");  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    visible("Oops!","error datalog");
  }
  dataString = "";
}


void timer(){
  int t_m = minute();
  int t_s = second();
  if (t_m==00 && t_s>50) {
    height_search();
    card_write();
  }
}

void screen_management(){
int  t = second();
  if (t==75){
  //Display Section
  String out_1 = String(panel_v,3);  
  out_1 += " Panel/V";
  String out_2 = String(batt_v,3);
  out_2 += " Batt/V"; 
  visible(out_1,out_2);
  delay(1000); //prevent overwriting
  }
  if (t>0){
   String out_1 = String(height);
   out_1 += " level";
   String out_2 = String(flow,7);
   out_2 += " m3/s";
   visible(out_1,out_2);
   delay(1000); //prevent overwriting
   }
}

void loop() {
  Serial.println("Hello, we've got this far...");
  
  //timer();
  height_search(); 
  //volt_mon();
  //screen_management();
  card_write();
}

