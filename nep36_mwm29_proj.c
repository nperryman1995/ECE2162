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

/*Function declarations*/
void initRegs(struct int_Reg *iR, struct float_Reg *fR);
void writeIntReg(struct int_Reg *iR, char *reg, uint32_t val);
void writeFloatReg(struct float_Reg *fR, char *reg, float val);
void storeMemory(struct memUnit *memData, int regtempVal, float regtempValFP, int regtempValInt, char type);
char detType(char *type);
uint32_t getIntReg(struct int_Reg *iR, char *reg);
float getFloatReg(struct float_Reg *fR, char *reg);
struct memUnit memRetr(struct memUnit *memData, int memNum);
int regLookup(char *reg);
void assignInstr(struct instruction *instr, char *temp);
void printInstr(struct instruction instr);
void showIntReg(struct int_Reg *iR);
void showFPReg(struct float_Reg *fR);
void showMemory(struct memUnit *memData);
void instShift(struct instruction *stages);
void instFetch(struct instruction *isntr, struct instruction *stages, uint32_t pc_Addr);
void printPipeline(uint32_t cycle_number, struct instruction *stages);

int main(int argc, char **argv)
{
	unsigned int cycle_number = 0;
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
	
	struct memUnit memData[64]; //memory array is 64W
	int i;
	for(i = 0; i < 64; i++) { //initialize memory
		memData[i].intMem = 0;
		memData[i].floatMem = 0;
		memData[i].type = 0;
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
		type = detType(tempReg);
		if(type == 0) {
			regtempValInt = atoi(tempReg);
		}else {
			regtempValFP = atof(tempReg);
		} //end if else
		storeMemory(memData, regtempVal/4, regtempValFP, regtempValInt, type);
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
			printPipeline(cycle_number, inst_stages); //print the contents of the pipeline
		} else {
			instFetch(entry, inst_stages, pcAddr); //Get the next instruction and put it in the first stage
			pcAddr = entry[pcAddr/4].address + 4; //get address of next instruction
			printPipeline(cycle_number, inst_stages); //print the contents of the pipeline
		}
	}
  
    /*---------Write code for algorithm above. Everything below is formatting---------*/
	
	return 1;
  
} //end main

/*Initialize Integer and floating point register files*/
void initRegs(struct int_Reg *iR, struct float_Reg *fR) {
	int i;
	for(i = 0; i < 32; i++) {
		iR->R_num[i] = 0;
		fR->F_num[i] = 0;
		if(i==0) { //R0 = 0, all others can be written to
			iR->canWrite[i] = 0;
		}else {
			iR->canWrite[i] = 1;
		} //end if else
	} //end for
} //end initRegs

/*Write to the specified integer register*/
void writeIntReg(struct int_Reg *iR, char *reg, uint32_t val) {
	int rN = regLookup(reg); //decode register number
	if(iR->canWrite[rN]==1) { //if register number is not restricted, write to it
		iR->R_num[rN] = val;
	} //end if
}// end writeIntReg

/*Write to the specified floating point register*/
void writeFloatReg(struct float_Reg *fR, char *reg, float val) {
	int rN = regLookup(reg); //decode register number
	fR->F_num[rN] = val;
} //end writeFloatReg

/*Write to the specified memory location*/
void storeMemory(struct memUnit *memData, int regtempVal, float regtempValFP, int regtempValInt, char type) {
	if(type == 0) { //if the value to be stored is an integer
		memData[regtempVal].type = 0;
		memData[regtempVal].intMem = regtempValInt;
	}else { //if the value to be stored is a floating point
		memData[regtempVal].type = 1;
		memData[regtempVal].floatMem = regtempValFP;
	} //end if else
} //end storeMemory

/*Determine if the number is a float or an int value*/
char detType(char *type) {
	char *p = type;
	char ret = 0;
	while (*p) { // While there are more characters to process
		if (*p=='.') {
			ret = 1;
			break;
		} else { // Otherwise, move on to the next character.
			p++;
		} //end if else
	} //end while
	return ret;
} //end detType

/*Get the specified integer register*/
uint32_t getIntReg(struct int_Reg *iR, char *reg) {
	int rN = regLookup(reg); //decode register number
	return iR->R_num[rN];
} //end getIntReg

/*Get the specified floating point register*/
float getFloatReg(struct float_Reg *fR, char *reg) {
	int rN = regLookup(reg); //decode register number
	return fR->F_num[rN];
} //end getFloatReg

/*Get the specified value from memory*/
struct memUnit memRetr(struct memUnit *memData, int memNum) {
	struct memUnit ret;
	if(memData[memNum].type == 0) {
		ret.type = 0;
		ret.intMem = memData[memNum].intMem;
		ret.floatMem = 0;
	}else {
		ret.type = 1;
		ret.intMem = 0;
		ret.floatMem = memData[memNum].floatMem;
	} //end if else
	return ret;
} //end memRetr

/*Given the register string, find the register number*/
int regLookup(char *reg) {
	char *p = reg;
	int ret = 0;
	while (*p) { // While there are more characters to process
		if ( isdigit(*p) || ( (*p=='-'||*p=='+') && isdigit(*(p+1)) )) {
			ret = strtol(p, &p, 10); // Read number
		} else { // Otherwise, move on to the next character.
			p++;
		} //end if else
	} //end while
	return ret;
} //end regLookup

/*Assign the type field of the instruction*/
void assignInstr(struct instruction *instr, char *temp) {
	if(!strcmp(temp, "Ld")){
		instr->type = ti_Ld;
	}else if(!strcmp(temp,"Sd")) {
		instr->type = ti_Sd;
	}else if(!strcmp(temp,"Beq")) {
		instr->type = ti_Beq;
	}else if(!strcmp(temp,"Bne")) {
		instr->type = ti_Bne;
	}else if(!strcmp(temp,"Add")) {
		instr->type = ti_Add;
	}else if(!strcmp(temp,"Add.d")) {
		instr->type = ti_Addd;
	}else if(!strcmp(temp,"Addi")) {
		instr->type = ti_Addi;
	}else if(!strcmp(temp,"Sub")) {
		instr->type = ti_Sub;
	}else if(!strcmp(temp,"Sub.d")) {
		instr->type = ti_Subd;
	}else {
		instr->type = ti_Multd;
	} //end if else
} //end assignInstr

/*print the instruction*/
void printInstr(struct instruction instr) {
	if(instr.isValid == 0) {
		printf("No Instruction\n");
		return;
	}
	switch(instr.type) {
		case ti_Ld:
			printf("Ld %s, %d(%s)",instr.Fa, instr.offset, instr.Ra);
			break;
		case ti_Sd:
			printf("Sd %s, %d(%s)",instr.Fa, instr.offset, instr.Ra);
			break;		
		case ti_Beq:
			printf("Beq %s, %s, %d",instr.Rs, instr.Rt, instr.offset);
			break;		
		case ti_Bne:
			printf("Bne %s, %s, %d",instr.Rs, instr.Rt, instr.offset);
			break;		
		case ti_Add:
			printf("Add %s, %s, %s", instr.Ra, instr.Rs, instr.Rt);
			break;		
		case ti_Addd:
			printf("Add.d %s, %s, %s", instr.Fa, instr.Fs, instr.Ft);
			break;		
		case ti_Addi:
			printf("Addi %s, %s, %d", instr.Rt, instr.Rs, instr.offset);
			break;		
		case ti_Sub:
			printf("Sub %s, %s, %s", instr.Ra, instr.Rs, instr.Rt);
			break;		
		case ti_Subd:
			printf("Sub.d %s, %s, %s", instr.Fa, instr.Fs, instr.Ft);
			break;		
		case ti_Multd:
			printf("Mult.d %s, %s, %s", instr.Fa, instr.Fs, instr.Ft);
			break;		
	} //end switch
	printf("\n");
}//end printInstr

/*Show contents of integer register files if non-zero*/
void showIntReg(struct int_Reg *iR) {
	int i;
	for(i = 0; i < 32; i++) {
		if(iR->R_num[i] != 0) {	
			printf("R%d = %d\n",i, iR->R_num[i]);
		} //end if
	} //end for
} //end showIntReg

/*Show contents of floating point register files if non-zero*/
void showFPReg(struct float_Reg *fR) {
	int i;
	for(i = 0; i < 32; i++) {
		if(fR->F_num[i] !=0) {
			printf("F%d = %.2f\n",i, fR->F_num[i]);
		} //end if
	} //end for	
} //end showFPReg

/*show contents of memory if non-zero*/
void showMemory(struct memUnit *memData) {
	int i;
	for(i = 0; i < 64; i++) {
		if(memData[i].type == 0) {
			if(memData[i].intMem != 0) {
				printf("Mem[%d] = %d\n", i*4, memData[i].intMem);
			} //end if
		}else {
			if(memData[i].floatMem != 0) {
				printf("Mem[%d] = %.2f\n", i*4, memData[i].floatMem);
			} //end if
		} //end if else
	}// end for
} //end showMemory

/*Shift the pipeline*/
void instShift(struct instruction *stages) {
	int i;
	for(i = 4; i > 0; i--) {
		stages[i] = stages[i-1];
	} //end for
} //end instShift

/*Get the next instruction and put it in the first stage*/
void instFetch(struct instruction *instr, struct instruction *stages, uint32_t pc_Addr) {
	instShift(stages);
	stages[0] = instr[pc_Addr/4];
} //end instFetch

/*Print the contents of the pipeline*/
void printPipeline(uint32_t cycle_number, struct instruction *stages) {
	printf("------------------------------------------------\n");
	printf("Cycle Number = %d\n", cycle_number);
	printf("Issue: ");
	printInstr(stages[0]);
	printf("Ex: ");
	printInstr(stages[1]);
	printf("Mem: ");
	printInstr(stages[2]);
	printf("WB: ");
	printInstr(stages[3]);
	printf("Commit: ");
	printInstr(stages[4]);
	printf("------------------------------------------------\n");
} //end printPipeline