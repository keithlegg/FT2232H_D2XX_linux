# FT2232H_D2XX_linux
minimalist example of full duplex SPI on an FT2232H chip, at USB2.0 speeds. 
 
Update 1/19/21 
    - tested on OSX 10.14.6 using D2XX 1.14.16 , and Ubuntu Linux. 

Based on AN_114.pdf from FTDI

See:
    http://www.ftdichip.com/Support/Documents/AppNotes/AN_114_FTDI_Hi_Speed_USB_To_SPI_Example.pdf


To use download the drivers and follow the instructions at : 

https://ftdichip.com/drivers/d2xx-drivers/

add this in to the example projects and compile it!


Background:

I had a hell of a time trying to find any examples of a working driver for an FT2232H chip. The example in Application Note 114, was designed for D2XX on windows, so I set out re writing it line by line for linux. This is a copy of my file before it got much more complicated. It might not be perfect, but its a working example, simple , and a lot closer than the PDF is to getting you up and running.

I was told D2XX doesnt work on linux and to use LIBFTDI instead, but I actually got it working and its FAST!. 
D2XX is FTDI's native- closed source library with windows compatibility , so be aware it may not be the first option for your design.

The defualt driver is the Virtual Com Port driver , it starts automatically when the device is plugged in. To develop d2xx drivers you have to temporarily remove VCP (until device is reset or power cycled): 

    sudo rmmod ftdi_sio; sudo rmmod usbserial;

    
    




 


