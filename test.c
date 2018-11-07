#include <stdio.h>
#include<stdlib.h>

int main() {
	
	int pcA = 12;
	
	int a = -3;
	
	printf("a = %d\n", pcA+4 + (a<<2));
	
	return 1;
}

	int doneFetch = 0;
	while(1) { //While there are more instructions to fetch and the pipeline is not empty
		cycle_number++;
		if(pcAddr/4 == (numInstr-1)) {
			instShift(inst_stages);
			if(doneFetch == 0) {
				doneFetch = 1;
				instFetch(entry, inst_stages, pcAddr);
			}else {
				inst_stages[0].isValid = 0;
			}
			if(inst_stages[4].isValid == 0) {
				cycle_number--;
				break;
			}
			if(inst_stages[1].isValid == 1) {
				instExecute(inst_stages[1], &iR, &fR, memData);
			}
		} else {
			instFetch(entry, inst_stages, pcAddr); //Get the next instruction and put it in the first stage
			if(inst_stages[1].isValid == 1) {
				instExecute(inst_stages[1], &iR, &fR, memData);
			}
			pcAddr = entry[pcAddr/4].address + 4; //get address of next instruction
		}
		//printPipeline(cycle_number, inst_stages); //print the contents of the pipeline
	}