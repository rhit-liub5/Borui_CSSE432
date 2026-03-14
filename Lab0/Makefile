# To compile all three programs, list all executable names here.
all: paint palindrome list

# Creates the executable "paint" using the flag "-o".
paint: paint.o
	gcc paint.o -o paint -g -lm -Wall

# Creates the object file "paint.o" using the flag "-c".
paint.o: paint.c
	gcc paint.c -c -g -lm -Wall

# Creates the executable "list" using the flag "-o".
list: list.o
	gcc list.o -o list -g -lm -Wall

# Creates the object file "list.o" using the flag "-c".
list.o: list.c
	gcc list.c -c -g -lm -Wall

# Add the statements here to compile "palindrome.c" and create "palindrome". 




# Delete object files and executatbles to start afresh.
# Be sure to delete the "palindrome" executable also.
# Delete these files BEFORE committing your work. 
clean:
	rm -f *.o paint palindrome list
