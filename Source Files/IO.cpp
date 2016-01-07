#include "IO.h"
#include "Dispatcher.h"
#include "stringcat.h"
#include <iostream>

using namespace std;

IO::IO() :
	_displaySemaphore("displaySemaphore", 1),
	_pipeOutside("PipeOutside", 1024),
	_pipeInside("PipeInside", 1024),
	_faultPipe("FaultPipe", 1024) {

}

IO::~IO()
{

	for (int i = 0; i < _numOfElevators; i++) {

		delete _elevators[i];
		delete _elevatorDataPools[i];
		delete _elevatorDataPoolPtrs[i];
		delete _displayThreads[i];
		delete _IOElevatorSemaphoresC[i];
		delete _IOElevatorSemaphoresP[i];

	}

	delete _dispatcher;

}

int IO::main(void) {

	GetNumberOfElevators();

	CreateElevatorSystem();
	CreateElevatorDataPools();

	CreateSemaphores();

	// Get user input and put in pipe
	ClassThread <IO> Thread1(this, &IO::PollForUserInput, ACTIVE, NULL); 

	InitializeDisplay();
	UpdateDisplays();

	while (1);

	return 0;

}

void IO::GetNumberOfElevators() {

	cout << "Enter the number of elevators: ";
	cin >> _numOfElevators;
		
}

void IO::CreateElevatorSystem() {

	for (int i = 0; i < _numOfElevators; i++) {

		CreateElevator(i);

	}

	CreateDispatcher();

}

void IO::CreateElevator(int i) {

	_elevators.push_back(new Elevator(i));
	_elevators[i]->Resume();

}

void IO::CreateElevatorDataPools() {

	for (int i = 0; i < _numOfElevators; i++) {

		_elevatorDataPools.push_back(new CDataPool("Elevator" + itos(i) + "Datapool", sizeof(struct dataPoolData)));
		_elevatorDataPoolPtrs.push_back((dataPoolData*)(_elevatorDataPools[i]->LinkDataPool()));
		_elevatorDataPoolPtrs[i]->direction = NODIR;
		_elevatorDataPoolPtrs[i]->doorStatus = CLOSED;
		_elevatorDataPoolPtrs[i]->movingStatus = IDLE;
		_elevatorDataPoolPtrs[i]->serviceStatus = NOFAULT;
		_elevatorDataPoolPtrs[i]->currentFloorNumber = 0;
		_elevatorDataPoolPtrs[i]->desiredFloorNumber = 0;

	}

}

void IO::CreateDispatcher() {

	_dispatcher = new Dispatcher(_numOfElevators);
	_dispatcher->Resume();

}

void IO::CreateSemaphores(){

	for (int i = 0; i < _numOfElevators; i++) {

		_IOElevatorSemaphoresP.push_back(new CSemaphore("IOElevatorSemaphoreP" + itos(i), 0));
		_IOElevatorSemaphoresC.push_back(new CSemaphore("IOElevatorSemaphoreC" + itos(i), 1));

	}

}

int IO::PollForUserInput(void *ThreadArgs) {

	char userInput[2];
	
	while (1) {

		PrintGetUserCommand();

		userInput[0] = _getch();
		userInput[1] = _getch();

		PrintUserCommand(userInput[0], userInput[1]);
		
		// If you have a fault command, send it over
		if (((userInput[0] == '-') &&
			(userInput[1] >= 0 + 0x30 && userInput[1] < _numOfElevators + 0x30)) 
			|| 
			((userInput[0] == '+') &&
			(userInput[1] >= 0 + 0x30 && userInput[1] < _numOfElevators + 0x30))) {

			_faultPipe.Write(userInput, sizeof(userInput));

		}
		// If you have a terminate command, send it over and stop polling
		else if (userInput[0] == 'e' && userInput[1] == 'e') {

			_faultPipe.Write(userInput, sizeof(userInput));
			return 0;

		}
		// Check if input is valid and check whether it's an outside or inside the elevator input
		else if ((userInput[0] == 'u' || userInput[0] == 'd') &&
			(userInput[1] >= 0 + 0x30 && userInput[1] <= 9 + 0x30)) {

			_elevatorCall.direction = userInput[0];
			_elevatorCall.currentFloorNumber = userInput[1] - 0x30;
			_pipeOutside.Write(&_elevatorCall, sizeof(outsideElevatorData));

		}
		else if ((userInput[0] >= 0 + 0x30 && userInput[0] < _numOfElevators + 0x30) &&
			(userInput[1] >= 0 + 0x30 && userInput[1] <= 9 + 0x30)) {

				_elevatorDestination.currentElevatorNumber = userInput[0] - 0x30;
				_elevatorDestination.desiredFloorNumber = userInput[1] - 0x30;
				_pipeInside.Write(&_elevatorDestination, sizeof(insideElevatorData));

		}

	}

	return 0;
	
}

void IO::InitializeDisplay() {

	_displaySemaphore.Wait();

	PrintTitle();

	for (int i = 0; i < 10; i++){

		MOVE_CURSOR(0, i * columnSpacing + columnSpacing + 10);
		cout << 10 - i - 1;

	}

	int x = 5 + _numOfElevators * rowSpacing;
	int y = 10 * columnSpacing - 10 * columnSpacing - 2 + 10;

	// Display elevators
	TEXT_COLOUR(0, 14);
	for (int i = 0; i < _numOfElevators; i++){

		for (int j = 0; j < x; j++) {

			MOVE_CURSOR(50, 100);
			cout << " ";

		}

	}
	_displaySemaphore.Signal();

}

void IO::UpdateDisplays() {

	CSemaphore elevatorMutex("elevatorSemaphore", 0);

	CURSOR_OFF();

	for (int i = 0; i < _numOfElevators; i++){

		_displayThreads.push_back(new ClassThread <IO>(this, &IO::UpdateDisplay, ACTIVE, &i));
		elevatorMutex.Wait();

	}

}

int IO::UpdateDisplay(void *ThreadArgs) {

	CSemaphore elevatorMutex("elevatorSemaphore", 0);
	int x;
	int y;
	int floorNum;
	int elevator = *static_cast<int*>(ThreadArgs);
	elevatorMutex.Signal();
	while (1) {

		_IOElevatorSemaphoresC[elevator]->Wait();
		_displaySemaphore.Wait();

		SLEEP(500);
		floorNum = _elevatorDataPoolPtrs[elevator]->currentFloorNumber;
		x = 5 + elevator * rowSpacing;
		y = 10 * columnSpacing - floorNum * columnSpacing - 2 + 10;

		Erase(x);
		UpdateElevator(elevator, x, y);
		UpdateElevatorDirection(elevator, x);
		
		_displaySemaphore.Signal();
		_IOElevatorSemaphoresP[elevator]->Signal();

	}
	
	return 0;

}

void IO::Erase(int x) {

	// Erase
	TEXT_COLOUR(15, 0);
	for (int i = 0; i < 10; i++){

		for (int j = 0; j < 5; j++) {

			for (int k = 0; k < 4; k++) {

				MOVE_CURSOR(x + j, 10 * columnSpacing - 2 - i * columnSpacing + k + 10);
				cout << " ";

			}

		}

	}

}

void IO::UpdateElevator(int elevator, int x, int y) {

	// Move elevator
	if (_elevatorDataPoolPtrs[elevator]->doorStatus != OPEN) {

		int textColour;

		if (_elevatorDataPoolPtrs[elevator]->serviceStatus == FAULT) {

			textColour = 12;

		}
		else {

			textColour = 10;

		}

		TEXT_COLOUR(0, textColour);
		for (int j = 0; j < 5; j++) {

			for (int k = 0; k < 4; k++) {

				MOVE_CURSOR(x + j, y + k);
				cout << " ";

			}

		}

	}
	else {

		// Open the door
		TEXT_COLOUR(0, 10);
		for (int j = 0; j < 5; j++) {

			for (int k = 0; k < 4; k++) {

				MOVE_CURSOR(x + j, y + k);
				cout << " ";

			}

		}
		TEXT_COLOUR(15, 0);
		for (int j = 1; j < 4; j++) {

			for (int k = 1; k < 4; k++) {

				MOVE_CURSOR(x + j, y + k);
				cout << " ";

			}

		}

	}

}

void IO::UpdateElevatorDirection(int elevator, int x) {

	if (_elevatorDataPoolPtrs[elevator]->direction == UP) {
		TEXT_COLOUR(14, 0);
		MOVE_CURSOR(x + 1, 8);
		cout << " /\\";
		MOVE_CURSOR(x + 1, 9);
		cout << "/||\\";
		MOVE_CURSOR(x + 1, 10);
		cout << " || ";
	}
	else if (_elevatorDataPoolPtrs[elevator]->direction == DOWN) {
		TEXT_COLOUR(14, 0);
		MOVE_CURSOR(x + 1, 8);
		cout << " ||";
		MOVE_CURSOR(x + 1, 9);
		cout << "\\||/";
		MOVE_CURSOR(x + 1, 10);
		cout << " \\/";
	}
	else {

		TEXT_COLOUR(14, 0);
		MOVE_CURSOR(x + 1, 8);
		cout << "     ";
		MOVE_CURSOR(x + 1, 9);
		cout << "     ";
		MOVE_CURSOR(x + 1, 10);
		cout << "     ";

	}

}

void IO::PrintGetUserCommand() {

	_displaySemaphore.Wait();
	TEXT_COLOUR(15, 0);
	MOVE_CURSOR(0, 55);
	cout << "Enter elevator command:";
	_displaySemaphore.Signal();

}

void IO::PrintUserCommand(char input1, char input2) {

	_displaySemaphore.Wait();
	TEXT_COLOUR(15, 0);
	MOVE_CURSOR(24, 55);
	cout << input1 << input2;
	_displaySemaphore.Signal();

}

void IO::PrintTitle() {

	MOVE_CURSOR(0, 0);

	const char* title =
		" _______  __   __  _______  _______  ______      _______  ___   __   __  _______    _______  _______  _______  _______ \n"
		"|       ||  | |  ||       ||       ||    _ |    |       ||   | |  |_|  ||       |  |       ||  _    ||  _    ||  _    |\n"
		"|  _____||  | |  ||    _  ||    ___||   | ||    |  _____||   | |       ||  _____|  |____   || | |   || | |   || | |   |\n"
		"| |_____ |  |_|  ||   |_| ||   |___ |   |_||_   | |_____ |   | |       || |_____    ____|  || | |   || | |   || | |   |\n"
		"|_____  ||       ||    ___||    ___||    __  |  |_____  ||   | |       ||_____  |  | ______|| |_|   || |_|   || |_|   |\n"
		" _____| ||       ||   |    |   |___ |   |  | |   _____| ||   | | ||_|| | _____| |  | |_____ |       ||       ||       |\n"
		"|_______||_______||___|    |_______||___|  |_|  |_______||___| |_|   |_||_______|  |_______||_______||_______||_______|\n";
	
	TEXT_COLOUR(12, 0);
	cout << title << endl;

}

