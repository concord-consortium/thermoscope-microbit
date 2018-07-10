# thermoscope-microbit
a microbit implementation of the thermoscope hardware

##  build development environment docker image.
You only need to do this the first time you
checkout the project

    docker-compose build

## build code.
Do this every time you change the code. Like most native build systems it
caches the build results so it will only build the changed files.

    docker-compose run --rm dev yt build

## copy result to MicroBit.
This command is for OS X.  On other platforms you'll need
to find the appropriate place to copy the file to.

    cp build/bbc-microbit-classic-gcc/source/thermoscope-microbit-combined.hex /Volumes/MICROBIT/

## Testing and configuring

I have not found a good BLE tool for OS X. I was using LightBlue which is available on
the app store. But it hasn't been updated since 2013. It used to be the fastest an easiest even though it has some bugs. But as of 2018-07-10 it no longer is able to browse the BLE info from the microbit.

There is an open source clone called Bluetility, but it is missing a disconnect button and hasn't been worked on since 2016

There is also nrf connect from Nordic semiconductor which makes some of the most popular BLE chips. But I could not get that to work on OS X. It seems it needs an external BLE dongle, but perhaps I'm just not doing things right.
