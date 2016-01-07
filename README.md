# Elevator Simulation
This is a simulation of an elevator system using principles of concurrent systems such as data pools, pipelines and semaphores in C++. This simulation allows for multiple elevators to be created at runtime.

The rt.cpp and rt.hpp library was provided by the professor. It contains the code for the semaphore, pipeline and datapool implementations.

In terms of architecture, the dispatcher, IO and the elevators are implemented as 4 ACTIVE CLASSES within a single process/project.

# How to Use
To request an elevator while standing outside on a given floor, one must enter command such as 'u0', 'u5' and 'd1', 'd6'. Where the letters 'u' and 'd' refer to a request by the passenger to go up or down respectively. The number tells the simulation which floor the request is being made from, not which floor the person wishes to be subsequently transported to. 

Once inside the elevator one must enter two numbers. The first chracter can be a '1', or '2', or '3', etc, to designate a command directed to elevator 1 or 2 or 3 respectively. The second character is in the range '0-9' to indicate which floor the elevator needs to go to. For example, a passenger stepping into elevator 2 and wishing to travel to floor 3 would be simulated by entering the characters '2' then '4'.

To simluate elevator faults one can press '+1' or '-1', '+2' or '-2', etc. The minus sign '-' means there is a fault at the elevator. The plus sign '+' clears the fault.

To stop the simulation one must press the sequence 'ee'.
