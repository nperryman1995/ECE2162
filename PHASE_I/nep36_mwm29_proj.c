/**************************************************************/
/* ECE 2162
 Noah Perryman
 Mitchell Moran
 compile instructions: 
 Execution Instructions
 ./
 ***************************************************************/

#include <stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include <ctype.h>
#include "struct_defs.h"

/*Definitions*/
#define INPUT_SIZE 1000 //amount of instructions that can be taken in by a file

int main(int argc, char **argv)
{
	unsigned int cycle_number = 1;
	uint32_t pcAddr = 0;
	struct int_Reg iR;
	struct float_Reg fR;
	initRegs(&iR, &fR); //initialize integer and floating point regsiters
	
	struct instruction entry[INPUT_SIZE]; //instruction struct for taking in new instructions
	struct instruction inst_stages[5]; //instruction structs for pipeline stages
	
	//Initialize stages to be NULL
	inst_stages[0].isValid = 0;
	inst_stages[1].isValid = 0;
	inst_stages[2].isValid = 0;
	inst_stages[3].isValid = 0;
	inst_stages[4].isValid = 0;
	
	//IS, EX, MEM, WB, COM
	//Create lookup tables for instruction start and end cycles at each stage
	//[i][0] = start cycle
	//[i][1] = end cycle
	uint32_t IS[INPUT_SIZE], EX[INPUT_SIZE], MEM[INPUT_SIZE], WB[INPUT_SIZE], COM[INPUT_SIZE]; 
	int i;
	for(i = 0; i < INPUT_SIZE; i++) {
		IS[i] = 0;
		EX[i] = 0;
		MEM[i] = 0;
		WB[i] = 0;
		COM[i] = 0;

	}
	
	char *fileName; //text file name
	static FILE *trace_fd;
	
	uint32_t intAdd_rs, intAdd_EX_Cycles, intAdd_MEM_cycles, intAdd_FUs; //text file input fields
	uint32_t FPAdd_rs, FPAdd_EX_Cycles, FPAdd_MEM_cycles, FPAdd_FUs;
	uint32_t FPMult_rs, FPMult_EX_Cycles, FPMult_MEM_cycles, FPMult_FUs;
	uint32_t ld_sd_rs, ld_sd_EX_Cycles, ld_sd_MEM_cycles, ld_sd_FUs;
	uint32_t ROB_Entries;
	uint32_t CDB_Buffer_Entries;
	int IntregInits, FPregInits;
	int MemInits;
	int numInstr;
	
	float memData[64]; //memory array is 64W
	for(i = 0; i < 64; i++) { //initialize memory
		memData[i] = 0;
	} //end if
	
	if (argc == 1) { //exit if no text file to read from
		printf("No input file. Stopping program\n");
		exit(0);
	} //end if
	
	fileName = argv[1]; //Retrieve file name and open it
	trace_fd = fopen(fileName, "rb");
	if (!trace_fd) { //if file cannot be open, exit program
		fprintf(stdout, "\ntrace file %s not opened.\n\n", fileName);
		exit(0);
	} //end if

	//read text file inputs
	fscanf(trace_fd, "%d %d %d %d", &intAdd_rs, &intAdd_EX_Cycles, &intAdd_MEM_cycles, &intAdd_FUs);
	fscanf(trace_fd, "%d %d %d %d", &FPAdd_rs, &FPAdd_EX_Cycles, &FPAdd_MEM_cycles, &FPAdd_FUs);
	fscanf(trace_fd, "%d %d %d %d", &FPMult_rs, &FPMult_EX_Cycles, &FPMult_MEM_cycles, &FPMult_FUs);
	fscanf(trace_fd, "%d %d %d %d", &ld_sd_rs, &ld_sd_EX_Cycles, &ld_sd_MEM_cycles, &ld_sd_FUs);
	fscanf(trace_fd, "%d %d", &ROB_Entries, &CDB_Buffer_Entries);
	fscanf(trace_fd, "%d %d %d", &IntregInits, &FPregInits, &MemInits);
	fscanf(trace_fd, "%d", &numInstr);
	
	//Test if inputs were read in correctly
	/*printf("%d %d %d %d\n", intAdd_rs, intAdd_EX_Cycles, intAdd_MEM_cycles, intAdd_FUs);
	printf("%d %d %d %d\n", FPAdd_rs, FPAdd_EX_Cycles, FPAdd_MEM_cycles, FPAdd_FUs);
	printf("%d %d %d %d\n", FPMult_rs, FPMult_EX_Cycles, FPMult_MEM_cycles, FPMult_FUs);
	printf("%d %d %d %d\n", ld_sd_rs, ld_sd_EX_Cycles, ld_sd_MEM_cycles, ld_sd_FUs);
	printf("%d %d\n", ROB_Entries, CDB_Buffer_Entries);
	printf("%d %d %d\n", IntregInits, FPregInits, MemInits);
	printf("%d\n", numInstr);*/
	
	char regtemp[50]; //temporaries for reading in values
	uint32_t regtempVal;
	float regtempValFP = 0;
	int regtempValInt = 0;
	char tempReg[50];
	for(i = 0; i < IntregInits; i++) { //read in integer register initializations
		fscanf(trace_fd, "%s %d", regtemp, &regtempVal);
		writeIntReg(&iR, regtemp, regtempVal);
	} //end for
    
	for(i = 0; i < FPregInits; i++) { //read in floating point register initializations
		fscanf(trace_fd, "%s %f", regtemp, &regtempValFP);
		writeFloatReg(&fR, regtemp, regtempValFP);
	} //end for

	char type;
	for(i = 0; i < MemInits; i++) { //read in memory initializations
		fscanf(trace_fd, "%s %d %s", regtemp, &regtempVal, tempReg);
		regtempValFP = atof(tempReg);
		storeMemory(memData, regtempVal/4, regtempValFP);
	} //end for
	
	//Test if inputs were read in correctly
	/*showIntReg(&iR);
	showFPReg(&fR);
	showMemory(memData);*/
	
	for(i = 0; i < numInstr; i++) { //Grab all of the instructions from the text
		fscanf(trace_fd, "%s", regtemp); //read in instruction
		assignInstr(&entry[i], regtemp); //decode instruction 
		switch(entry[i].type) {
			case ti_Ld: //Load case
				fscanf(trace_fd, "%*c %3[^,] %*c %d %*c %3[^)] %*c", entry[i].Fa, &entry[i].offset, entry[i].Ra);
				break;
			case ti_Sd: //Store case
				fscanf(trace_fd, "%*c %3[^,] %*c %d %*c %3[^)] %*c", entry[i].Fa, &entry[i].offset, entry[i].Ra);
				break;		
			case ti_Beq: //Branch if equal case
				fscanf(trace_fd, "%*c %3[^,] %*c %3[^,] %*c %d", entry[i].Rs, entry[i].Rt, &entry[i].offset);
				break;		
			case ti_Bne: //Branch if not equal case
				fscanf(trace_fd, "%*c %3[^,] %*c %3[^,] %*c %d", entry[i].Rs, entry[i].Rt, &entry[i].offset);
				break;		
			case ti_Add: //Integer addition case
				fscanf(trace_fd, "%*c %3[^,] %*c %3[^,] %*c %s", entry[i].Ra, entry[i].Rs, entry[i].Rt);
				break;		
			case ti_Addd: //Floating point addition case
				fscanf(trace_fd, "%*c %3[^,] %*c %3[^,] %*c %s", entry[i].Fa, entry[i].Fs, entry[i].Ft);
				break;		
			case ti_Addi: //Integer Immediate addition case
				fscanf(trace_fd, "%*c %3[^,] %*c %3[^,] %*c %d", entry[i].Rt, entry[i].Rs, &entry[i].offset);
				break;		
			case ti_Sub: //Integer subtraction
				fscanf(trace_fd, "%*c %3[^,] %*c %3[^,] %*c %s", entry[i].Ra, entry[i].Rs, entry[i].Rt);
				break;		
			case ti_Subd: //Floating point subtraction
				fscanf(trace_fd, "%*c %3[^,] %*c %3[^,] %*c %s", entry[i].Fa, entry[i].Fs, entry[i].Ft);
				break;		
			case ti_Multd: //Floating point multiplication
				fscanf(trace_fd, "%*c %3[^,] %*c %3[^,] %*c %s", entry[i].Fa, entry[i].Fs, entry[i].Ft);
				break;		
		} //end switch
		entry[i].address = i*4;
		entry[i].isValid = 1;
	} //end for
  
	fclose(trace_fd); //close trace file
  
	//Test Instruction decode and store functions
	/*for(i = 0; i < numInstr; i++) {
		printInstr(entry[i]);
		printf("\n");
	}*/
	
	//Test Get functions
	/*printf("R2 = %d\n", getIntReg(&iR, "R2"));
	printf("F2 = %.2f\n", getFloatReg(&fR, "F2"));
	struct memUnit temporaryMem = memRetr(memData, 8/4);
	if(temporaryMem.type == 0) {
		printf("Mem[8] = %d\n", temporaryMem.intMem);
	}else {
		printf("Mem[8] = %.2f\n", temporaryMem.floatMem);
	} //end if else*/
	
	/*---------Write code for algorithm below. Everything above is formatting---------*/
	//IS, EX, MEM, WB, COM
	int noFetch = 0;
	while(1) { //While there are more instructions to fetch and the pipeline is not empty
		instShift(inst_stages);
		if(noFetch == 1 && checkPipeline(inst_stages) == 0) {
			break;
		}
		if((pcAddr/4 == (numInstr-1))) {
			if(noFetch == 0) {
				noFetch = 1;
				instFetch(entry, inst_stages, pcAddr); //Get the next instruction and put it in the first stage
			}else {
				inst_stages[0].isValid = 0;
			}
		}else {
			instFetch(entry, inst_stages, pcAddr); //Get the next instruction and put it in the first stage
		}
		if(inst_stages[1].isValid == 1) { //if an instruction needs to be dealt with in the EX stage
			exExecution(inst_stages[1], &iR, &fR, memData);
		}
		if(inst_stages[3].isValid == 1) { //If there is a load/store instruciton
			memExecution(inst_stages[3], &iR, &fR, memData);
		}

		
		if(inst_stages[0].isValid == 1) {
			IS[(inst_stages[0].address)/4] = cycle_number;
		}
		if(inst_stages[1].isValid == 1) {
			EX[(inst_stages[1].address)/4] = cycle_number;
		}
		if(inst_stages[2].isValid == 1) {
			MEM[(inst_stages[2].address)/4] = cycle_number;
		}		
		if(inst_stages[3].isValid == 1) {
			WB[(inst_stages[3].address)/4] = cycle_number;
		}	
		if(inst_stages[4].isValid == 1) {
			COM[(inst_stages[4].address)/4] = cycle_number;
		}
		pcAddr = pcAddr + 4;
		//printPipeline(cycle_number, inst_stages); //print the contents of the pipeline
		cycle_number++;
	}
  
	printResults(entry, IS, EX, MEM, WB, COM, numInstr);
	showIntReg(&iR);
	showFPReg(&fR);
	showMemory(memData);
  
    /*---------Write code for algorithm above. Everything below is formatting---------*/
	
	return 1;
  
} //end main