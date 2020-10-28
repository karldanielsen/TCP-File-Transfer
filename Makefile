main: server client
	g++ -g -o server server.o -lpthread 
	g++ -g -o client client.o
server:
	g++ -Wall -Wextra -Werror -ggdb -c server.cpp
client:
	g++ -Wall -Wextra -Werror -ggdb -c client.cpp
clean:
	rm -f server client server.o client.o
