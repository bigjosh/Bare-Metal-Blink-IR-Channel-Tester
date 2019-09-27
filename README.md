# Bare Metal Blink IR Channel Tester

This program lets you measure the response of the physical IR communications channel between two blinks. You can use it to figure out how much signal-to-noise you have to work with when designing blinks IR communication protocols.

## WARNING

This code will suppress sleep and IR code download. 

If you leave this running it will eventually kill the battery and you will not be able to download a new program via IR.  

Be sure to remember to load a normal blinks program via the hardware programming port when you are done testing!

## Bare Metal

This is very unlike almost all other blinks programs you've ever seen in that it talks directly to the hardware. To make this possible, it includes the `hardware.h` file from BlinkOS and also turns off interrupts with `cli()` which effectively give us exclusive access to the hardware as long as we do not return from `setup()`. In exchange, we give up being able to use any blinklib or blinkOS functions or services - we have to do everything ourselves. 

Note that there is almost never a good reason to do this in real life!

## Measuring channel quality

This tester program lets us (almost) directly measure the time between when the TX LED turns on and when the RX LED triggers using an oscilloscope.

The program has TX mode and RX mode.  

A RED pixel indicates a face in RX mode, and a GREEN pixel indicates a face in TX mode. Pressing and lifting the button advances to the next mode/face. 

Both modes generate their output on the `T` pin on the debug header of the circuit board. (Note that it seems like the `T` and `R` pins are swapped and the `A` pin does not work at all on newer blinks!)

To make measurements, you will want to connect an oscilloscope scope to the `G` and `T` pins of one TX mode blink and one RX mode blink, and then point their respective test sides. Here is one way that can look...

![](connections.jpg)

In this case, face 5 on the top blink is transmitting to face 0 on the bottom blink.

With the `T` pin outputs from the RX and TX blink each connected to a channel on your scope, scale everything up so you can easily see a 2-3 volt signal. 

#### Maximum theoretical bandwidth

Set your time scale to about 1us per div. Turn on both the TX and RX signal channels. Set a trigger on the rising edge of the signal from the TX blink.

With the blinks set up so that they can see each others' faces, you should see something like this...

![](TimeToTriggerByTX.png)

Here the YELLOW trace is TX and BLUE trace is RX.

You are interested in the time between when the TX goes high (TX LED turns on) and the RX goes low (the RX LED triggers). The shorter this time is, the higher the theoretical bandwidth of the communications path. In this case, the time is about 6us. 

You can move the blinks around to see how alignment impacts this time. Cleaner path should lead to shorter times (better). The longest time is what sets the worst case.

You can also try different battery voltage levels. Lower voltage on the TX blink should mean dimmer LED and so longer times to trigger the RX blinks (worse). The impact of battery voltage on the RX side is less clear since the `1` to `0` transition voltage is dependent on the battery voltage. LMK if you discover any correlation between RX voltage and trigger time!   

You can also set the scope display to "infinite" if you want to accumulate a bunch of reading to find the boundaries.

#### Ambient Noise

Set your time scale to about 1ms per div. We only care about the signal from the RX channel, so you can turn off the trace from the TX blink. Set a trigger on the rising edge of the signal from the RX blink.

Now separate the blinks so that the RX is looking into the wilderness. 

It should look something like this...

 ![](TimerToTriggerAmbient.png)

Here the BLUE trace is the RX.

You are interested in the time that the RX signal is high between triggers. The longer this time is, the more time it will take for an RX LED to trigger because of noise rather than real signal. In this case, the time is about 1.5ms.

You can move the blink around and point the face you are testing a a bright wall or a cover it with your hand and see how that impacts the time. Brighter should lead to shorter times (worse). The shortest time is what sets the worst case. The impact of battery voltage on the ambient noise triggers is less clear since the `1` to `0` transition voltage is dependent on the battery voltage. LMK if you discover any correlation between RX voltage and ambient trigger time!   
 
You can also set the scope display to "infinite" if you want to accumulate a bunch of reading to find the boundaries.


## Concept

To stay cheap, instead of a typical LED for transmit and photo-transistor to receive, blinks use IR LEDs for both transmitting and receiving. Transmitting on an LED is easy, just turn it on. Receiving on an LED is slightly trickier.  We treat the diode inside the LED as a capacitor by reverse charging it. When photons hit the junction, they carry charge across the barrier and discharge it. When enough photons have hit, the voltage across the LED will drop below a digital 1 level and the micro-controller can sense this.

## Channel quality

Our bandwidth is fundamentally limited by the amount if time it takes to accumulate enough photons on the RX LED to change it from a `1` voltage to a `0` voltage. The lower this time, the faster we can theoretically send data.  The brighter the TX LED (more photons created) and the clearer the path (more photons that make it from the TX to the RX), then the shorter time it will take to accumulate these photons. 

There are also a bounds on the maximum time we can wait to accumulate enough photons. Physically, there will always be a leakage current across a non-ideal diode that will bleed the charge away. In practice, in a dark room this time is on the order of seconds, so it is not really a factor in practice.

But since we want blinks to work in places that are not pitch dark, we do have to worry about photons from the ambient light reaching the RX LED. This is a real factor since even indirect sunlight can be brighter than our TX LED.

As long as the time it takes for the ambient light to trigger the RX LED is *longer* than the time it takes for the TX LED to trigger the RX LED, then we have a shot at being able to reasonably send data across the channel. 

## Operation

### TX side

The blink continuously sends pulses on the indicated side. Each pulse is ~10us wide (#defined in code) and they have ~100us between them (computed 3*width). 

The blink turns debug pin `T` on when the IR LED is on. (Rising edge of `T` means IR LED just turned on)

### RX side

The blink charges up the indicated IR LED and waits for it to discharge.

The blink turns on debug pin `T` while it is waiting. (Falling edge of `T` means IR LED just triggered)

It also waits for about 120% of the LED on time to prevent triggering twice from the same incoming pulse. 
  
## Conclusion

That's all, you just measured the only two parameters that characterize the communication channel between any two blink faces! The longest TX trigger time and the shortest ambient trigger time across a population of blinks and across a range of lighting conditions and battery voltage levels will limit the maximum theoretical communications rate between blinks.

Practically speaking, the real world maximum bandwidth will also be limited by the horsepower of the CPU and the protocol constraints.      
