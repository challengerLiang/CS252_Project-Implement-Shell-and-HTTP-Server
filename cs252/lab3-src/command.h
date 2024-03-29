
#ifndef command_h
#define command_h

// Command Data Structure
struct SimpleCommand {
	// Available space for arguments currently preallocated
	int _numOfAvailableArguments;

	// Number of arguments
	int _numOfArguments;
	char ** _arguments;
	
	SimpleCommand();
	void insertArgument( char * argument );
};

struct Command {
	int _numOfAvailableSimpleCommands;
	int _numOfSimpleCommands;
	SimpleCommand ** _simpleCommands;
	char * _outFile;
	char * _inFile;
	char * _errFile;
	int _background;
	/*add code by myself*/
    int _append;
    int _ambiguousOut;
	/*add code by myself end*/

	void prompt();
	void print();
	void execute();
	void clear();
	
	Command();
	void insertSimpleCommand( SimpleCommand * simpleCommand );

	//static void expandWildcardsIfNecessary(char * arg);

	static Command _currentCommand;
	static SimpleCommand *_currentSimpleCommand;
};

#endif
