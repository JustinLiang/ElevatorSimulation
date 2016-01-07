#include "Elevator.h"
#include "stringcat.h"

Elevator::Elevator(int elevatorNumber) :
	_elevatorNumber(elevatorNumber),
	_destinationFloor(-1),
	_DispatcherElevatorMutex("IODispatcherElevator"),
	_elevatorDataPool("Elevator" + itos(_elevatorNumber) + "Datapool", sizeof(dataPoolData)),
	_pipeOutside("PipeOutside" + itos(_elevatorNumber)),
	_pipeInside("PipeInside" + itos(_elevatorNumber)),
	_faultPipe("FaultPipe" + itos(_elevatorNumber)),
	_IOElevatorSemaphoreP("IOElevatorSemaphoreP" + itos(_elevatorNumber), 0),
	_IOElevatorSemaphoreC("IOElevatorSemaphoreC" + itos(_elevatorNumber), 1){

	_elevatorDataPoolPtr = (dataPoolData*)(_elevatorDataPool.LinkDataPool());

}


Elevator::~Elevator()
{



}

int Elevator::main() {

	PollForElevatorCall();

	return 0;

}

void Elevator::PollForElevatorCall() {

	while (1) {

		SLEEP(50);

		CheckForFaultRequest();

		CheckForOutsideElevatorRequest();
		
		CheckForInsideElevatorRequest();

	}

}


bool Elevator::CheckForFaultRequest() {

	if (_faultPipe.TestForData() >= sizeof(_faultInput)) {

		_faultPipe.Read(&_faultInput, sizeof(_faultInput));

		if (_faultInput[0] == 'e' && _faultInput[1] == 'e') {

			_IOElevatorSemaphoreP.Wait();
			_elevatorDataPoolPtr->direction = NODIR;
			_elevatorDataPoolPtr->doorStatus = CLOSED;
			_IOElevatorSemaphoreC.Signal();

			RemovePendingRequests();
			queueData floorZero;
			floorZero.destination = 0;
			floorZero.destinationStatus = TERMINATED;
			floorZero.direction = DOWN;
			_destinationPQ.push(floorZero);
			GoToFloor();

		}
		else {

			if (_faultInput[0] == '-') {

				_IOElevatorSemaphoreP.Wait();
				_elevatorDataPoolPtr->serviceStatus = FAULT;
				_elevatorDataPoolPtr->doorStatus = CLOSED;
				_elevatorDataPoolPtr->movingStatus = IDLE;
				_elevatorDataPoolPtr->direction = NODIR;
				RemovePendingRequests(); // pop off all requests
				_IOElevatorSemaphoreC.Signal();

			}
			else if (_faultInput[0] == '+') {

				_IOElevatorSemaphoreP.Wait();
				_elevatorDataPoolPtr->serviceStatus = NOFAULT;
				_elevatorDataPoolPtr->doorStatus = CLOSED;
				_elevatorDataPoolPtr->direction = NODIR;
				_IOElevatorSemaphoreC.Signal();

			}
		}

		return true;

	}

	return false;

}

void Elevator::CheckForOutsideElevatorRequest() {

	if (_pipeOutside.TestForData() >= sizeof(outsideElevatorData)) {

		outsideElevatorData elevatorCall;
		_pipeOutside.Read(&elevatorCall, sizeof(outsideElevatorData));

		queueData destination;
		destination.destination = elevatorCall.currentFloorNumber;
		destination.destinationStatus = PICKUP;
		_direction = elevatorCall.direction;
		destination.direction = _direction;
		_elevatorDataPoolPtr->desiredFloorNumber = destination.destination;

		if (_elevatorDataPoolPtr->movingStatus == MOVING) {

			_destinationPQ.push(destination);

			if (_elevatorDataPoolPtr->doorStatus != OPEN) {

				GoToFloor();

			}

		}
		else {

			if (_destinationPQ.empty()) {

				_elevatorDataPoolPtr->direction = elevatorCall.direction;

			}

			_destinationPQ.push(destination);

			if (_elevatorDataPoolPtr->doorStatus != OPEN) {

				GoToFloor();

			}

		}

	}

}

void Elevator::CheckForInsideElevatorRequest() {

	if (_pipeInside.TestForData() >= sizeof(insideElevatorData)) {

		insideElevatorData elevatorDestination;
		_pipeInside.Read(&elevatorDestination, sizeof(insideElevatorData));

		queueData destination;
		destination.destination = elevatorDestination.desiredFloorNumber;
		destination.destinationStatus = DROPOFF;
		destination.direction = _direction;

		// Close the door
		_IOElevatorSemaphoreP.Wait();
		_elevatorDataPoolPtr->doorStatus = CLOSED;
		_IOElevatorSemaphoreC.Signal();

		_destinationPQ.push(destination);

		GoToFloor();

	}

}

void Elevator::GoToFloor() {

	// TODO put mutex here?
	_elevatorDataPoolPtr->movingStatus = MOVING;

	_destinationFloor = _destinationPQ.top().destination;
	_destinationStatus = _destinationPQ.top().destinationStatus;

	// We do not need a semaphore here because the current floor number
	// is only changed by the while loop below when the elevator is moving
	if (_elevatorDataPoolPtr->currentFloorNumber < _destinationFloor) {

		while (_elevatorDataPoolPtr->currentFloorNumber != _destinationFloor) {
			
			if (CheckForFaultRequest()) {

				return;

			}

			_IOElevatorSemaphoreP.Wait();
			_DispatcherElevatorMutex.Wait();
			_elevatorDataPoolPtr->currentFloorNumber++;
			_DispatcherElevatorMutex.Signal();
			_IOElevatorSemaphoreC.Signal();
			
			if (_pipeOutside.TestForData() >= sizeof(outsideElevatorData)) {

				return;

			}

		}

	}
	else if(_elevatorDataPoolPtr->currentFloorNumber > _destinationFloor) {

		while (_elevatorDataPoolPtr->currentFloorNumber != _destinationFloor) {

			if (CheckForFaultRequest()) {

				return;

			}

			_IOElevatorSemaphoreP.Wait();
			_DispatcherElevatorMutex.Wait();
			_elevatorDataPoolPtr->currentFloorNumber--;
			_DispatcherElevatorMutex.Signal();
			_IOElevatorSemaphoreC.Signal();

			if (_pipeOutside.TestForData() >= sizeof(outsideElevatorData)) {

				return;

			}

		}

	}

	_destinationPQ.pop();

	if (_destinationStatus == PICKUP) {

		// Open the door
		_IOElevatorSemaphoreP.Wait();
		_elevatorDataPoolPtr->doorStatus = OPEN;
		_elevatorDataPoolPtr->direction = _direction;

		// Stopped moving
		_elevatorDataPoolPtr->movingStatus = IDLE;
		_IOElevatorSemaphoreC.Signal();

	}
	else if (_destinationStatus == DROPOFF) {

		// Open the door to let people off for 1 second
		_IOElevatorSemaphoreP.Wait();
		_elevatorDataPoolPtr->doorStatus = OPEN;
		_IOElevatorSemaphoreC.Signal();
		SLEEP(1000);

		// Close the door
		_IOElevatorSemaphoreP.Wait();
		_elevatorDataPoolPtr->doorStatus = CLOSED;

		// Stopped moving
		_elevatorDataPoolPtr->movingStatus = IDLE;
		_IOElevatorSemaphoreC.Signal();

		// If there are still places to go, pop it off and call this function again
		if (!_destinationPQ.empty()) {

			// Close the door
			_IOElevatorSemaphoreP.Wait();
			_elevatorDataPoolPtr->doorStatus = CLOSED;
			_IOElevatorSemaphoreC.Signal();

			GoToFloor();

		}

		_IOElevatorSemaphoreP.Wait();
		// If we are not at a pick up, then we are done
		if (_elevatorDataPoolPtr->doorStatus != OPEN) {
			_elevatorDataPoolPtr->direction = NODIR;
		}
		_IOElevatorSemaphoreC.Signal();

	}
	else if (_destinationStatus == TERMINATED) {

		_IOElevatorSemaphoreP.Wait();
		_elevatorDataPoolPtr->doorStatus = OPEN;
		_IOElevatorSemaphoreC.Signal();

	}		

}

void Elevator::RemovePendingRequests() {

	while (!_destinationPQ.empty()) {

		_destinationPQ.pop();

	}

}