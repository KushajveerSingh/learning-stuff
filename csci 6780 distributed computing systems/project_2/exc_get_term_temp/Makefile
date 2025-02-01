compile-server:
	g++ --std=c++11 -pthread server.cpp -o myftpserver

compile-client:
	g++ client.cpp -o myftp

execute-server:
	g++ --std=c++11 -pthread server.cpp -o myftpserver
	./myftpserver 8800 9800

execute-client:
	g++ --std=c++11 -pthread client.cpp -o myftp
	./myftp 0.0.0.0 8800 9800

execute-server1:
	g++ --std=c++11 -pthread server.cpp -o myftpserver
	./myftpserver 8801 9801

execute-client1:
	g++ --std=c++11 -pthread client.cpp -o myftp
	./myftp 0.0.0.0 8801 9801
