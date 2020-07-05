all:UDPServer UDPClient Stalk Test

UDPServer:
	gcc -std=c99 -o Server UDPServer.c

UDPClient:
	gcc -std=c99 -o Client UDPClient.c

Stalk:
	gcc -pthread -std=c99 -o stalk stalk.c

Test:
	gcc -std=c99 -o Test test.c

clean:
	rm -f UDPServer UDPClient Stalk Test
