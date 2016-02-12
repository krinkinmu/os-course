deafult: rb

rb: main_rb.o rb.o
	gcc $^ -o $@

%.o: %.c
	gcc -Ofast $< -c -o $@

.PHONY: clean
clean:
	rm -rf *.o rb
