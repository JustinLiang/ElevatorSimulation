#ifndef __ELEVATOR__
#define __ELEVATOR__

#include "rt.h"
#include "data.h"
#include <queue>
#include <vector>
#include <functional>

/**
* @details The Elevator class is responsible for moving the elevator between
* the floors and signalling the IO class via a semaphore of when it has
* changed floors or status. This way, the IO class can respond by updating the 
* display console. 
*	It is also responsible for responding to different inputs such as fault
* or termination requests. 
*	The Elevator class contains a priority queue to store pending requests
* and decide which floor has higher priority and move the elevator to that
* floor. 
*/
class Elevator : public ActiveClass {

public:

	/**
	* Constructor of the Elevator class that initializes the variables.
	*/
	Elevator(int elevatorNumber);

	/**
	* @details Destructor of the class.
	*/
	~Elevator();

private:
	
	/**
	* The elevator number.
	*/
	int _elevatorNumber;

	/**
	* Destination floor to go to.
	*/
	int _destinationFloor;

	/**
	* Destination status of the elevator.
	*/
	int _destinationStatus;

	/**
	* Direction of the elevator.
	*/
	char _direction;

	/**
	* @details Mutex between the elevator and dispatcher. Since this algorithm
	* allows for dynamic allocation of elevators, it would be more efficient to use
	* one mutex instead of (2 * number of elevators) consumer and producer
	* semaphores. This mutex is used right before the FindClosestElevator()
	* algorithm. This is because we need to freeze the elevators temporarily
	* to compare the elevators to find the closest elevators. If we don't do this,
	* then the elevator might move as the FindClosestElevator() is trying to find
	* the right elevator which could result in an error.
	*/
	CMutex _DispatcherElevatorMutex;

	/**
	* Elevator datapool
	*/
	CDataPool _elevatorDataPool;

	/**
	* Elevator datapool pointer
	*/
	dataPoolData *_elevatorDataPoolPtr;

	/**
	* The pipeline to receive elevator call information from outside the elevator.
	*/
	CPipe _pipeOutside;

	/**
	* The pipeline to receive elevator call information from inside the elevator.
	*/
	CPipe _pipeInside;

	/**
	* The pipeline to receive fault or termination information to the dispatcher.
	*/
	CPipe _faultPipe;

	/**
	* Char array containing the fault input from the user.
	*/
	char _faultInput[2];

	/**
	* This is the consumer semaphore used between the IO and Elevator. It is used
	* in conjunction with the producer semaphore to signal the IO to update the
	* console display.
	*/
	CSemaphore _IOElevatorSemaphoreP;

	/**
	* This is the consumer semaphore used between the IO and Elevator. It is used
	* in conjunction with the producer semaphore to signal the IO to update the
	* console display.
	*/
	CSemaphore _IOElevatorSemaphoreC;

	/**
	* Initialize the priority queue to store pending requests for the elevators.
	*/
	std::priority_queue<queueData> _destinationPQ;

	/**
	* @detail The main of this active polls for the elevator call. It initializes 
	* the main thread to be run on the elevator.
	*/
	int main(void);

	/**
	* @details Polls for the three pipelines for the elevator call dispatched by
	* the dispatcher. 
	*/
	void PollForElevatorCall();

	/**
	* @details Tests the pipe to see if there is a fault or termination request.
	*	If there is a request to terminate, then it sets the datapool information
	* to close the doors. It then removes all the pending requests and calls
	* the GoToFloor() function to go to floor zero.
	*	If there is a request to fault one of the elevators. It updates the
	* datapool information to close the doors, set the fault status, set the 
	* moving status and update the direction to no direction. It thens removes
	* all pending requests and signals the IO to update the display.
	*	If there is a request to remove the fault on one of the elevators. It
	* updates the datapool to remove the fault status and signals the IO to 
	* update the display.
	* @return Returns true if there is a fault, false otherwise.
	*/
	bool CheckForFaultRequest();

	/**
	* @details Tests the pipe to see if there is an outside elevator request. 
	* If there is, it creates a queueData struct to store the data and push it
	* into the priority queue. It updates this struct and sets the destination
	* type to be a PICKUP, the direction and the destination floor number.
	*	If the elevator is in operation (MOVING), push the queueData into the
	* priority queue. If it is MOVING and the door is not open, then call
	* GoToFloor() to go to the floor and pickup the user.
	*	Else, if the queue is empty, update the datapool with the direction of 
	* the elevator call. This is usually the first operation that happens. Then
	* push the destination to the priority queue. If the door status is CLOSED,
	* then it means the elevator is idle and we call GoToFloor() to go to 
	* the floor.
	*/
	void CheckForOutsideElevatorRequest();

	/**
	* @details Tests the pipe to see if there is an inside elevator request. 
	* If there is, it creates a queueData struct to store the data and push it
	* into the priority queue. It updates this struct and sets the destination
	* type to be a DROPOFF, the direction and the destination floor number.
	*	It then closes the door and pushes the queue to the priority queue. Then
	* it clals GoToFloor().
	*/
	void CheckForInsideElevatorRequest();

	/**
	* @details This function is used to go to the different floors. It is 
	* recursive and pops off the top item in the priority queue to address all
	* the pending requests. 
	*	It first goes through a loop that updates the current floor number of the
	* elevator and signals the IO semaphore to update the elevator. At the same
	* time, in this loop, it checks for the fault request or new elevator calls.
	* When it gets a new request, it exits the while loop and addresses the new
	* request. In otherwords, it would push in the new request and the priority
	* queue will put the one with the highest priority to the top.
	*	If there are no requests and after the elevator has traversed to the
	* destination, the code will check to see if the request is a PICKUP, DROPOFF
	* or TERMINATION. 
	*	If we are at a PICKUP, the algorithm will open the door and wait for a 
	* inside elevator call. 
	*	If we are at a DROPOFF, the algorithm will open the door and let people
	* off and then close the door. Then, it will check to see if the priority 
	* queue is empty, if it is not empty it will pop the top off and move to the
	* new elevator call destination by calling GoToFloor() again (recursive calls).
	* When the priority queue is finally empty, the elevator is done and, signal
	* the semaphore to update the display.
	*	If we are at the TERMINATION (floor zero), it will open the door and 
	* signal the IO to update the display and stop polling for inputs.
	*/
	void GoToFloor();

	/** 
	* @details This function loops through the priority queue and pops off the
	* data until it is empty.
	*/
	void RemovePendingRequests();

	

};

#endif