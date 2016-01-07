#include "Dispatcher.h"
#include "stringcat.h"

Dispatcher::Dispatcher(int numOfElevators) :
	_numOfElevators(numOfElevators),
	_DispatcherElevatorMutex("_DispatcherElevatorMutex"),
	_pipeOutside("PipeOutside", 1024),
	_pipeInside("PipeInside", 1024),
	_faultPipe("FaultPipe", 1024) {

	_elevatorCall.currentFloorNumber = 0;
	_elevatorCall.direction = NODIR;
	_elevatorDestination.currentElevatorNumber = 0;
	_elevatorDestination.desiredFloorNumber = 0;

}

Dispatcher::~Dispatcher() {

	for (int i = 0; i < _numOfElevators; i++) {

		delete _elevatorPipesOutside[i];
		delete _elevatorPipesInside[i];
		delete _elevatorDataPools[i];
		delete _elevatorDataPoolPtrs[i];
		delete _elevatorFaultPipe[i];

	}

}

int Dispatcher::main(void) {

	CreateElevatorDataPools();

	CreateElevatorPipes();

	PollForIOData();

	return 0;

}

void Dispatcher::CreateElevatorDataPools() {

	for (int i = 0; i < _numOfElevators; i++) {

		_elevatorDataPools.push_back(new CDataPool("Elevator" + itos(i) + "Datapool", sizeof(struct dataPoolData)));
		_elevatorDataPoolPtrs.push_back((dataPoolData*)(_elevatorDataPools[i]->LinkDataPool()));

	}

}

void Dispatcher::CreateElevatorPipes() {

	for (int i = 0; i < _numOfElevators; i++) {

		_elevatorPipesOutside.push_back(new CPipe("PipeOutside" + itos(i), 1024));
		_elevatorPipesInside.push_back(new CPipe("PipeInside" + itos(i), 1024));
		_elevatorFaultPipe.push_back(new CPipe("FaultPipe" + itos(i), 1024));

	}

}

void Dispatcher::PollForIOData() {

	while (1) {
		
		SLEEP(50);

		if (_pipeOutside.TestForData() >= sizeof(outsideElevatorData)) {


			_pipeOutside.Read(&_elevatorCall, sizeof(outsideElevatorData));
			CallForClosestElevator();

		}
		if (_pipeInside.TestForData() >= sizeof(insideElevatorData)) {

			_pipeInside.Read(&_elevatorDestination, sizeof(insideElevatorData));
			
			if (_elevatorDataPoolPtrs[_elevatorDestination.currentElevatorNumber]->doorStatus == OPEN) {
				
				SendElevatorToDestination();

			}


		}
		if (_faultPipe.TestForData() >= sizeof(_faultInput)) {

			_faultPipe.Read(&_faultInput, sizeof(_faultInput));
			
			if (_faultInput[0] == 'e' && _faultInput[1] == 'e') {

				TerminateElevators();

			}
			else {

				SendFaultToElevator();

			}

		}

	}

}

void Dispatcher::CallForClosestElevator() {

	int closestElevator = -1;

	// Instead of having x number of producer consumer semaphores, we use
	// one mutex for all the elevators and stops the elevators briefly to figure 
	// out which elevator is closest by accessing all their data pools
	_DispatcherElevatorMutex.Wait();

	SLEEP(50);
	closestElevator = FindClosestElevator();

	// Skip if no elevator is available at all
	if (closestElevator == -1) {

		return;

	}

	// If elevator found is in the wrong direction then skip it
	if (_elevatorDataPoolPtrs[closestElevator]->direction != NODIR 
		&& _elevatorDataPoolPtrs[closestElevator]->direction != _elevatorCall.direction) {

		return;

	}

	_DispatcherElevatorMutex.Signal();

	_elevatorPipesOutside[closestElevator]->Write(&_elevatorCall, sizeof(outsideElevatorData));

}

int Dispatcher::FindClosestElevator() {

	// Initialize variables
	int closestElevator = -1;
	int closestDistance = 1000;
	char userDirection = _elevatorCall.direction;
	int newDistance;
	char newDirection;
	char doorStatus;
	char serviceStatus;

	int destination = _elevatorCall.currentFloorNumber;

	// Try to find the closest elevator that is free
	for (int newElevator = 0; newElevator < _numOfElevators; newElevator++) {

		newDistance = abs(_elevatorDataPoolPtrs[newElevator]->currentFloorNumber - _elevatorCall.currentFloorNumber);
		newDirection = _elevatorDataPoolPtrs[newElevator]->direction;
		doorStatus = _elevatorDataPoolPtrs[newElevator]->doorStatus;
		serviceStatus = _elevatorDataPoolPtrs[newElevator]->serviceStatus;

		// If the elevator[i] is going up and has gone past the floor where the person
		// requests for the elevator, it ignores the request.If the elevator[i] is going
		// down and has gone past the floor where the person request for the elevator,
		// it ignores the request and does nothing.
		if ((_elevatorDataPoolPtrs[newElevator]->currentFloorNumber > destination
			&& newDirection == UP)
			||
			(_elevatorDataPoolPtrs[newElevator]->currentFloorNumber < destination
			&& newDirection == DOWN)) {

			// do nothing

		}
		// Else, it checks if the elevator[i] distance to the floor where the person
		// requested for the elevator is greater than the closestDistance calculated
		// from looping through the elevators.It also checks if the call direction is
		// the same as the direction of the elevator.It also checks if the door is not
		// open and is not faulted.Essentially, in this case, we find the closest elevator
		// that is moving and can intercept the floor request.If this is true, we set
		// the closestElevator to this new elevator[i] and set the closestDistance to
		// the new distance from the elevator[i].
		else if ((newDistance < closestDistance) && ((newDirection == userDirection) || (newDirection == NODIR))
			&& (doorStatus != OPEN) && (serviceStatus == NOFAULT)) {

			// Check to see if the call is before the current desired destination of the elevator
			if (_elevatorDataPoolPtrs[newElevator]->movingStatus == MOVING) {

				if (newDistance <= abs(_elevatorDataPoolPtrs[newElevator]->desiredFloorNumber - _elevatorDataPoolPtrs[newElevator]->currentFloorNumber)){

					closestElevator = newElevator;
					closestDistance = newDistance;

				}

			}
			else {

				closestElevator = newElevator;
				closestDistance = newDistance;

			}

		}		

	}

	// If we cannot find an elevator, then we reloop through the elevators and
	// take the closest one that is busy, ie.any elevator that is stopped and
	// waiting for the user to input a call inside the elevator.
	if (closestElevator == -1) {

		for (int newElevator = 0; newElevator < _numOfElevators; newElevator++) {

			newDistance = abs(_elevatorDataPoolPtrs[newElevator]->currentFloorNumber - _elevatorCall.currentFloorNumber);
			newDirection = _elevatorDataPoolPtrs[newElevator]->direction;
			serviceStatus = _elevatorDataPoolPtrs[newElevator]->serviceStatus;

			// If the elevator[i] is going up and has gone past the floor where the person
			// requests for the elevator, it ignores the request.If the elevator[i] is going
			// down and has gone past the floor where the person request for the elevator,
			// it ignores the request and does nothing.
			if ((_elevatorDataPoolPtrs[newElevator]->currentFloorNumber > destination
				&& newDirection == UP)
				||
				(_elevatorDataPoolPtrs[newElevator]->currentFloorNumber < destination
				&& newDirection == DOWN)) {

			}
			// Find the elevator with the closest distance and is going in the same direction
			else if ((newDistance <= closestDistance) && ((newDirection == userDirection) || (newDirection == NODIR))
				&& (serviceStatus == NOFAULT)) {

				closestElevator = newElevator;
				closestDistance = newDistance;

			}			

		}

	}

	return closestElevator;

}

void Dispatcher::SendElevatorToDestination() {

	int desiredFloor = _elevatorDestination.desiredFloorNumber;
	int elevatorNumber = _elevatorDestination.currentElevatorNumber;
	int currentFloor = _elevatorDataPoolPtrs[elevatorNumber]->currentFloorNumber;
	char direction = _elevatorDataPoolPtrs[elevatorNumber]->direction;
	char fault = _elevatorDataPoolPtrs[elevatorNumber]->serviceStatus;

	// If the user presses up and his direction is up, go up
	// If the user presses down and his direction is down, go down
	// Otherwise, the user initially called the elevator to go up but now wants to
	// go down which is bad
	if (((desiredFloor >= currentFloor) && (direction == UP)) ||
		((desiredFloor <= currentFloor) && (direction == DOWN))
		&&
		(fault != FAULT)) {

		_elevatorPipesInside[elevatorNumber]->Write(&_elevatorDestination, sizeof(insideElevatorData));

	}

}

void Dispatcher::TerminateElevators() {

	for (int i = 0; i < _numOfElevators; i++) {

		_elevatorFaultPipe[i]->Write(&_faultInput, sizeof(_faultInput));

	}

}

void Dispatcher::SendFaultToElevator() {

	char fault = _elevatorDataPoolPtrs[_faultInput[1] - 0x30]->serviceStatus;

	// Should not send if input is + and there is no fault currently
	if (!(_faultInput[0] == '+' && fault == NOFAULT)) {

		_elevatorFaultPipe[_faultInput[1] - 0x30]->Write(&_faultInput, sizeof(_faultInput));

	}

}
