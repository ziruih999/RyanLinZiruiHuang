all:Stalk 


Stalk:
	gcc -pthread -o s-talk s-talk.c list.c

clean:
	rm -f s-talk
