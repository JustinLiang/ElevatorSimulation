#ifndef __DISPATCHER__
#define __DISPATCHER__

#include <vector>

#include "rt.h"
#include "Elevator.h"
#include "IO.h"
#include "data.h"

/**
* @details The Dispatcher class is used to handle the inputs coming froming the
* IO class. It then sends the input to the correct elevator(s). 
*/
class Dispatcher : public ActiveClass {

public:

	/**
	* Constructor that initializes the member variables.
	*/
	Dispatcher(int numOfElevators);

	/**
	* Destructor that releases the memory for all dynamically allocated objects.
	*/
	~Dispatcher();

private:

	/**
	* Total number of elevators.
	*/
	int _numOfElevators;

	/**
	* Vector of elevator datapools.
	*/
	std::vector<CDataPool*> _elevatorDataPools;

	/**
	* Vector of elevator datapool pointers.
	*/
	std::vector<dataPoolData*> _elevatorDataPoolPtrs;

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
	* Stores the elevator call for someone outisde the elevator to be sent through
	* a pipeline to the elevator.
	*/
	outsideElevatorData _elevatorCall;

	/**
	* Stores the elevator call for someone inside the elevator to be sent through
	* a pipeline to the elevator.
	*/
	insideElevatorData _elevatorDestination;

	/**
	/* @details Vector of pipelines to pipe outside elevator calls to the elevators.
	*/
	std::vector<CPipe*> _elevatorPipesOutside;

	/**
	* @details Vector of pipelines to pipe inside elevator calls to the elevators.
	*/
	std::vector<CPipe*> _elevatorPipesInside;

	/**
	* @details Vector of pipelines to pipe fault data to the elevators.
	*/
	std::vector<CPipe*> _elevatorFaultPipe;

	/**
	* @details Creates the elevator datapools and pipes and polls for the IO data
	* and sends the inputs to the elevator based on the data received from the IO.
	*/
	int main(void);

	/**
	* @details Creates the datapools and datapool pointers to give the class access 
	* to the elevator information. All of these objects are stored in a vector.
	*/
	void CreateElevatorDataPools();

	/**
	* @details Instantiates a vector of elevator pipes. There are three pipes. One
	* is the pipe for elevator calls on the outside, one of for elevator calls on
	* the inside and the last is for fault/termination calls.
	*/
	void CreateElevatorPipes();

	/**
	* @details Continuously polls for IO data. It tests for the data from the pipes.
	* If it gets a call from the outside for an elevator it calls a function to
	* find the closest elevator that is available. If it gets a call from the
	* inside of the elevator it calls a function to send the elevator to drop the
	* person off at the desired destination. If it gets a fault input, it sends
	* the fault info to the elevator. If it gets a termination input, it sends
	* the termination input to all the elevators.
	*/
	void PollForIOData();

	/**
	* @details Sends the outside elevator call to the closest elevator available
	* via a pipeline. It calls the FindClosestElevator() function to find the
	* closest elevator.
	*/
	void CallForClosestElevator();

	/**
	* @details This function runs the main dispatcher algorithm used to find the
	* closest elevator. It uses a mutex to temporarily freeze the elevators 
	* and access their datapools to find the optimal elevator.
	*	If the elevator[i] is going up and has gone past the floor where the person
	* requests for the elevator, it ignores the request. If the elevator[i] is going
	* down and has gone past the floor where the person request for the elevator,
	* it ignores the request and does nothing.
	*	Else, it checks if the elevator[i] distance to the floor where the person
	* requested for the elevator is greater than the closestDistance calculated
	* from looping through the elevators. It also checks if the call direction is
	* the same as the direction of the elevator. It also checks if the door is not
	* open and is not faulted. Essentially, in this case, we find the closest elevator
	* that is moving and can intercept the floor request. If this is true, we set
	* the closestElevator to this new elevator[i] and set the closestDistance to
	* the new distance from the elevator[i].
	*	If we cannot find an elevator, then we reloop through the elevators and 
	* take the closest one that is busy, ie. any elevator that is stopped and
	* waiting for the user to input a call inside the elevator.
	*@return Returns the number of the elevator that is closest to the floor where
	* the user made a call for the elevator from the outside.
	*/
	int FindClosestElevator();

	/**
	* @details Sends the elevator to the destination call made inside the elevator.
	*/
	void SendElevatorToDestination();

	/**
	* @details Sends the termination input 'ee' to all the elevators via a pipeline.
	*/
	void TerminateElevators();
	
	/**
	* @details Send the fault input to the elevator that needs to be faulted via
	* a pipeline.
	*/
	void SendFaultToElevator();

};

#endif
