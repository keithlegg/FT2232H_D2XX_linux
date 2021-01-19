TOPDIR  := $(shell cd ..; pwd)
include $(TOPDIR)/Rules.make


#the main shebang
APP = keithspi

#code for initializing the device ONCE 
#APP = init_main 


all: $(APP)

$(APP): main.c	
	$(CC) main.c -o $(APP) $(CFLAGS)	
	
clean:
	rm -f *.o ; rm $(APP)

reset:
	sudo rmmod ftdi_sio;sudo rmmod usbserial 
	