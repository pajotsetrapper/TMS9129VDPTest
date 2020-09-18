# TMS9129VDPTest

Arduino-based TMS9129 VideoProcessor test.
Based on the work done by Nejat Dilek: https://www.youtube.com/watch?v=1CjWzx8yxoA / https://pastiebin.com/5eccfd51d28cc.

This sketch (which does need cleanup ;-)) tests the backdrop functionality & has the proper configuration in place to ensure an interrupt is generated after every screen.
It does not have the functionality report back on wether an interrupt was effectively triggered.
For the latter, I would suggest to connect the VDP's interrupt to a free interrupt-capable Arduino input & attach an interrupt service routine to it (in which to read the status register & do whatever you like in there).

Did not take the time to add that functionality, connecting a probe/oscilloscope was faster.
Feel free to contribute :-).

For more info: https://www.msx.org/forum/msx-talk/graphics-and-music/functional-vdp-test-tms9129nl-with-arduino
Connection diagram & the VDP's programming guide have been added in this project
