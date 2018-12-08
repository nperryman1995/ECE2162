/**************************************************************/
/* ECE 2162
 * Noah Perryman
 * Mitchell Moran
 * Project Phase II
*/
/***************************************************************/

#include <stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include <ctype.h>
#include "struct_defs.h"

/*Definitions*/
#define INPUT_SIZE 100 //amount of instructions that can be taken in by a file
#define MAX_ROB 1000 //maximum size of ROB Table

int main(int argc, char **argv)
{
	unsigned int cycle_number = 1;
	uint32_t pcAddr = 0;
	struct int_Reg iR;
	struct float_Reg fR;
	initRegs(&iR, &fR); //initialize integer and floating point regsiters
	
	struct instruction entry[INPUT_SIZE]; //instruction struct for taking in new instructions. Instruction buffer
	struct CDB_buffer iBuffer[INPUT_SIZE]; //cdb buffer entries. capped at input size, but will only utilize a certain amount of them depending on the input parameter
	struct CDB_buffer fABuffer[INPUT_SIZE];
	struct CDB_buffer fMBuffer[INPUT_SIZE];
	struct RS_entry iRS[INPUT_SIZE]; //integer adder reservation station
	struct RS_entry fARS[INPUT_SIZE]; //fp adder reservation station
	struct RS_entry fMRS[INPUT_SIZE]; //fp multpilier reservation station
	struct ROB_entry reOrder[MAX_ROB]; //ReOrder Buffer
	struct RAT_entry rat_Table[MAX_ROB]; //RAT Table
	
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
		iRS[i].isBusy = 2;
		fARS[i].isBusy = 2;
		fMRS[i].isBusy = 2;
		iBuffer[i].isValid = 1;
		fABuffer[i].isValid = 1;
		fMBuffer[i].isValid = 1;
	}
	
	for(i = 0; i < MAX_ROB; i++) {
		rat_Table[i].rType = 2;
		reOrder[i].type = 11;
		if(i==0) {
			reOrder[i].head = 1;
		}else {
			reOrder[i].head = 0;
		}
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
	
	//Initialize reservation stations
	for(i = 0; i < intAdd_rs; i++) {
		iRS[i].isBusy = 0;
	}
	
	for(i = 0; i < FPAdd_rs; i++) {
		fARS[i].isBusy = 0;
	}
	
	for(i = 0; i < FPMult_rs; i++) {
		fMRS[i].isBusy = 0;
	}
	
	//Initialize cdb buffer
	for(i = 0; i < CDB_Buffer_Entries; i++) {
		iBuffer[i].isValid = 0;
		fABuffer[i].isValid = 0;
		fMBuffer[i].isValid = 0;
	}
	
	//initialize ROB tables
	for(i = 0; i < ROB_Entries; i++) {
		reOrder[i].type = 10;
	}
	
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
	
	//IS, EX, MEM, WB, COM
	unsigned char noFetch = 0;
	unsigned char iRS_Done = 0;
	unsigned char fARS_Done = 0;
	unsigned char fMRS_Done = 0;
	unsigned char rob_Done = 0;
	uint32_t pcAddrQueue = 0;
	struct instruction forResStat;
	unsigned char rsType; //0 = int RS, 1 = fP Add RS, 2 = fp Mult RS
	unsigned char rsStall = 0; //0 = rs entry available, 1 = no rs entry available (stall)
	unsigned char buf_choice = 3;
	while(1) { //While there are more instructions to fetch and the pipeline is not empty	
		//Reset Fetch, Reservation Station, and ROB checkers each cycle
		noFetch = 0;
		iRS_Done = 0;
		fARS_Done = 0;
		fMRS_Done = 0;
		rob_Done = 0;
		if(pcAddrQueue = entry[numInstr - 1].address) { //No more instructions in the instruction queue
			noFetch = 1; 
		}
		
		buf_choice = cdb_Execute(iBuffer, fABuffer, fMBuffer);
		switch(buf_choice) {
			case 0:
				broadCastCDBVal(iBuffer, iRS, fARS, fMRS, reOrder, INPUT_SIZE);
				clearBufEntry(iBuffer);
				break;
			case 1:
				broadCastCDBVal(fABuffer, iRS, fARS, fMRS, reOrder, INPUT_SIZE));
				clearBufEntry(fABuffer);
				break;
			case 2:
				broadCastCDBVal(fMBuffer, iRS, fARS, fMRS, reOrder, INPUT_SIZE));
				clearBufEntry(fMBuffer);
				break;
		}
		
		updateROB(reOrder, ROB_Entries, rat_Table, &iR, &fR);

		//Decode the Reservation Station to Send the instruction to if there is no stall
		if(rsStall == 0) {
			forResStat = entry[pcAddrQueue];
			switch(forResStat.type) {
				case ti_Ld:
					break;
				case ti_Sd:
					break;		
				case ti_Beq:
					break;		
				case ti_Bne:
					break;		
				case ti_Add:
					rsType = 0;
					break;		
				case ti_Addd:
					rsType = 1;
					break;		
				case ti_Addi:
					rsType = 0;
					break;		
				case ti_Sub:
					rsType = 0;
					break;		
				case ti_Subd:
					rsType = 1;
					break;		
				case ti_Multd:
					rsType = 2;
					break;	
			}
		}
		
		if(rsType == 0) {
			for(i = 0; i < intAdd_rs; i++) {
				if(iRS[i].isBusy == 0) {
					rsStall = 0;
					iRSFill(iRS, reOrder, &iR, rat_Table, i, &forResStat, ROB_Entries, MAX_ROB);
					iRS[i].cyclesLeft = intAdd_EX_Cycles + 1;
					break;
				}
				rsStall = 1;
			}
		}else if(rsType == 1) {
			for(i = 0; i < FPAdd_rs; i++) {
				if(fARS[i].isBusy == 0) {
					rsStall = 0;
					fARSFill(fARS, reOrder, &fR, rat_Table, i, &forResStat, ROB_Entries, MAX_ROB);
					fARS[i].cyclesLeft = FPAdd_EX_Cycles + 1;
					break;
				}
				rsStall = 1;				
			}			
		}else{
			for(i = 0; i < FPMult_rs; i++) {
				if(iRS[i].isBusy == 0) {
					rsStall = 0;
					fMRSFill(fMRS, reOrder, &fR, rat_Table, i, &forResStat, ROB_Entries, MAX_ROB);
					fMRS[i].cyclesLeft = FPMult_EX_Cycles+1;
					break;
				}
				rsStall = 1;				
			}				
		}
		
		RS_Exectute(iRS, fARS, fMRS, INPUT_SIZE, iBuffer, fABuffer, fMBuffer, cycle_number);
		
		//Check if Reservation Stations are empty
		for(i = 0; i < intAdd_rs; i++) {
			if(iRS[i].isBusy == 0) {
				iRS_Done = 1;
				break;
			}
		}
	
		for(i = 0; i < FPAdd_rs; i++) {
			if(fARS[i].isBusy == 0) {
				fARS_Done = 1;
				break;
			}
		}
	
		for(i = 0; i < FPMult_rs; i++) {
			if(fMRS[i].isBusy == 0) {
				break;
				fMRS_Done = 1;
			}
		}

		//check ROB table
		for(i = 0; i < ROB_Entries; i++) {
			if(reOrder[i].type == 10) {
				rob_Done = 1;
				break;
			}
		}
		
		//If there is nothing more to fetch from the instruction queue, all of the reservation stations are empty,
		//and all of the ROB entries are empty, then there is nothing left to do
		if(noFetch == 1 && iRS_Done == 1 && fARS_Done == 1 && fMRS_Done == 1 && robDone == 1) {
			break;
		}
		
		cycle_number++;
	}
  
	printResults(entry, IS, EX, MEM, WB, COM, numInstr);
	showIntReg(&iR);
	showFPReg(&fR);
	showMemory(memData);
	
	return 1;
  
} //end main