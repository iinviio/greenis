CC = gcc

%o: %.c ./greenis.h
	$(CC) -c -o $@ $<

greenis: main.o parser.o database.o
	$(CC) -o greenis $^

.PHONY: clean

clean:
	rm ./*.o