#include <stdio.h>
#include<stdlib.h>
#include<stdint.h>

int main() {

		uint32_t address = 12;
		uint32_t getAddr = address & 0x6;
		printf("Address: %d\n", getAddr);

}