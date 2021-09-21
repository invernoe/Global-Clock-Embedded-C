# Global-Clock-Embedded-C
A C Project running on a TM4C123GH6PM microcontroller that displays the current clock for different cities using the FreeRTOS APIs

1. General Description  
The main goal of the project is to display the time in different cities on a 16x2 LCD using TivaC and the FreeRTOS framework. The code starts with the user entering the current time of London by using the keyboard serial input to the TivaC UART. Then, the code by default should display the clock in London and increments the time every second. After that, the program output to the user the cities that he can choose from to display their current time. The TivaC achieves other cities’ time by adding hour differences to the original London current time, and then displays the correct output by incrementing every second.

2. Functions Description  
Queue1 is responsible to send the incremented time that done by task1 and sends it task2.  
Queue2 is responsible to take the initial time that happens in task3 and sends it to task1.  
void TASK1(void *)  
- this task is Periodic; it receives the initialized time from task3 and then increment the
time every one second then it sends the output to queue1 so task2 can receive the
exact time and display it on LCD.  
void TASK2(void *)  
- it receives the time from task1 as a value of hours, minutes, seconds respectively and
it then turns these values into ASCII Codes so that we can display it in row2 on the
LCD. Also, it fetches the current city name using the current city integer variable and
displays it in row1 on the LCD.  
void itoa(int n, char s[])  
- this function converts n to characters in s.
void reverse(char s[])
- this function reverses the string s in place.
void UART_TASK(void *)  
- At initialization UART_TASK calls the read_Time() function so that the user initializes
the London time with a given value. Then sends this value to task1 using Queue2. This
is a continuous function, the infinite loop prints to the user to select a city to display
its time, the current city is set to London by default, then awaits the user to input a
number between 0 and 9 to select a city.
void printStringUART(char* string)  
- Output the given character array character by character into the UART transmit pin.
void UART_Init(void)
- this function is for initialization of the UART and activating the needed ports.
AMessage Read_Time()  
- This function prompts the user to enter the current time in London in hh:mm:ss
format after the user presses enter the function then transforms the input from ASCII
code into our AMessage struct and returns it as output.
