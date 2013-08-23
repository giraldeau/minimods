all: showstack

clean:
	rm showstack

showstack: showstack.c
	$(CC) -O0 -fno-omit-frame-pointer -g -o $@ $<

.PHONY = all clean