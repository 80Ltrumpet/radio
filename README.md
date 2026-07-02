# Wireless Puzzle

This (previously private) repository contains the firmware for a wireless-sensor-network-based puzzle I contributed to a Halloween-themed scavenger hunt for a local volleyball club.

The puzzle was set in a small room full of Halloween decorations where the main clue was a journal. The journal was open to an entry where words naming some of the objects in the room were presented in a bold font, and the entry was written such that those words appeared in a specific sequence. The goal of the puzzle was to pick up each of those objects in order. Upon picking up the last object, a lamp behind a semi-opaque screen would flicker on to reveal the next clue in the scavenger hunt.

Embedded in each object was an [Adafruit Feather](https://www.adafruit.com/product/3077) with an integrated RFM69HCW radio module and an added LSM6DSOX IMU via an [expansion board](https://www.adafruit.com/product/4565). One or more RGB LEDs was integrated into the object to provide a form of feedback to the players. All of this was powered by a rechargable battery, so I wired up a toggle switch to connect and disconnect power. I also configured the microcontroller to disable unused functions and operate in the lowest power modes whenever possible.

Main [puzzle logic](./src/root/puzzle.cc) was controlled by an old Arduino MEGA 2560 with a breadboarded RFM69HCW and a relay for the lamp. For in-situ debugging or intervention, I kept this connected to a laptop to access the [console](./src/console.cc) providing a set of [commands](./src/command_registry.cc) I defined for the purpose.

As long as the players picked up objects in the correct order, the LEDs of the held objects would glow purple with a gentle throbbing pattern. If an object was picked up in the wrong order, all of the held objects' LEDs would rapidly blink red in a sine pattern until they were put down. Once the puzzle was solved, the LEDs would throb green.

This puzzle turned out to be more difficult than anticipated, since most players did not naturally know what to do when the LEDs flashed red. Players would also often inadvertently confuse each other with inaccurate observations or assumptions, which also, somewhat surprisingly, discouraged further or repeated experimentation. As the puzzle operator, I provided hints (in costume!) and surreptitiously modified the puzzle state when groups would get stuck.
