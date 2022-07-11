vm: main.c
	gcc $< -pthread -lm -o $@  

.PHONY: clean

clean:
	rm vm