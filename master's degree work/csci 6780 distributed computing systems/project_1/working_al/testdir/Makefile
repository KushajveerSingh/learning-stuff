compile:
	g++ client.cpp -o client.out
	g++ server.cpp -o sever.out

run-client:
	./client.out 127.0.0.1 8800

run-server:
	./server.out 8800

execute-client:
	g++ client.cpp -o client.out
	./client.out 127.0.0.1 8800

execute-server:
	g++ server.cpp -o server.out
	./server.out 8800

execute-server-a:
	g++ trial_server_AL.cpp -o trial_server_AL.out
	./trial_server_AL.out 8800

execute-tclient-a:
	g++ cta_client.cpp -o client.out
	./client.out 127.0.0.1 8800

fresh_and_clean:
	make clean
	make setup
	make execute-tclient-a

clean:
	rm cta_client.cpp
	rm client.out
	rm n2ta_network.hpp
	#rm t.txt
setup:
	cp ../cta_client.cpp cta_client.cpp
	cp ../n2ta_network.hpp n2ta_network.hpp	
