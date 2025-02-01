compile:
	g++ client.cpp -o myftp
	g++ server.cpp -o myftpserver

run-client:
	./myftp 127.0.0.1 8800

run-server:
	./myftpserver 8800

execute-client:
	g++ client.cpp -o myftp
	./myftp 127.0.0.1 8800

execute-server:
	g++ server.cpp -o myftpserver
	./myftpserver 8800

copy:
	cp client.cpp network.hpp ../src/client
	cp server.cpp server.hpp network.hpp ../src/server

aes-execute-server:
	g++ sta_server.cpp -o myftpserver
	./myftpserver 8800

aec-execute-client:
	g++ cta_client.cpp -o myftp
	./myftp 127.0.0.1 8800

aeco-execute-client:
	g++ cta_client.cpp -o myftp
	./myftp 128.192.101.137 8800
