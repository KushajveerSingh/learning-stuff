execute-server:
	g++ server.cpp -o myftpserver -lpthread
	./myftpserver 8800 9800

execute-alt-server:
	g++ server.cpp -o myftpserver -lpthread
	./myftpserver 8850 9850

execute-client:
	g++ client.cpp -o myftp -lpthread
	./myftp 0.0.0.0 8800 9800

texecute-test_thread:
	g++ test_thread.cpp -o test_thread -lpthread
	./test_thread
