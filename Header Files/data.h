#ifndef __DATA__
#define __DATA__

const char OPEN = 'o';
const char CLOSED = 'c';
const char UP = 'u';
const char DOWN = 'd';
const char NODIR = 'n';
const char MOVING = 'm';
const char IDLE = 'i';
const char PICKUP = 'p';
const char DROPOFF = 'd';
const char FAULT = 'f';
const char NOFAULT = 'n';
const char TERMINATED = 't';

/**
* @details The struct data that is stored in the datapool and is used to store
* the various status' of the elevators:
*	- direction: the direction (up, down, nodir) of the elevator
*	- doorStatus: the door status of the elevator (closed, open)
*	- movingStatus: the status describing whether the elevator is moving
*	- serviceStatus: whether the elevator is faulted or not
*	- currentFloorNumber: the current floor number the elevator is on
*	- desiredFloorNumber: the floor number that the elevator needs to go to
*/
struct dataPoolData {

	char direction; // 'u' up, 'd' down, 'n' no direction
	char doorStatus; // 'c' closed, 'o' open
	char movingStatus; // 'm' moving, 'i' idle
	char serviceStatus; // 'f' fault, 'n' no fault
	int currentFloorNumber;
	int desiredFloorNumber;

};

/**
* @details The struct containing the elevator call made by someone outside the
* elevators. 
*	- direction: the direction the person wants to go
*	- currentFloorNumber: the current floor number of the person making the call
*/
struct outsideElevatorData {

	char direction;
	int currentFloorNumber;
	
};

/**
* @details The struct containing the elevator call made by someone inside the
* elevators.
*	- currentElevatorNumber: the current elevator number the person is in
*	- desiredFloorNumber: the floor the person wishes to go to
*/
struct insideElevatorData{

	int currentElevatorNumber;
	int desiredFloorNumber;

};

/**
* @details The struct data that goes in the priority queue used by the elevators.
*	- destination: the destination of the call
*	- destinationStatus: whether the destination is a PICKUP, DROPOFF or TERMINATE
*	- direction: the direction of the call
* The function also overloads the operator <. It is a max priority queue if the
* elevator is going down. In otherwords, it wants to go to the larger floor numbers
* first. It is a min priority queue if the elevator is going up. In otherwords,
* it wants to go to the lower floor numbers first. Ie. If there is a queue to go
* to floors 3, 5 and 7 and the elevator is going up, it should go to floor 3 first
* then floor 5 and finally floor 7.
*/
struct queueData {

	int destination;
	char destinationStatus; // 'p' pickup, 'd' dropoff, 't' terminate
	char direction; // 'u' up, 'd' down

	bool operator<(const queueData &o) const
	{
		if (direction == UP) {

			return destination > o.destination;

		}else if(direction == DOWN) {

			return destination < o.destination;

		}
	}

};

#endif