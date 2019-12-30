
#pragma once

// Analyze a mains-derived sine wave, one cycle at a time.
// Intended usage: to estimate power usage, both by finding peaks
// during each mains cycle, and by integrating the area under the wave.
// Devices that chop or PWM the A/C wave (leading or trailing edge dimmers, the triac
// in a temperature-adjustable soldering iron, etc.) are should get more accurate
// results if we integrate rather than just do peak detection, because the wave
// we see is no longer a pure sine wave/

// Pete Wentworth, December 2019

class WaveIntegrator
{
  private:

    int readingsInCycle;      // determined by main program
    int numReadingsToGo;      // counts down to 0, when the A/C cycle is complete.
    bool cycleDataReady;
    double longTermErrorEstimate = 0;

  public:

    double zeroVal;           // estimate of ADC reading when no A/C wave is present.
    double highHalf;          // sum of each  (ADC - zeroVal) which is positive
    double lowHalf;           // sum of each  (ADC - zeroVal) which is negative
    double bothHalves;        // combined halves with balancing, set at end of cycle
    double highPeak;          // max of (ADC - zeroVal), should be a positive value
    double lowPeak;           // min of (ADC - zeroVal), should be a negative value

  private:
    void reset() {
      highHalf = 0;
      lowHalf = 0;
      highPeak = 0;
      lowPeak = 0;
      numReadingsToGo = readingsInCycle;
    }

  public:

    WaveIntegrator(int readingsPerCycle, int initialZeroVal)
    { readingsInCycle = readingsPerCycle;
      zeroVal = initialZeroVal;
    }

    // Returns true if data from the cycle is ready, false otherwise
    bool addReading(int ival) {

      if (numReadingsToGo <= 0) reset();

      double dval = ival - zeroVal;
      if (dval > 0) {
        highHalf += dval;
        if (dval > highPeak) {
          highPeak = dval;
        }
      }
      else if (dval < 0) {  // process the negative half of the sine wave
        lowHalf += dval;
        if (dval < lowPeak) {
          lowPeak = dval;
        }
      }

      if (--numReadingsToGo <= 0) {  // Don't clear values until the caller has seen them.

        bothHalves = highHalf - lowHalf;    // Sum of two halves of the wave.

        // If zeroValue is slightly off, the low and high sums over the
        // cycle become unbalanced.  In an ideal world, we should get highHalf + lowHalf == 0.
        // Any difference is the error component or noise.

        double err = highHalf + lowHalf;
        // Slowly drift our assummed zeroVal in respose to the errors we see.
        // This should let the system learn the midpoint dynamically.
        // So the software will compensate for balance resistors in the circuit
        // that are not precidely matched.
        zeroVal += (err / readingsInCycle) * 0.02;
        
        return true;
      }
      return false;  // no results for this cycle yet.
    }
};
