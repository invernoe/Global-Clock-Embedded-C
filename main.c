#include <stdint.h>
#include <stdbool.h> 
#include <stdlib.h>
#include <string.h>
 
#include <FreeRTOS.h>
#include "FreeRTOSConfig.h"
#include "tm4c123gh6pm.h"
#include <task.h>
#include "queue.h"
#include "semphr.h"
#include "lcd.h"
#include "utils/uartstdio.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

#include "driverlib/pin_map.h"
#include "tm4c123gh6pm.h"

//Structure used to store and send time between tasks
//it contains 3 variables each one stores a different unit in time
typedef struct Message
{
unsigned char hours;
unsigned char minutes;
unsigned char seconds;
} AMessage;

void InitTask(void *);
void UART_TASK(void *);
void TASK1(void *);
void TASK2(void *);
void printStringUART(char* string);
void UARTHandler(void);
void reverse(char s[]);
void itoa(int n, char s[]);

AMessage Read_Time(void);

//Cities to be displayed in the UART
char cities[][10] = {"London", "Paris", "Madrid", "Rome", "Athens", "Ankara", "Istanbul", "Cairo", "Moscow", "Tehran"};
//Time differences from london based on hours and same indices as the cities variable
char timediff[] = {0, 1, 1, 1, 2, 2, 2, 2, 3, 4};
//integer to index current city
uint8_t currentCity = 0;

//Queue1 is responsible to send the incremented time that done by task1 and send it task2
xQueueHandle xQueue1;
//Queue2 is responsible to take the intial time that happens in task3 and sends it to task1
xQueueHandle xQueue2;

	int main()
	{
		
		__asm("CPSIE I");

		xQueue1= xQueueCreate(1,sizeof(AMessage));
		xQueue2= xQueueCreate(1,sizeof(AMessage));
		if(xQueue1!=NULL && xQueue2!=NULL)
		{
			xTaskCreate(InitTask,"Init Task", 200, NULL, 7, NULL);
			xTaskCreate(UART_TASK,"UART Task", 200, NULL, 1, NULL);
			xTaskCreate(TASK1,"UART Task", 200, NULL, 1, NULL);
			xTaskCreate(TASK2,"UART Task", 200, NULL, 1, NULL);
			
			vTaskStartScheduler();
		}
		
		while(1);
	}

/*this task is Periodic, it recieves the initialized time from task3 and then increment the time every one second
 then it sends the output to queue1 so task2 can recieve the exact time and display it on LCD */
void TASK1(void *){
	portBASE_TYPE xStatus1;
	portBASE_TYPE xStatus2;
	AMessage Tim;
	
	xStatus2 = xQueueReceive(xQueue2, &Tim, portMAX_DELAY);
				
	for(;;){
		vTaskDelay(1000 / portTICK_RATE_MS);
		Tim.seconds++;
		if(Tim.seconds == 60){
			Tim.seconds = 0;
			Tim.minutes += 1;
			if(Tim.minutes == 60){
				Tim.minutes = 0;
				Tim.hours += 1;
				if(Tim.hours == 24){
					Tim.hours = 0;
				}
			}
		}
		
		xStatus1 = xQueueSendToBack(xQueue1, &Tim, portMAX_DELAY);
	}
}
char oldCityName[10];

/*it receive the time from task1 as a value of hours, minuits, seconds respectively and it then turns these values 
into ASCII Codes so that we can display it in row2 on the LCD 
Also, it fetchs the current city name using the current city integer variable and displays it in row1 on the LCD*/
void TASK2(void *){
	portBASE_TYPE xStatus;
	AMessage Tim;
	for(;;){
		xStatus = xQueueReceive(xQueue1, &Tim, portMAX_DELAY);
		//correct time and city name based on current City variable
		Tim.hours = (Tim.hours + timediff[currentCity]) % 24;
		char* currentCityName = cities[currentCity];
		
	
		char currentHour[3];
		itoa(Tim.hours,currentHour);
		
		char currentMin[3];
		itoa(Tim.minutes,currentMin);
		
		char currentSec[3];
		itoa(Tim.seconds,currentSec);
		
		//Display city name at row 1 and city time at row 2
		if( strcmp( currentCityName,oldCityName)){
			strcpy( oldCityName,currentCityName);
		  LCD_Clear();
		  LCD_PrintLn(0,currentCityName);
		}

		 char temp[20]="";
		 strcat(temp,currentHour);
		 strcat(temp,":");
		 strcat(temp,currentMin);
		 strcat(temp,":");
		 strcat(temp,currentSec);
		
		LCD_PrintLn(1,temp);
	}
}

/* itoa:  convert n to characters in s */
 void itoa(int n, char s[])
 {
     int i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     
         if(i == 1)                /* if number is single digit */
             s[i++] = '0';    /* add 0 at the end of it */
         
     s[i] = '\0';
     reverse(s);
 }
 
 /* reverse:  reverse string s in place */
 void reverse(char s[])
 {
     int i, j;
     char c;
 
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }
/* At initialization UART_TASK calls the read_Time() function so that the user initializes the london time with a given value
   Then sends this value to task1 using Queue2.
   This is a continuous function, the infinite loop prints to the user to select a city to display its time, the current city
   is set to London by default, then awaits the user to input a number between 0 and 9 to select a city. */ 
 
void UART_TASK(void *) {
		printStringUART("\r\n**********************************************\r\n" );
		printStringUART("\t Time in different cities:\r\n" );
		printStringUART("**********************************************\r\n" );
		AMessage Tim = Read_Time();
		xQueueSendToBack(xQueue2, &Tim, portMAX_DELAY);
		for(;;)
		{
				printStringUART("Select a city to see its current time:\r\n" );
				for(int i = 0; i < 10; i++){
					UARTCharPut(UART0_BASE, (i + 48));
					printStringUART("-");
					printStringUART(cities[i]);
					printStringUART(" \r\n");
			
				}
				printStringUART("\r\n");
				
				char cityNum = UARTCharGet(UART0_BASE);
				if(cityNum >= '0' && cityNum <= '9'){
					UARTCharPut(UART0_BASE, cityNum);
					currentCity = cityNum - 48;
					printStringUART("-");
					printStringUART(cities[currentCity]);
					printStringUART(" \r\n");
					
				} else{
					printStringUART("invalid city number");
				}
				
				printStringUART("\r\n");
				
				taskYIELD();
		}
}

/*Output the given character array character by character into the UART transmit pin */
void printStringUART(char* string) {
	 while(*string)
		{
		 UARTCharPut(UART0_BASE, *(string++));
		}
}

void UART_Init(void) {
	//Activate UART0 and Port A Clock
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0|GPIO_PIN_1);

	//disable UART For configuration
	UARTDisable(UART0_BASE);
	
	// Configuration 9600 baud rate and 8 bit
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	UARTFIFODisable(UART0_BASE);
	
	// enable UART after configuration
	UARTEnable(UART0_BASE);
}
/*This function prompts the user to enter the current time in London in hh:mm:ss format
    after the user presses enter the function then transforms the input from ASCII code into
    our AMessage struct and returns it as output*/
AMessage Read_Time() {
	AMessage Tim;
	unsigned char time[3] = {0, 0, 0};
	printStringUART("enter london current time in hh:mm:ss format : \r\n");
		
		int i = 0;
		char currentCharacter; 
		while(1){
			
			currentCharacter = UARTCharGet(UART0_BASE);
			UARTCharPut(UART0_BASE,currentCharacter);
			if(currentCharacter == 0x0D){
				printStringUART("\r\n");
				break;
			}
			
			if(currentCharacter == ':'){
				i++;
			} 
			else
			{
			time[i] = (time[i] * 10) + (currentCharacter-'0');
			}
		}
		
		Tim.hours = time[0];
		Tim.minutes = time[1];
		Tim.seconds = time[2];
		
		return Tim;
}

void InitTask(void *) {
		UART_Init();
	  LCD_init();                                
		vTaskSuspend(NULL);
}