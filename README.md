# CS-350-Emerging-Sys-Arch-and-Tech



## -    Summarize the project and what problem it was solving.

The objective of this project is to use, timers, interrupts, state machines, task schedulers, and drivers to create a thermostat.  This thermostat is to run on a TI cc3220s circuit board
and use the red led to turn on and off to simulate the heat turning on and off. The program reads the ambient temp, then compares it the user set temp to set the state of the state machine.  The state machine is used to control heat being on and off as well as the led being on or off. 


## -    What did yo u do particularly well?

I feel that the overall I did well creating the state machines to run each state to run the thermostat. 


## -    Where could you improve?

The major improvement with this project would be to try and have more of the code fall within the state machines instead of before the state machine.  


## -    What tools and/or resources are you adding to your support network?

Learning another coding language adds more support to my network


## -    What skills from this project will be particularly transferable to other projects and/or course work?


Every skill learned within this project will help me in my journey.  Learning how to make a state machine, interact with the UART and getting a better understanding on how to interact with 
embedded devices weill all be very transferrable to future projects. 


## -    How did you make this project maintainable, readable, and adaptable?

To try and make the project maintainable,  the two state machinces are within callable functions where most of the program actions are handled within these state machines. 