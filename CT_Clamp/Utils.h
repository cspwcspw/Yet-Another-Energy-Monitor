 
#pragma once


// Pete Wentworth, December 2019


// I'm a fanboy of printf and doubles, and anti-fanboy
// of cramping my style on a Uno. However ...
// Here is my own lightweight minimal workaround for the Uno.
// It does very simple format string interpretation,
// recognizing only text and special conversion for
// %d  %ld  %[.d]f  %c  %s  %%
// floats always get a fixed number of decimal places.

void sayf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int count = 0;
  while (*fmt != '\0') {
    count++;
    // Serial.print(count); Serial.print(" "); Serial.println(*fmt);
    if (*fmt == '%') {
      ++fmt;
      if (*fmt == '%') {
        Serial.print('%');
      }
      else  if (*fmt == 'd') {
        int iv = va_arg(args, int);
        Serial.print(iv);
      }
      else if (*fmt == 'l') {
        ++fmt; // assume it says %ld
        long lv = va_arg(args, long);
        Serial.print(lv);
      }
      else if (*fmt == 'c') {
        char cv = va_arg(args, int);
        Serial.print(cv);
      }
      else if (*fmt == 's') {

        char *sv = va_arg(args, char *);
        Serial.print(sv);

      }
      else // we only allow [[width].[precision]f
        // where both width and precision can only be single digits
      {
        int width = 0; // default

        if (isDigit(*fmt)) // we have a width
        {
          width = *fmt - '0';
          ++fmt;
        }
        int precision = 2; // default
        if (*fmt == '.') // we have a precsion
        { ++fmt;
          precision = *fmt - '0';
          ++fmt;
        }
        if (*fmt == 'f') {
          double dv = va_arg(args, double);
          char buf[15];
          dtostrf(dv , width, precision, buf);
          Serial.print(buf);
        }
      }
    }
    else // it is just a standard non-escaped character, echo it to output
    {
      Serial.print(*fmt);
    }
    ++fmt;
  }
  va_end(args);
}





// Linear interpolate readings to Watts.
// Using a mapping table lets us cater for non-linearity in the ADC conversions,
// or in the sensor itself.  So to calibrate, you need to have some
// independent way of accurately measuring the watts.  Then fill numbers
// into the table below.  The first and last entry are chosen so that
// we cannot fall off the end of the table when searching.


const int numRefPts = 20;  // each x,y takes two slots in the array
const double xs[numRefPts] =  {
  // reading -> watts
  -0.001,   2.2,     // dummy first table entry, calculated
  0.11,   2.2,
  1.57,   10.5,
  2.88,  28,
  4.71, 54.7,
  6.6, 59,
  120.41, 1050,
  180,  1750,
  280,  2750,
  2800, 27500    // dummy last table entry, calculated
};


double toWatts(double x)
{
  int p = 2;
  while (xs[p] <= x) p += 2; // find first position of even p so that xs[p] is bigger than x.
  // The value we want lies between indexes p-2 and p in our table.   So interpolate.
  double f = (double)(x - xs[p - 2]) / (double)(xs[p] - xs[p - 2]); // 0 <= f < 1
  double result = xs[p - 1] + f * (xs[p + 1] - xs[p - 1]);

  return result;
}

// Find straight line through the XY points, using
// https://www.varsitytutors.com/hotmath/hotmath_help/topics/line-of-best-fit

double slopeM;
double interceptB;

void findBestFitStraightLine()
{
  // Ignore dummy points on ends of table, and account for fact that
  // x and y points are interleaved in a single table.
  double xMean = 0;
  double yMean = 0;
  for (int i = 2; i < numRefPts - 2; i = i + 2) {
    xMean += xs[i];
    yMean += xs[i + 1];
  }
  int n = (numRefPts - 4) / 2;
  xMean = xMean / n;
  yMean = yMean / n;
  double sumDeviations = 0;
  double sumXDiffSq = 0;
  for (int i = 2; i < numRefPts - 2; i = i + 2) {
    double xDiff = (xs[i] - xMean);
    sumDeviations += xDiff * (xs[i + 1] - yMean);
    sumXDiffSq += xDiff * xDiff;
  }

  slopeM = sumDeviations / sumXDiffSq;
  interceptB = yMean - slopeM * xMean;

  sayf("Calculated m = %f, b=%f\n", slopeM, interceptB);
}

double toWatts2(double x)  // use straight line assumption
{
  return interceptB + slopeM * x;
}
