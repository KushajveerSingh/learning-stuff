#include <thread>
#include <iostream>
#include <unordered_map>
#include <mutex>
//what's left: look over notes to understand file issues - implement get, address issues to keep server/client ok - fairly close, but still undersanding threading


//TODO: look into pointers/variables that can't/shouldn't/be careful about what should be passed between the thread and the main
//TODO: look up/understand async/future/etc. so it works consistently
//likely finished: understand this RAII method so it actually makes sense, right now it doesn't
//likely finished: RAII mutex possibly, RAII thread itself correctly
//

//while I did not create a class to specify pointers in between threads and main
//we should be careful, but it is possible using this RAII method to
//declare pointers for both, keep track of how many are being used
//and destruct once the pointer number reaches 0
//it would take a google search, but I don't understand this RAII yet
//this actually needs to be used I believe.
//
//
//for now, declaring individual char* for each thread, to keep
//main and thread pointers separate

std::unordered_map<std::string, char> file_map;
std::mutex table_m; // mutex for the table, table has a set of functions to change values, all with an associated mutex lock

int MAX_WAIT = 1000;


std::unordered_map<std::thread::id, std::thread*> thread_map; // keeps the currently alive threads in the map, key is the thread id
//note - thread id is not an integer, likely will be declaring a pseudo-integer we will have to map to an internal thread_id - or specify the thread_id when declaring the thread
//unknown if I need to delete the references to thread* for each thread(ie, memory leaks?)
int num_threads;
std::mutex thread_map_m;

int MAX_DAEMONS = 4; //to be defined in network.hpp or server.hpp


std::string file_contents = "1234567890qwertyuiop who's a test. You are!";

void create_file(){
    //
    FILE *file;
    //std::mutex mutex_2; these could be declared, but the table should do the mutex work
    //no threads could check this mutex making it useless
    std::string file_name = "test_file.txt";
    file = fopen(file_name.c_str(), "w");
    if (file == NULL) {
	perror("cannot create file to write to");
    }
    fprintf(file, "%s", file_contents.c_str());
    //
    fclose(file);
}

//add_file for start of thread, remove_file for end
//these could be implemented before starting a new method for get, but
//the idea should be similar
int active_file(std::string complete_file_str){
    int count = 0;
    std::lock_guard<std::mutex> lock(table_m);
    //table_m should be locked
    if (file_map.find(complete_file_str) == file_map.end()){
	file_map[complete_file_str] = 'A'; //TODO possible reference necessary
	return 0;
    }
    else{
	if (file_map[complete_file_str] !='A'){
	    std::cout << "char not active for unopened file" << std::endl;
	}
	return -1;
    }	    
}

int finished_file(std::string complete_file_str){
    std::lock_guard<std::mutex> lock(table_m);
    file_map.erase(complete_file_str);
    return 0;
}

class Thread_Safety{
    std::thread& main_thread;
    std::thread::id daemon_id; 

    public:
        Thread_Safety(std::thread& daemon) : main_thread(daemon){
	    thread_map_m.lock(); //exception unsafe, but without a short function call, this seems ok
	    daemon_id = daemon.get_id();
	    thread_map[daemon_id] = &daemon;
	    ++num_threads;
	    thread_map_m.unlock();
	}
	~Thread_Safety(){
	    if (main_thread.joinable()){
		main_thread.join();
	    }
	    if (thread_map.find(daemon_id) != thread_map.end()){
		thread_map_m.lock();
		thread_map.erase(daemon_id);
		--num_threads;
		thread_map_m.unlock();
	    }
	    else{
		std::cout << "possible errors from num_threads on thread deconstruction" << std::endl; 
		//decrementing num_threads may need to be outside the if statement while it was already erased from thread_map
	    }
	}
    //used for construction and deconstruction of a thread - using RAII should improve exception safety for thread deconstruction
    //I will need to look into this to determine more about mutexes and how mutexes should work in combination with thread_RAII itself
};

//void activate_thread(std::thread& daemon){
//    thread_map_m.lock();
//    std::thread::id daemon_id = daemon.get_id();
//    thread_map[daemon_id] = &daemon;
//    ++num_threads;
//    thread_map_m.unlock();
//}

void file_thread(std::string complete_file_str){
    std::cout << "in thread " << std::this_thread::get_id() << std::endl;
    active_file(complete_file_str);
    create_file();
    finished_file(complete_file_str);
}

int main(){
    //
    //
    // other options - std::vector
    std::string complete_file_str = "newfolder\\file_text.txt";
    if (num_threads < MAX_DAEMONS){
        std::thread daemon(file_thread, complete_file_str);
        //
	Thread_Safety safe_daemon(daemon); //I don't know if these two lines could be joined into one, but that would seem better
    }
    else{
	std::cout << "Max number of " << MAX_DAEMONS << "reached, unable to process request" << std::endl;
    }
    return 0;
}

