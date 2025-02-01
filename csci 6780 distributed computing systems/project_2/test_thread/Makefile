execute-server:
	g++ server.cpp -o myftpserver -lpthread
	./myftpserver 8800 9800

execute-client:
	g++ client.cpp -o myftp -lpthread
	./myftp 0.0.0.0 8800 9800

ex-test_thread:
	g++ test_thread.cpp -o test_thread -lpthread
	./test_thread
