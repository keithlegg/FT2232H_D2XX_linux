#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>   // needed to sleep.
#include <math.h>

#include "../ftd2xx.h"


#define false 0 


#define MemSize 32 //define data quantity you want to send out

//declare for BAD command 
const BYTE AA_ECHO_CMD_1         = '\xAA';
const BYTE AB_ECHO_CMD_2         = '\xAB';
const BYTE BAD_COMMAND_RESPONSE  = '\xFA'; 

//declare for MPSSE command
const BYTE MSB_RISING_EDGE_CLOCK_BYTE_OUT   = '\x10'; 
const BYTE MSB_FALLING_EDGE_CLOCK_BYTE_OUT  = '\x11';
const BYTE MSB_RISING_EDGE_CLOCK_BIT_OUT    = '\x12';
const BYTE MSB_FALLING_EDGE_CLOCK_BIT_OUT   = '\x13';
const BYTE MSB_RISING_EDGE_CLOCK_BYTE_IN    = '\x20';
const BYTE MSB_RISING_EDGE_CLOCK_BIT_IN     = '\x22';
const BYTE MSB_FALLING_EDGE_CLOCK_BYTE_IN   = '\x24';
const BYTE MSB_FALLING_EDGE_CLOCK_BIT_IN    = '\x26';


/******************************************/

FT_STATUS ftStatus;

DWORD dwClockDivisor   = 29; //Value of clock divisor, SCL Frequency = 60/((1+29)*2) (MHz) = 1Mhz

BYTE OutputBuffer[512];      //Buffer to hold MPSSE commands AND DATA to be SENT to FT2232H
BYTE InputBuffer[512];       //Buffer to hold Data bytes to be READ from FT2232H

DWORD dwNumBytesToSend = 0;  //Index of output buffer
DWORD dwNumBytesSent   = 0, dwNumBytesRead = 0, dwNumInputBuffer = 0;

WORD DataOutBuffer[MemSize];
WORD DataInBuffer[MemSize];

BYTE ByteDataRead;
WORD MemAddress = 0x00;
WORD i=0;



/******************************************/

void show_buffers(){

    int xx = 0;

	if(dwNumBytesRead){
	    printf("#$ --- Number of bytes in input buffer %d \n", dwNumBytesRead);
        
	    if(dwNumBytesRead){	
	        for(xx=0;xx<dwNumBytesRead;xx++){    
	            printf("#$     --- input buffer byte:%d %d\n", xx, InputBuffer[xx]);	    
	        }
        }
    }

	if(dwNumBytesSent){    
		printf("#$ --- Number of bytes sent  %d \n", dwNumBytesSent);
	    if(dwNumBytesSent){	    
	        for(xx=0;xx<dwNumBytesSent;xx++){    
	            printf("#$ ---     output buffer byte:%d %d\n", xx, OutputBuffer[xx]);	    
	        }       
	    }		
    }

}


/******************************************/
//this routine is used to enable SPI device
void SPI_CSEnable(){
	int loop = 0;
	for( loop=0;loop<5;loop++) //one 0x80 command can keep 0.2us, do 5 times to stay in this situation for 1us
	{
		OutputBuffer[dwNumBytesToSend++] = '\x80'; //GPIO command for ADBUS
		OutputBuffer[dwNumBytesToSend++] = '\x00'; //set CS, MOSI and SCL low ***
		OutputBuffer[dwNumBytesToSend++] = '\x0b'; //bit3:CS, bit2:MISO,bit1:MOSI, bit0:SCK
	}
}

/******************************************/
//this routine is used to disable SPI device
void SPI_CSDisable()
{
	int loop=0;
	for( loop=0;loop<5;loop++) //one 0x80 command can keep 0.2us, do 5 times to stay in this situation for 1us
	{
		OutputBuffer[dwNumBytesToSend++] = '\x80'; //GPIO command for ADBUS 
		OutputBuffer[dwNumBytesToSend++] = '\x08'; //set CS high, MOSI and SCL low	*** 	
		OutputBuffer[dwNumBytesToSend++] = '\x0b'; //bit3:CS, bit2:MISO,bit1:MOSI, bit0:SCK
	}
}

/******************************************/
 
//this routine is used to initial SPI interface
BOOL SPI_Initial(FT_HANDLE ftHandle)
{
	DWORD dwCount;
	dwNumBytesToSend = 0;
	dwNumBytesRead   = 0;

	ftStatus = FT_ResetDevice(ftHandle); //Reset USB device
	
	//Purge USB receive buffer first by reading out all old data from FT2232H receive buffer
	ftStatus |= FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);
	
	// Get the number of bytes in the FT2232H receive buffer
	if ((ftStatus == FT_OK) && (dwNumInputBuffer > 0))
	{
		ftStatus |= FT_Read(ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);
	}
	
	//Read out the data from FT2232H receive buffer...
	
	//ARRRG. THE DOCS SAY EXPLICITLY SET DEFAULTS EVEN IF YOU DONT NEED TO 
	//BUT THIS BLOCK OF CODE SEEMS TO CAUSE THE "EVERY OTHER - 0XAA init BUG" 
	
	// ftStatus |= FT_SetUSBParameters(ftHandle, 65535, 65535); //Set USB request transfer size
	// ftStatus |= FT_SetChars(ftHandle, false, 0, false, 0);   //Disable event and error characters
	// ftStatus |= FT_SetTimeouts(ftHandle, 3000, 3000); 	     //Sets the read and write timeouts in 3 sec for the FT2232H
	// ftStatus |= FT_SetLatencyTimer(ftHandle, 1);       	     //Set the latency timer (default is 16mS)
	// ftStatus |= FT_SetBitMode(ftHandle, 0x0, 0x00);   	     //Reset controller
	// ftStatus |= FT_SetBitMode(ftHandle, 0x0, 0x02);          //Enable MPSSE mode
	
	//ARRRG. THE DOCS SAY EXPLICITLY SET DEFAULTS EVEN IF YOU DONT NEED TO 
	//BUT THIS BLOCK OF CODE SEEMS TO CAUSE THE "EVERY OTHER - 0XAA init BUG"

	if (ftStatus != FT_OK)
	{
		printf("fail on initialize FT2232H device !\n");
		return false;
	}

    usleep(50000); // Wait 50ms for all the USB stuff to complete and work
	
	//////////////////////////////////////////////////////////////////
	// Synchronize the MPSSE by sending a bogus opcode (0xAB), 
	// The MPSSE will respond with "Bad Command" (0xFA) followed by
	// the bogus opcode itself.
	//////////////////////////////////////////////////////////////////

	dwNumBytesToSend = 0;
	dwNumBytesRead   = 0;


	OutputBuffer[dwNumBytesToSend++] ='\xAA'; //Add BAD command 0xAA
	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send off the BAD commands
	dwNumBytesToSend = 0;	//Clear output buffer
	do
	{
		ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer); // Get the number of bytes in the device input buffer
	} while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));      //or Timeout
	
	bool bCommandEchod = false;



	ftStatus = FT_Read(ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead); 	//Read out the data from input buffer
	for (dwCount = 0; dwCount < (dwNumBytesRead-1); dwCount++)                      //Checkif Bad command and echo command received
	{
		if ((InputBuffer[dwCount]==(BYTE)('\xFA')) && (InputBuffer[dwCount+1]==(BYTE)('\xAA')) )
		{
		    bCommandEchod = true;
		    break;
		}
	}

	if(bCommandEchod == false)
	{
	    printf("\n-------------------------\nfailed to synchronize MPSSE with command '0xAA'\n");
		printf("\n-------------------------\n");
        show_buffers();

  	    return false;
	    // Error, cant receive echo command , fail to synchronize MPSSE interface;
	}

	//////////////////////////////////////////////////////////////////
	// Synchronize the MPSSE interface by sending bad command 0xAB
	//////////////////////////////////////////////////////////////////

	dwNumBytesToSend = 0;

	//Clear output buffer
	OutputBuffer[dwNumBytesToSend++] ='\xAB'; //Send BAD command 0xAB
	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend,&dwNumBytesSent); // Send off the BAD commands
	dwNumBytesToSend = 0; //Clear output buffer
	do
	{
		ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);
		//Get the number of bytes in the device input buffer
	}while((dwNumInputBuffer == 0) && (ftStatus == FT_OK)); //or Timeout
	bCommandEchod = false;


	ftStatus = FT_Read(ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead); //Read out the data from input buffer
	for (dwCount = 0; dwCount < (dwNumBytesRead-1); dwCount++)//Check if Bad command and echo command received
	{
		if((InputBuffer[dwCount]==(BYTE)('\xFA')) && (InputBuffer[dwCount+1]==(BYTE)('\xAB')))
		{
			bCommandEchod = true;
			break;
		}
    }

	if(bCommandEchod==false)
	{
		printf("\n-------------------------\nfailed to synchronize MPSSE with command '0xAB'\n");
		printf("\n-------------------------\n");		
        show_buffers();

		return false;
		// Error, cant receive echo command , fail to synchronize MPSSE interface; 
	}

	////////////////////////////////////////////////////////////////////
	//Configure the MPSSE for SPI communication with EEPROM
	//Although the default settings may be appropriate for a particular application, it is always a good 
    //practice to explicitly send all of the op-codes to enable or disable each of these features
	//////////////////////////////////////////////////////////////////

	OutputBuffer[dwNumBytesToSend++] ='\x8A'; //Ensure disable clock divide by5 for 60Mhz master clock
	OutputBuffer[dwNumBytesToSend++] ='\x97'; //Ensure turn off adaptive clocking
    OutputBuffer[dwNumBytesToSend++] ='\x8D'; //disable 3 phase data clock
	
	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend,&dwNumBytesSent);// Send out the commands
	
	dwNumBytesToSend = 0;//Clear output buffer
	OutputBuffer[dwNumBytesToSend++] ='\x80'; //Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] ='\x00'; //Set SDA, SCL high, WP disabledby SK, DO at bit＆＊, GPIOL0 at bit＆＊
	OutputBuffer[dwNumBytesToSend++] ='\x0b'; //Set SK,DO,GPIOL0 pins as output with bit＊＊, other pins as input with bit＆＊
	
	//The FT2232D is based around a 12MHz clock.  A 16-bit divisor is used to program the data transmission 
	// The SK clock frequency can be worked out by below algorithm with divide by 5 set as off
	// SK frequency = 60MHz /((1 + [(1 +0xValueH*256) OR 0xValueL])*2)
	
	OutputBuffer[dwNumBytesToSend++] ='\x86';	                      //Command to set clock divisor
	
	OutputBuffer[dwNumBytesToSend++] = (BYTE)(dwClockDivisor &'\xFF');  //Set 0xValueL of clock divisor
	OutputBuffer[dwNumBytesToSend++] = (BYTE)(dwClockDivisor >> 8);     //Set 0xValueH 

	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend,&dwNumBytesSent);// Send out the commands
	dwNumBytesToSend = 0; //Clear output buffer
	usleep(20000); //20 ms

	//Delay for a while
	//Turn off loop back in case
	OutputBuffer[dwNumBytesToSend++] ='\x85'; //Command to turn off loopback of TDI/TDO connection
	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend,&dwNumBytesSent);// Send out the commands
	dwNumBytesToSend = 0; //Clear output buffer
	usleep(30000); //(30 ms);

	//Delay for a while
	printf("SPI initial successful\n");
	return true;

}

/******************************************/
/******************************************/

//keith made this from WriteEECmd to send a single byte. Works but may not be right.
FT_STATUS single_byte_write(FT_HANDLE ftHandle, BYTE command, WORD *bdata)
{

	dwNumBytesSent=0;

	SPI_CSEnable();

	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BIT_OUT;
	OutputBuffer[dwNumBytesToSend++] = 7; //7+1 = 8
	OutputBuffer[dwNumBytesToSend++] = command;

	SPI_CSDisable();

	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend,&dwNumBytesSent); //send MPSSE command to MPSSE engine.
	dwNumBytesToSend = 0; //Clear output buffer

	/*********************************/ 	//debug - test of read 	//debug - test of read 	
    //dwNumBytesRead = 0; //Clear input buffer ?? 
    //THIS SEEMS TO TIME OUT BUT SOME DATA IS COMING BACK??
    //ftStatus = FT_Read(ftHandle, InputBuffer, 2, &dwNumBytesRead);  
	//*bdata = (InputBuffer[0] << 8) + InputBuffer[1];
    //*bdata = InputBuffer[0];
	/*********************************/ 	//debug - test of read 	//debug - test of read 
	return ftStatus;

}

/******************************************/

//this routine is used to read one word data from a random address
BOOL SPI_KeithFullDuplexTest(FT_HANDLE ftHandle, BYTE spi_send, BYTE* spi_recieve) // WORD* spi_recieve)
{

	//////////////////////

	dwNumBytesToSend = 0; //Clear output buffer
    dwNumBytesRead   = 0; //Clear input buffer

	SPI_CSEnable();     
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_OUT; // '\x11';
	OutputBuffer[dwNumBytesToSend++] = '\x07'; //write Size Low  ( 15+1 bytes )
	OutputBuffer[dwNumBytesToSend++] = '\x00'; //write Size High

	OutputBuffer[dwNumBytesToSend++] = '\x01'; //byte1 sent
	OutputBuffer[dwNumBytesToSend++] = '\x02'; //byte2 sent 
	OutputBuffer[dwNumBytesToSend++] = '\x04'; //byte3 sent 
	OutputBuffer[dwNumBytesToSend++] = '\x08'; //byte4 sent
	OutputBuffer[dwNumBytesToSend++] = '\x10'; //byte5 sent 
	OutputBuffer[dwNumBytesToSend++] = '\x20'; //byte6 sent 
	OutputBuffer[dwNumBytesToSend++] = '\xf0'; //byte7 sent 
	OutputBuffer[dwNumBytesToSend++] = '\x0f'; //byte8 sent 
    
	OutputBuffer[dwNumBytesToSend++] = MSB_RISING_EDGE_CLOCK_BYTE_IN; // '\x20';
	OutputBuffer[dwNumBytesToSend++] = '\x07'; //read Size Low
	OutputBuffer[dwNumBytesToSend++] = '\x00'; //read Size High
	SPI_CSDisable();

	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend,&dwNumBytesSent); //send out MPSSE command to MPSSE engine
	ftStatus = FT_Read(ftHandle, InputBuffer, 8, &dwNumBytesRead); //Read XXX bytes from device receive buffer

    ////////////////////// 
	for(i=0;i<dwNumBytesRead;i++){
	    printf("Read data - idx: %d = %d\n", i, InputBuffer[i]);
    }
    //////////////////////

    dwNumBytesRead   = 0; //Clear Input buffer 
    dwNumBytesToSend = 0; //Clear output buffer

	return ftStatus;
}


/******************************************/

int main(int argc, char **argv) 
{
	


	FT_HANDLE ftdiHandle;
	
	DWORD numDevs;

	FT_DEVICE_LIST_INFO_NODE *devInfo;
	ftStatus = FT_CreateDeviceInfoList(&numDevs);// p* to unsigned long - number of connected devices.

    /*
	if(ftStatus == FT_OK)
	{
	  printf("Number of devices is %d\n", numDevs); //show the number 
    }else return 1;
    */

	if(numDevs > 0) 
	{
		// allocate storage for list based on numDevs
		devInfo =(FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);

		// get the device information list
		ftStatus = FT_GetDeviceInfoList(devInfo,&numDevs);
		if(ftStatus == FT_OK) 
		{

			for(i = 0; i < numDevs; i++) 
			{
				printf("Dev %d:\n",i);
				printf(" Flags=0x%x\n"      ,devInfo[i].Flags);
				printf(" Type=0x%x\n"       ,devInfo[i].Type);
				printf(" ID=0x%x\n"         ,devInfo[i].ID);
				printf(" LocId=0x%x\n"      ,devInfo[i].LocId);
				printf(" SerialNumber=%s\n" ,devInfo[i].SerialNumber);
				printf(" Description=%s\n"  ,devInfo[i].Description);
				printf(" ftHandle=0x%x\n"   ,devInfo[i].ftHandle);
			}
		}
	}else return 1;


	ftStatus = FT_Open(0,&ftdiHandle);
	if (ftStatus != FT_OK)
	{
		printf("Can't open FT2232H device!\n");
		return 1;

	} else
	
	// Port opened successfully
	printf("Successfully opened FT2232H device!\n");


	if (SPI_Initial(ftdiHandle) == TRUE)	
	{

		char ReadByte = 0;

		//initial output buffer
		for	(i=0;i<MemSize;i++)	
		{	
			DataOutBuffer[i] = i;
	    }
	
		//Purge USB received buffer first before read operation
		ftStatus = FT_GetQueueStatus(ftdiHandle, &dwNumInputBuffer);
		
		// Get the number of bytes in the device receive buffer
		if((ftStatus == FT_OK) && (dwNumInputBuffer > 0))
		{
		    FT_Read(ftdiHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);
	    }

        
        // ----------------------------------------
 		//              Parameters
		// ftHandle          - Handle of the device.
		// lpBuffer          - Pointer to the buffer that receives the data from the device.
		// dwBytesToRead     - Number of bytes to be read from the device.
		// lpdwBytesReturned - Pointer to a variable of type DWORD which receives the number of 
		//                     bytes read from the device.
        // ----------------------------------------

		SPI_KeithFullDuplexTest(ftdiHandle, i, &DataInBuffer[i]);

        //FOR DEBUGGING DATA IN BUFFERS
	    //printf("\n\n#### POST\n");
        //show_buffers();

        //////////////////////////////////////////
	}
 
	FT_Close(ftdiHandle);


	return 0;
}