#include <stdio.h>
#include "foo.h"
#include <unistd.h>
 
int main(void)
{
	printf("pid = %d\n", getpid());
    puts("This is a shared library test...");
	for(;;){
		foo();
		sleep(1);
	}
    return 0;
}
