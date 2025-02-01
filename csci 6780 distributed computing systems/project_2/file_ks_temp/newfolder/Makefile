compile:
	g++ client.cpp -o client.out -lpthread
	g++ server.cpp -o sever.out -lpthread

run-client:
	./client.out 127.0.0.1 8800

run-server:
	./server.out 8800

execute-client:
	g++ client.cpp -o client.out -lpthread
	./client.out 127.0.0.1 8800

execute-server:
	g++ server.cpp -o server.out -lpthread
	./server.out 8800

execute-server-a:
	g++ server.cpp -o server.out -lpthread
	./server.out 8800

aclient-execute:
	g++ client.cpp -o client.out -lpthread
	./client.out 127.0.0.1 8800

altclient-execute:
	g++ client.cpp -o client.out -lpthread
	./client.out 127.0.0.1 8850

fresh_and_clean:
	make clean
	make setup
	make aclient-execute

clean:
	rm client.cpp
	rm network.hpp
	rm client.out
	#rm t.txt
setup:
	cp ../client.cpp client.cpp
	cp ../network.hpp network.hpp	
