#include <FIR.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <RH_RF95.h>
#include "RTClib.h"

#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 1024

#define SEC_WIND_AVERAGE 5 // Number of second wind average reported

#define SD_CHIP_SELECT 10
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 900.0 //change this depending on what mini-GNI + control station pair

// Calibration constants for the anemometer
const float anemometer_calibration_coeff_a = 0;
const float anemometer_calibration_coeff_b = 0.02468637;
const float anemometer_calibration_coeff_c = 1.818727;

// Function prototypes
void TC5_Handler(void);
bool tcIsSyncing();
void tcStartCounter();
void tcReset();
void tcDisable();
void tcConfigure(int);
void isr_pin_rising();

// Make an instance of the FIR filter. In this case it will be
// an N point moving average
FIR<float, SEC_WIND_AVERAGE> fir;

RTC_PCF8523 rtc;
RH_RF95 rf95(RFM95_CS, RFM95_INT);
File logfile;
char logfile_name[13];
uint32_t rising_counter = 0; // Counts the number of rising pulses
int filtered_frequency; // FIR filtered anemometer frequency


void send_windspeed_on_lora(float windspeed)
{
  // Select LoRa
  digitalWrite(SD_CHIP_SELECT, HIGH);
  digitalWrite(RFM95_CS, LOW);
  
  // Send the scaled windspeed (windspeed * 10) via the LoRA radio
  char buffer[5] = "";
  sprintf(buffer, "%.1f\r\n", windspeed);
  rf95.send((uint8_t *)buffer, 5);
  delay(10);
  rf95.waitPacketSent();

  // De-select LoRa
  digitalWrite(RFM95_CS, HIGH);
  digitalWrite(SD_CHIP_SELECT, LOW);
}

void get_log_file_name(char *filename)
{
  /*
  Format is YYMMDDHH.DAT
  YY  - Year
  MM - Month
  DD - Day
  HH  - Hour
  */

  DateTime now = rtc.now();

  strcpy(filename, "00000000.TXT");

  // Set the Year
  filename[0] = '0' + (now.year() / 10) % 10;
  filename[1] = '0' + now.year() % 10;

  // Set the Month
  filename[2] = '0' + (now.month() / 10) % 10;
  filename[3] = '0' + now.month() % 10;

  // Set the Day
  filename[4] = '0' + (now.day() / 10) % 10;
  filename[5] = '0' + now.day() % 10;

  // Set the hour
  filename[6] = '0' + (now.hour() / 10) % 10;
  filename[7] = '0' + now.hour() % 10;
}


bool check_and_initialize_file(char *fname)
{
  /*
   * Check to see if the filename already exists on the SD card. If not, create
   * it and write the header.
   */
   if (!SD.exists(fname)){
     logfile = SD.open(fname, FILE_WRITE);
     if (!logfile)
     {
       Serial.println("Cannot open/create log file.");
     }

     logfile.write("mini-GNI Anemometer Log\r\n");
     logfile.write("Anemometer calibration coefficients: ");
     char buffer[100] = "";
     sprintf(buffer, "a=%f\tb=%f\tc=%f\r\n");
     logfile.write(buffer);
     logfile.close();
   }
}


float frequency_to_windspeed(float a, float b, float c, uint32_t frequency)
{
  /*
   * This function turns the frequency into the windspeed using the
   * calibration coefficients for the given sensor. Currently supports
   * up to quadratic calibration curves.
   */
   if (frequency == 0)
   {
    return 0;
   }
  return a * frequency * frequency + b * frequency + c;
}


void setup()
{
  // Setup serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("Mini-GNI Anemometer");

  // Setup FIR Filter
  float fir_coef[SEC_WIND_AVERAGE];
  memset(fir_coef, 1, sizeof(fir_coef));
  fir.setFilterCoeffs(fir_coef);

  // Setup the RTC and set the datetime if it wasnt initializaed yet
  if (! rtc.begin())
  {
    Serial.println("Cannot Start RTC");
    while (1);
  }

  if (! rtc.initialized())
  {
    Serial.println("RTC not initialized - setting to compile date/time.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Setup interrupts and timer for anemometer measurement
  attachInterrupt(digitalPinToInterrupt(6), isr_pin_rising, RISING);
  tcConfigure(1024); //configure the timer to run at 1 Hertz (1024 counts)
  tcStartCounter(); //starts the timer

  // LoRA Radio Setup
  pinMode(RFM95_RST, OUTPUT);
  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  if(!rf95.init())
  {
    Serial.println("Radio Initialization Error");
  }
  if (!rf95.setFrequency(RF95_FREQ))
  {
    Serial.println("Cannot set radio module frequency");
  }
  digitalWrite(RFM95_CS, HIGH);

  // Start SD Card
  if (!SD.begin(SD_CHIP_SELECT))
  {
      Serial.println("Card init. failed!");
  }

  // Open up the logfile for this datetime and write header
  get_log_file_name(logfile_name);
  logfile = SD.open(logfile_name, FILE_WRITE);
  logfile.print("Datetime\tFrequency\tWindspeed\r\n");
  logfile.print("Timestamp\tHz\tm/s\r\n");
  logfile.close();
}

void loop()
{
  // Main loop:
  // * Get the measured filtered anemometer frequency.
  // * Calibrate frequency to physical units.
  // * Send the data over serial and log to file.
  // * Every 30 packets, send one via LoRA.

  static int radio_message_counter = 30;

  // Get the measured filtered anemometer frequency.
  int anemometer_frequency = filtered_frequency;
  DateTime now = rtc.now();

  // Calibrate frequency to physical units.
  float anemometer_windspeed = frequency_to_windspeed(anemometer_calibration_coeff_a,
                                                     anemometer_calibration_coeff_b,
                                                     anemometer_calibration_coeff_c,
                                                     filtered_frequency);

  // Send data over serial and log to file.
  char message[50] = "";
  sprintf(message, "%02d/%02d/%04dT%02d:%02d:%02d\t%d\t%f\r\n", now.month(), now.day(), now.year(),
          now.hour(), now.minute(), now.second(), filtered_frequency, anemometer_windspeed);


 // Write message to file
 logfile = SD.open(logfile_name, FILE_WRITE);
 logfile.print(message);
 logfile.close();

 // Echo to serial terminal (optional)
 Serial.print(message);
  
 // Send message via radio every 30th message
 if (29 <= radio_message_counter)
 {
   // Time to send the packet and reset
   Serial.println("Sending via LORA.");
   send_windspeed_on_lora(anemometer_windspeed);
   radio_message_counter = 0;
 }
 radio_message_counter ++;

 delay(1000);
}




/*
 * Timer/Interrupt helpers, ISRs - There should be no need to modify any of this
 */

void isr_pin_rising(){
  /*
   * ISR for counting rising events
   */
  rising_counter++;
}


void TC5_Handler (void) {
  //Serial.println(millis());  // Want to check that this really runs at 1 Hz?
  filtered_frequency = fir.processReading(rising_counter);
  rising_counter = 0;
  TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}


/* 
 *  TIMER SPECIFIC FUNCTIONS FOLLOW
 *  you shouldn't change these unless you know what you're doing
 */

//Configures the TC to generate output events at the sample frequency.
//Configures the TC in Frequency Generation mode, with an event output once
//each time the audio sample frequency period expires.
 void tcConfigure(int sampleRate)
{
 // Enable GCLK for TCC2 and TC5 (timer counter input clock)
 GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
 while (GCLK->STATUS.bit.SYNCBUSY);

 tcReset(); //reset TC5

 // Set Timer counter Mode to 16 bits
 TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
 // Set TC5 mode as match frequency
 TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
 //set prescaler and enable TC5
 TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024 | TC_CTRLA_ENABLE;
 //set TC5 timer counter based off of the system clock and the user defined sample rate or waveform
 TC5->COUNT16.CC[0].reg = (uint16_t) (SystemCoreClock / sampleRate - 1);
 while (tcIsSyncing());
 
 // Configure interrupt request
 NVIC_DisableIRQ(TC5_IRQn);
 NVIC_ClearPendingIRQ(TC5_IRQn);
 NVIC_SetPriority(TC5_IRQn, 0);
 NVIC_EnableIRQ(TC5_IRQn);

 // Enable the TC5 interrupt request
 TC5->COUNT16.INTENSET.bit.MC0 = 1;
 while (tcIsSyncing()); //wait until TC5 is done syncing 
} 

//Function that is used to check if TC5 is done syncing
//returns true when it is done syncing
bool tcIsSyncing()
{
  return TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY;
}

//This function enables TC5 and waits for it to be ready
void tcStartCounter()
{
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE; //set the CTRLA register
  while (tcIsSyncing()); //wait until snyc'd
}

//Reset TC5 
void tcReset()
{
  TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (tcIsSyncing());
  while (TC5->COUNT16.CTRLA.bit.SWRST);
}

//disable TC5
void tcDisable()
{
  TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (tcIsSyncing());
}
