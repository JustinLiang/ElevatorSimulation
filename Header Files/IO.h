#ifndef __IO__
#define __IO__

#include "rt.h"
#include "data.h"
#include "Elevator.h"

#include <string>
#include <vector>

class Dispatcher; // Forward declare the Dispatcher class

const int columnSpacing = 4; // Spacing for the columns
const int rowSpacing = 10; // Spacing for the rows

/**
* @details The IO class is responsible for instantiating the entire elevator system,
* getting the user input and updating the display on the screen. 
*/
class IO : public ActiveClass {

public:

	/**
	* Constructor that initializes the member variables.
	*/
	IO();

	/**
	* Destructor that releases the memory for all dynamically allocated objects.
	*/
	~IO();

private:

	/**
	* Number of elevators.
	*/
	int _numOfElevators;

	/**
	* Vector of elevator objects.
	*/
	std::vector<Elevator*> _elevators;

	/**
	* Vector of elevator datapools.
	*/
	std::vector<CDataPool*> _elevatorDataPools;
	
	/**
	* Vector of elevator datapool pointers.
	*/
	std::vector<dataPoolData*> _elevatorDataPoolPtrs;

	/**
	* Dispatcher object.
	*/
	Dispatcher* _dispatcher;

	/**
	* Semaphore used to prevent simulataneuous updating of the display by several
	* functions.
	*/
	CSemaphore _displaySemaphore;

	/**
	* Vector of display ClassThreads.
	*/
	std::vector<ClassThread <IO>*> _displayThreads;

	/**
	* Vector of IO/Elevator producer semaphores.
	*/
	std::vector<CSemaphore*> _IOElevatorSemaphoresP;

	/**
	* Vector of IO/Elevator consumer semaphores.
	*/
	std::vector<CSemaphore*> _IOElevatorSemaphoresC;
	
	/**
	* The pipeline to send elevator call information from outside the elevator.
	*/
	CPipe _pipeOutside;

	/**
	* The pipeline to send elevator call information from inside the elevator.
	*/
	CPipe _pipeInside;

	/**
	* The pipeline to send fault or termination information to the dispatcher.
	*/
	CPipe _faultPipe;

	/**
	* Stores the elevator call for someone outisde the elevator to be sent through
	* a pipeline to the dispatcher.
	*/
	outsideElevatorData _elevatorCall;

	/**
	* Stores the elevator call for someone inside the elevator to be sent through
	* a pipeline to the dispatcher.
	*/
	insideElevatorData _elevatorDestination;

	/**
	* @details This is the active class' main function that gets the number
	* of elevators and calls the functions to instantiate the elevators, dispatcher,
	* datapools and semaphores. It also calls the threads to poll for the user
	* input and update the console display.
	*/
	int main(void);

	/**
	* @details Get the number of elevators from the user.
	*/
	void GetNumberOfElevators();

	/**
	* @details Instantiates the elevators and dispatcher objects.
	*/
	void CreateElevatorSystem();

	/**
	* @details Instantiates a vector of elevator objects.
	* @param[in] i The elevator number you wish to create.
	*/
	void CreateElevator(int i);

	/**
	* @details Instantiates a vector of elevator data pools and
	* assigns them with default values.
	*/
	void CreateElevatorDataPools();

	/**
	* @details Instantiates a vector of dispatcher objects.
	*/
	void CreateDispatcher();

	/**
	* @details Instantiates a vector of producer consumer semaphores
	* that will be used between the IO and elevator objects.
	*/
	void CreateSemaphores();

	/**
	* @details Polls for the user input. If a valid input is entered
	* it sends it via a pipe to the dispatcher.
	* @return Returns 0 when 'ee' is pressed and stops polling.
	*/
	int PollForUserInput(void *ThreadArgs);

	/**
	* @details Initializes the console display. First it prints the title
	* and then it prints out the floor numbers. Finally, it prints out the
	* elevators with a green text colour.
	*/
	void InitializeDisplay();

	/**
	* @details Turns the cursor off and creates a vector of display ClassThreads
	* that runs the UpdateDisplay thread for each elevator.
	*/
	void UpdateDisplays();

	/**
	* @details Updates the display of the elevators with the new status
	* updates. It first erases the current display of the elevator. It then
	* updates the elevator location, fault status and door status.
	* @return Returns 0 when the thread is done.
	*/
	int UpdateDisplay(void *ThreadArgs);

	/**
	* @details Loops through the elevator graphics for one of the elevators
	* and erases it from the console display.
	* @param[in] x The x location of where the elevator is.
	*/
	void Erase(int x);

	/**
	* @details Updates the elevator to display it's fault status (turns red
	* if the elevator is faulted). Moves the elevator to the new location
	* based on the inputs x and y. Opens the elevator door based on the status in
	* the datapool.
	* @param[in] elevator The elevator number.
	* @param[in] x The x location of the elevator.
	* @param[in] y The y location of the elevator.
	*/
	void UpdateElevator(int elevator, int x, int y);

	/**
	* @details Updates the display with an arrow showing whether the elevator
	* is going up or down.
	* @param[in] elevator The elevator number.
	* @param[in] x The x location of the elevator.
	*/
	void UpdateElevatorDirection(int elevator, int x);

	/**
	* @details Prints out a statement to get the user command.
	*/
	void PrintGetUserCommand();
	
	/**
	* @details Prints out the user command.
	*/
	void PrintUserCommand(char input1, char input2);
	
	/**
	* @details Prints out the title "SUPER SIMS 2000" in fancy ASCII text.
	*/
	void PrintTitle();

};

#endif