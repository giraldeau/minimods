#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_STACK 100

volatile int x;
char buf[4096];

void show_stack(unsigned long stack) {
	int i;
	unsigned long addr;
	printf("%-18s %-18s\n", "address", "value");
	for (i = 0; i < MAX_STACK; i++) {
		addr = stack + i * sizeof(unsigned long);
		printf("0x%016lx 0x%016lx\n", addr, *(unsigned long *)addr);
	}
}

void d() {
	register unsigned long rsp asm("rsp");
	int fd = open("/proc/minimod", O_WRONLY);
	int y = x++;
	show_stack(rsp);
	write(fd, &y, sizeof(y));
	fd = open("/proc/self/maps", O_RDONLY);
	read(fd, buf, 4096);
	printf("%s\n", buf);
}

void c() { x++; d(); }
void b() { x++; c(); }
void a() { x++; b(); }

int main(int argc, char **argv) {
	a();
	return 0;
}
