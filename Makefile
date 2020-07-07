all:UDPServer UDPClient Stalk Test

Stalk:
	gcc -pthread -o stalk stalk.c

clean:
	rm -f stalk
