# ESP32_Energy_Monitor

##  Sensing, and making sense of a CT Clamp SCT-013-030

The current sensing clamp I have is a Banggood device, 
http://statics3.seeedstudio.com/assets/file/bazaar/product/101990028-SCT-013-030-Datasheet.pdf 

If we believe these device specs (I am somewhat skeptical), the response is
1V per 30Amps.  Is this 1V peak-to-peak, Amplitude, Average, or RMS?  

Rather than start from https://openenergymonitor.org/forum-archive/node/225.html,
it seemed like an interesting idea to try to figure things out "from scratch".  If
nothing else, this approach will allow me to better appreciate how others have 
tackled this problem.

After a first look I thought 

1. The clamp is less sensitive than the specs tell.  Based on my tests 
in the next section, I estimate sensitivity at 19.5mV per amp, so 30 amps
will produce 583mv RMS rather than 1V.  The upside is we could probably measure more mains current.

2. Small A/C signals have some pitfalls for this kind of DIY project. A sensible approach would
be to amplify the sensor output through an op-amp with high input impedance, making it easier to
deal with.   But one would need an opamp, and a dual rail power supply (say +5V, -5V).  And that
doesn't seem to fit the spirit of a DIY project based around an ESP-32 or an Arduino.  

3. The signal is not an arbitrary A/C signal with high frequencies. 
It is the output induced by
a 50Hz or 60Hz mains sine wave.  This low frequency wave is hugely to our advantage. 
Much of the
AC amplifier theory is based on handling high frequencies and 
faithful waveform amplification.  Neither is important here.   

4.  I need to set some limits of what I am trying to do.  How little 
current should I be able to sense?  How much?  At what resolution?   


My medium value load is a domestic oil heater with two separately switched elements, nominally 1000w and 1500w at 220v. 
To this I could add other domestic resistive loads: a toaster, or a clothes iron. 
My mains is 240V, 50 Hz. 

My low power load is a 20 Watt incandescent lamp.  I measured the resistance, 
but whoa, I learned something interesting!  My calculations said it would consume 
more than 200Watts.  Turns out this is not so.  Tungsten filaments increase their 
resistance as they get hot. So, in this case, "Resistance is Futile". 
https://www.wired.com/story/need-an-ohms-law-party-trick-take-a-lightbulbs-temperature/


* 20W bulb:                  
* elem1:  1000w nominal, r1=46.7 ohms measured resistive load.  
* elem2:  1500w nominal, r2=28.6 ohms measured resistive load.  
* both :  2500w nominal, rb=17.8 ohms measured resistive load.

The theory says we should have r = r1.r2/(r1+r2) = 17.73 ohms. Close enough!  
So my real power at 240V RMS mains in each element is V*V / R

* elem1 = 1233W
* elem2 = 2014W
* both  = 3235W  ?? 

 Load   | Expected   Multimeter AC   Scope       RMS =.707xAmp   Average V calc
       current    RMS reading    Amplitude   (calculated)    = 0.637xAmp
 
lamp20              1.5mV
lamp60              5.0mV
elem1   5.14A     112 mV           156mV        110mV            99mv
elem2   8.39A     176 mV           250mV        177mV           159mV
both   13.53A     271 mV           388mV        274mV           247mv

My (oldish) house wiring and my extension leads have some additional 
supply resistance. The 240V voltage in the house falls off considerably
under high load, which accounts 
for the fact that both elements on together use a bit less than the
sum of each one on independently.   

So by my calculations and measurements, the sensor is less 
sensitive than claimed 19.5mV RMS per amp. If so, 30 Amps will produce 583mV RMS.


## Strategy

I took a wrong turn, believing I needed to amplify the voltage from my sensor.  So
I dug a little into single transistor Common Emitter Amplifier design.  I didn't think I needed to
preserve both halves of the A/C cycle, so I deliberately mis-biased the transistor
to clip one half of the wave, in a trade-off that gave me more gain for the other
half of the wave.  With some theory from https://www.nutsvolts.com/magazine/article/bipolar_transistor_cookbook_part_3 (Figure 15) 
and some modelling help from LTSpice, I modelled and built this amplifier:  It inverts its
input signal, but that shouldn't bother our attempt to find peaks or areas under the one half of the curve.    

![simulatedAmplifier](Images/simulated_Amplifier.png "Simulated amplifier")

The specific transistor Q1 could be substituted by any small signal NPN transistor.  Capacitor C1 blocks
DC feedback to the transitor base.  At 50Hz you can substitute a wide range of values and it will work
just fine.   

Of course an OpAmp-based amplifier might have been easier, but I didn't have a rail-to-rail opamp to play with, didn't want to use dual-rail power, and I know if I'd be able to clip one half of the signal.

The output shows the Vin voltage from the sensor, ((white, with 6 different amplitudes), and the (green) amplified output, with some clipping on one half of the sin wave.   

![simulatedOutputs](Images/simulated_outputs.png "Simulated outputs")

I thought the initial dip in the green V(vout) at 20ms was important: it assures us that we're still in the
active region of the transistor for small positive inputs, before we clip.  In other words, we're amplifiying a 
small portion of the positive half of the white wave before clipping, but all of the negative half of the white wave. 

## Keep It Simpler: Version 2!

Then a friend visited and said "Do you know what AREF and analogReference()
do on the Arduino?".  So suddenly I could adjust the ADC scale and sensitivity 
to match the signal, rather than have to amplify the signal to match the ADC. The 
internal 1.1V reference gave an ADC scale that was ideal to measure output
from a sensor that was producing 1V.  The 1.1V reference voltage 
is also exposed on the AREF pin on the Arduino.   

Thr amplifier I had built was history.  Here is the new Version 2 
circuit:  Two resistors voltage-divide the AREF in half, and that becomes the  
bias for one end of the current clamp.  A capacitor gives the 
AC current in the sensor a path to ground (other than allowing it to 
travel solely through my biasing resistors, which messes with the bias).

![circuit](Images/circuit.png "circuit")

## Build It!  

After a trial-run on a breadboard, I soldered up my own shield.  The female 
connector for the mini-jack plug on the sensor was scavanged from an old AT 
motherboard.  

![shield](Images/shield.jpg "shield")

There is a fair amount of electrical noise in my workspace.  I got noticable 
reductions in the noise (sampled by the software) by detaching my 
diagnostic tools, and by keeping the leads short. Hence the reason for soldering
what was needed onto the shield.  