
// Pete Wentworth, December 2019
// A playpen to sense, and make sense of, output from an SCT-013-030 Current Clamp Reader

#include "ledOutput.h"
#include "Utils.h"
#include "WaveIntegrator.h"

const int mainsFrequency = 50;
const int numSamplesPerCycle = 100;
long timeBetweenClampSamples = 
                  (long) ((1000000.0 / mainsFrequency) / numSamplesPerCycle);
long nextSampleDue = 0;        // The timing here is in microsecs


// Set this to the midpoint of the ADC sample range for your hardware.
const int zeroReadingGuess = 510;

WaveIntegrator theIntegrator(numSamplesPerCycle, zeroReadingGuess);


int cyclesUntilNextReport = 0;        // This counts down on each completed mains cycle, till the report becomes due.

const int maxIntervalSteps = 7; // A little table that lets us interactively change reporting frequency to every cycle, each half second, each second, etc...
int cyclesBetweenReports [maxIntervalSteps] = { 1, mainsFrequency / 2, mainsFrequency * 1, mainsFrequency * 2, mainsFrequency * 10, mainsFrequency * 30, mainsFrequency * 60 };

int reportingIndex = 3;         // We'll start off at one report every two seconds.

int reportingStyle = 2;         // How verbose do you want your reporting output to be?

bool rawMode = false;           // Don't interpret or analyze any data - just show raw ADC readings.

bool extraInfo = false;         // Experimental / diagnostic

// In a reporting period (which can be say 50 mains cycles or 5000 mains cycles)
// we sum up data from each cycle and report a smoothed average.

double sumOfHighPeaks, sumOfLowPeaks, sumOfHighHalf, sumOfLowHalf, sumOfBothHalves;

void resetForNextCycle() {
  cyclesUntilNextReport = cyclesBetweenReports[reportingIndex];
  sumOfHighPeaks = 0;
  sumOfLowPeaks = 0;
  sumOfBothHalves = 0;
  sumOfLowHalf = 0;
  sumOfHighHalf = 0;
  sumOfBothHalves = 0;
}

void setup() {
  Serial.begin(115200);
  ledBegin();
  
  // Make the ADC more sensitive, based on AREF voltage which should be 1.2V.
  // This also puts the said 1.2V onto the AREF pin, so we can use that to bias the
  // clamp's midpoint voltage.
  analogReference(INTERNAL);
  pinMode(A0, INPUT);

  findBestFitStraightLine();

  resetForNextCycle();
}


void showHelp()
{
  sayf("SCT-013-030 Current Clamp Reader (%s %s)\n%s\n", __DATE__, __TIME__, __FILE__);
  Serial.println("?   -  this help");
  Serial.println("0   -  no reporting");
  Serial.println("1   -  show waveArea? value (averaged over reporting cycles)");
  Serial.println("2   -  show above with Watts (averaged over reporting cycles)");
  Serial.println("3   -  show more detail with half-cycles, etc");
  Serial.println("+   -  increase reporting interval: 1/f, 0.5sec, 1sec, ... ");
  Serial.println("-   -  decrease reporting interval");
  Serial.println("r   -  toggle RAW mode - show analog sensor reading, nothing else");
  sayf("Mains freq=%dHz.  Samples per cycle=%d.  zeroVal = %f.\n",
       mainsFrequency,  numSamplesPerCycle, theIntegrator.zeroVal);
}

void processInputFromUser()
{
  // This software is really a lab playpen, so having infrastructure to let 
  // the user interactively tweak things is useful.
  
  int n;
  if (Serial.available() > 0) {
    n = Serial.read();
    switch (n) {

      case '?': case 'h': showHelp();  break;
      case 'r': rawMode = !rawMode;  break;
      case 'x': extraInfo = !extraInfo; break;
      case '0': reportingStyle = 0;  break;
      case '1': reportingStyle = 1;  break;
      case '2': reportingStyle = 2;  break;
      case '3': reportingStyle = 3;  break;
      case '4': reportingStyle = 4;  break;

      case '+' : if (reportingIndex + 1 < maxIntervalSteps) reportingIndex++;
        resetForNextCycle();
        break;

      case '-' : if (reportingIndex > 0) reportingIndex--;
        resetForNextCycle();
        break;
    }
  }
}

void loop() {

  // This "polled" style of main loop is similar to by Microsoft's XNA framework.
  // I don't generally use delay() or interrupts or timers.  The main loop runs
  // as fast as it can, gets the current time (as accurately as it can), and
  // keeps track of when things are due to happen.  It makes it easier to 
  // add other stuff like "sample the temperature every minute" or sample the 
  // mains voltage.

  long timeNow = micros();

  if (timeNow >= nextSampleDue) {

    nextSampleDue += timeBetweenClampSamples;
 
    int val;
    // Take the sample, with a couple of ADC reads.
    // Some reports say that changing the ADC channel (say A0 to A1)
    // sometimes gives a false reading on the first sample. 
    val = analogRead(A0);
    val = analogRead(A0);

    if (rawMode) {
      Serial.println(val);
    }
    else {

      bool cycleComplete = theIntegrator.addReading(val);

      if (cycleComplete) {

        if (extraInfo) {  // playpen stuff
          sayf("Extra: loSum=%f  hiSum=%f loPeak=%f   hiPeak=%f\n",
               theIntegrator.lowHalf,   theIntegrator.highHalf,
               theIntegrator.lowPeak,   theIntegrator.highPeak);
        }

        sumOfBothHalves += theIntegrator.bothHalves;

        // more diagnostic stuff:
        sumOfHighHalf += theIntegrator.highHalf;
        sumOfLowHalf += theIntegrator.lowHalf;
        sumOfHighPeaks += theIntegrator.highPeak;
        sumOfLowPeaks += theIntegrator.lowPeak;

        if (--cyclesUntilNextReport <= 0) {

          cyclesUntilNextReport = cyclesBetweenReports[reportingIndex];
          double waveArea = sumOfBothHalves / numSamplesPerCycle / cyclesUntilNextReport;

          // The numbers we get are pretty meaningless, until we convert them to
          // a watts reading.  But the important thing is they must change, and be
          // sensitive, to changes in the load.  This seems to do the trick reasonably.

          double highHalfAvg = sumOfHighHalf / numSamplesPerCycle / cyclesUntilNextReport;
          double lowHalfAvg =  sumOfLowHalf / numSamplesPerCycle / cyclesUntilNextReport;
          long highPeakAvg = (long ) (sumOfHighPeaks / cyclesUntilNextReport);
          long lowPeakAvg = (long ) (sumOfLowPeaks / cyclesUntilNextReport);
          long peakToPeak = highPeakAvg - lowPeakAvg;
          

          resetForNextCycle();

          double watts1 = toWatts(waveArea);
          double watts2 = toWatts2(waveArea);

          switch (reportingStyle) {

            case 0: break;
            case 1:
              sayf("%f\n", waveArea);
              break;
            case 2:
              sayf("waveArea: %f --> %f Watts\n",  waveArea, watts1);
              displayDouble(watts1, 6, 2, 2); 
              break;
            case 3:
              sayf("two halves: %0.3f %0.3f (lopsided by %0.3f) zeroVal=%0.4f   waveArea=%0.3f --> %f Watts\n",
                   lowHalfAvg,  highHalfAvg , lowHalfAvg + highHalfAvg,  theIntegrator.zeroVal, waveArea, watts1 );
              break;

            case 4:
              sayf("waveArea: %f --> %f Watts (or %f Watts)\n",  waveArea, watts1, watts2);
              break;
          }
        }
      }
    }
  }

  processInputFromUser();

}
