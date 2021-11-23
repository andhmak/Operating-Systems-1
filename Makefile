all:
	gcc -o parent parent.c -lpthread
	gcc -o child child.c -lpthread
clean:
	rm -f parent child parent.o child.o