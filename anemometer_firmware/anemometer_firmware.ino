#include <FIR.h>

#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 1024

#define SEC_WIND_AVERAGE 5 // Number of second wind average reported

uint32_t rising_counter = 0; // Counts the number of rising pulses

// Function prototypes
void startTimer(int frequencyHz);
void setTimerFrequency(int frequencyHz);
void TC3_Handler();

// Make an instance of the FIR filter. In this case it will be
// an N point moving average
FIR<float, SEC_WIND_AVERAGE> fir;

int filtered_frequency;

void setup() {
  // Setup FIR Filter 
  float fir_coef[SEC_WIND_AVERAGE];
  memset(fir_coef, 1, sizeof(fir_coef));
  fir.setFilterCoeffs(fir_coef);
  
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(6), isr_pin_rising, RISING);
  startTimer(1);

}

void loop() {
 delay(1000);
 Serial.print(filtered_frequency);
 Serial.print(" - ");
 Serial.println(frequency_to_windspeed(filtered_frequency));
}

float frequency_to_windspeed(uint32_t frequency)
{
  /*
   * This function turns the frequency into the windspeed using the
   * calibration coefficients for the given sensor. Currently supports
   * up to quadratic calibration curves.
   */
  float a = -1.99e-5;
  float b = 0.038;
  float c = 1.2918;
  return a * frequency * frequency + b * frequency + c;
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

void setTimerFrequency(int frequencyHz) {
  /*
   * Setup the frequency at which our timer event fires
   */
  int compareValue = (CPU_HZ / (TIMER_PRESCALER_DIV * frequencyHz)) - 1;
  TcCount16* TC = (TcCount16*) TC3;
  // Make sure the count is in a proportional position to where it was
  // to prevent any jitter or disconnect when changing the compare value.
  TC->COUNT.reg = map(TC->COUNT.reg, 0, TC->CC[0].reg, 0, compareValue);
  TC->CC[0].reg = compareValue;
  while (TC->STATUS.bit.SYNCBUSY == 1);
}

void startTimer(int frequencyHz) {
  /*
   * Start a timer with the specified frequency. This calls setTimerFreuqency for you.
   */
  REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC2_TC3) ;
  while ( GCLK->STATUS.bit.SYNCBUSY == 1 ); // wait for sync

  TcCount16* TC = (TcCount16*) TC3;

  TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Use the 16-bit timer
  TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Use match mode so that the timer counter resets when the count matches the compare register
  TC->CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Set prescaler to 1024
  TC->CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  setTimerFrequency(frequencyHz);

  // Enable the compare interrupt
  TC->INTENSET.reg = 0;
  TC->INTENSET.bit.MC0 = 1;

  NVIC_EnableIRQ(TC3_IRQn);

  TC->CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync
}

void TC3_Handler() {
  /*
   * ISR handler for the timer event. Fires when the compare register matches the timer count.
   */
  TcCount16* TC = (TcCount16*) TC3;
  if (TC->INTFLAG.bit.MC0 == 1) {
    TC->INTFLAG.bit.MC0 = 1;

    // ISR Code
    filtered_frequency = fir.processReading(rising_counter);
    rising_counter = 0;
  }
}
