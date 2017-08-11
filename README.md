# FT2232H_D2XX_linux
 minimalist exampe of SPI on an FT2232H chip. Runs on Ubuntu Linux. Based on AN_114 pdf 

    //YOU HAVE TO RUN THIS FIRST TO DISABLE VCP !
    sudo rmmod ftdi_sio; sudo rmmod usbserial;
    
    
I had a hell of a time trying to find any examples of a working driver for an FT2232H chip.

The example in Application Note 114, was designed for D2XX on windows, so I set out re writing it line by line for linux.

I finally got it working. This is a copy of my file before it got much more convoluted. It might not be perfect, but its a lot closer than the PDF is top get you up and running. 


