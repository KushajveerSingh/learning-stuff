#include <fstream>
#include <string>
#include <iostream>
#include <time.h>
using namespace std;

//this is a file showing the syntax for creating a log file with timestamp
//this can be modified into a module or fit directly into client or server
//the purpose was to show a viable method to use the syntax within C++

//where the log is stored
const string LOG_FILE = "log.txt";

bool log_store(string, bool);
bool log_store_pop(string, bool); //basic functionality for 'ls' has been checked
//error functionality still questionable

bool log_store(string command, bool output){
	//this opens a file stream and inputs the command written
	//the boolean adds the command output to the log file (ie using 'ls')
  
  	//it closes it every time, which is likely suboptimal
	//closer to project end a better solution of closing the stream on quit can be implemented
       	
	ofstream file;
      	file.open(LOG_FILE);
	time_t cur_time = time(NULL);
	file<<command.c_str()<<'\n';
	file<<ctime(&cur_time);
	file.close();

	if (output){
		//appends return of command to the end of the log
		
		string string_command = command + ">>" + LOG_FILE;
		const char * final_command = string_command.c_str();
		system(final_command);
	}

	return 0;
}


bool log_store_pop(string command, bool output){
	//this opens a file stream and inputs the command written
	//the boolean adds the command output to the log file (ie using 'ls')
  
  	//it closes it every time, which is likely suboptimal
	//closer to project end a better solution of closing the stream on quit can be implemented
       	
	ofstream file;
      	file.open(LOG_FILE);
	time_t cur_time = time(NULL);
	file<<command.c_str()<<'\n';
	file<<ctime(&cur_time);

	if (output){
		FILE *return_file;
		int buff_len = 255;
		char buff[buff_len];
		string return_string;
		
		//appends return of command to the end of the log
		return_file = popen(command.c_str(), "r"); 
		if (return_file == NULL){
			string err = "error on return_file";
			file<<err<<endl;
			file.close();
			throw err;
			//runtime_error(err);
		};
		while ( fgets(buff, buff_len, return_file) != NULL){
			return_string += buff;
		}
		file<<return_string;
		cout<<return_string<<endl; //this should go to the client as well/instead
		if (pclose(return_file) < 0){
			string err = "error using pclose()";
			file<<err<<endl;
			file.close();
			throw err;

			//runtime_error(err);
		}

	}
	
	file.close();
	return 0;
}

int main () {
	// a sample to run the 'ls' command
  cout<<"printing a command to file: "<<LOG_FILE<<endl;
	
	string basic_command = "ls";
	return log_store_pop(basic_command, 1);
}
