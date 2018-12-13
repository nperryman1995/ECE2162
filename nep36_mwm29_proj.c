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

int main(int argc, char **argv)
{
	unsigned int cycle_number = 1;
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
	struct RAT_entry int_rat_Table[MAX_ROB]; //RAT Tables
	struct RAT_entry float_rat_Table[MAX_ROB];
	struct LSQ_entry lsq_Table[INPUT_SIZE]; //load/store queue
	struct recover_Program RIP[50]; //For branch recovery
	struct BTB btbBuffer[8]; //branch target buffer
	struct branchHistory bHist[INPUT_SIZE]; //History of branch instruction
	
	//IS, EX, MEM, WB, COM
	//Create lookup tables for instruction start and end cycles at each stage
	uint32_t IS[INPUT_SIZE], EX[INPUT_SIZE], MEM[INPUT_SIZE], WB[INPUT_SIZE], COM[INPUT_SIZE]; 
	uint32_t reset[INPUT_SIZE];
	int i;
	for(i = 0; i < INPUT_SIZE; i++) { //Initialize Entries
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
		lsq_Table[i].isBusy = 2;
		if(i == 0) {
			lsq_Table[i].isHead = 1;
		}else {
			lsq_Table[i].isHead = 0;
		}
		reset[i] = -1;
		bHist[i].isValid = 0;
	}
	
	for(i = 0; i < 8; i++) { //Initialize Entries
		btbBuffer[i].BTB_array.isValid = 1;
		btbBuffer[i].BTB_array.type = 10;
		btbBuffer[i].nextAddr = 0;
	}
	
	for(i = 0; i < MAX_ROB; i++) { //Initialize Entries
		int_rat_Table[i].rType = 2;
		float_rat_Table[i].rType = 2;
		int_rat_Table[i].iOrf = 0;
		float_rat_Table[i].iOrf = 1;
		reOrder[i].type = 11;
		if(i==0) {
			reOrder[i].isHead = 1;
		}else {
			reOrder[i].isHead = 0;
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
			reOrder[i].address = 0;
			reOrder[i].type = 10;
			reOrder[i].destReg[0] = 0;
			reOrder[i].destReg[1] = 0;
			reOrder[i].destReg[2] = 0;
			reOrder[i].destReg[3] = 0;
			reOrder[i].intVal = 0;
			reOrder[i].floatVal = 0;
			reOrder[i].finOp = 0;
			reOrder[i].isHead = 0;
			if(i == 0) {
				reOrder[i].isHead = 1;
			}
		
	}
	
	//initialize load/store queue
	for(i = 0; i < ld_sd_rs; i++) {
		lsq_Table[i].isBusy = 0;
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
	
	//Flags to check if certain fields are empty
	unsigned char noFetch = 0;
	unsigned char iRS_Done = 0;
	unsigned char fARS_Done = 0;
	unsigned char fMRS_Done = 0;
	unsigned char rob_Done = 0;
	unsigned char cdbDone = 0;
	unsigned char lsqDone = 0;
	uint32_t pcAddrQueue = 0;
	uint32_t predAddr = 0;
	struct instruction forResStat;
	unsigned char rsType; //0 = int RS, 1 = fP Add RS, 2 = fp Mult RS
	unsigned char rsStall = 0; //0 = rs entry available, 1 = no rs entry available (stall)
	unsigned char buf_choice = 3;
	while(1) { //While there are more instructions to fetch and the pipeline is not empty	
		//Reset flags at the beginning
		iRS_Done = 1;
		fARS_Done = 1;
		fMRS_Done = 1;
		rob_Done = 1;
		cdbDone = 1;
		lsqDone = 1;
		
		updateROB(lsq_Table, ld_sd_rs, ld_sd_MEM_cycles, MEM, reOrder, ROB_Entries, int_rat_Table, float_rat_Table, &iR, &fR, WB, COM, cycle_number, memData); //commit ROB entry 
		
		//Decode the Reservation Station to Send the instruction to if there is no stall
		if(rsStall == 0 && noFetch == 0) { //If an instruction is not waiting for a reservation station and there are more instructions to fetch
			forResStat = entry[pcAddrQueue/4]; //fetch instruction and decode
			if(pcAddrQueue == entry[numInstr - 1].address) { //If at last instruction, raise the flag to not fetch any more instructions unless disproven later
				noFetch = 1;
			}
			uint32_t getBTB = pcAddrQueue & 0x6; //get index for BTB
			int j;
			if(forResStat.type == ti_Beq || forResStat.type == ti_Bne) { //find a free entry in the branch history table and update it if a branch
				for(j = 0; j < INPUT_SIZE; j++) {
					if(bHist[j].isValid == 0) {
						bHist[j].isValid = 1;
						bHist[j].br = forResStat;
						bHist[j].pcPlusFour = pcAddrQueue + 4;
						bHist[j].takenAddr = -1;
						break;
					}
				}
			}
			if(btbBuffer[getBTB].BTB_array.address == pcAddrQueue && btbBuffer[getBTB].BTB_array.type != 10) { //if the BTB contains a prediction for this address, take it, otherwise do not take it
				if(forResStat.type == ti_Beq || forResStat.type == ti_Bne) {
					pcAddrQueue = btbBuffer[getBTB].nextAddr;
					bHist[j].predAddr = pcAddrQueue;
					bHist[j].isTaken = 1;
				}
			}else{
				if(pcAddrQueue != entry[numInstr - 1].address) {
					pcAddrQueue = pcAddrQueue + 4;
				}
				if(forResStat.type == ti_Beq || forResStat.type == ti_Bne) {
					bHist[j].predAddr = pcAddrQueue;
					bHist[j].isTaken = 0;
				}
			}
			switch(forResStat.type) { //decode instruction type
				case ti_Ld:
					rsType = 3;
					break;
				case ti_Sd:
					rsType = 3;
					break;		
				case ti_Beq:
					rsType = 4;
					break;		
				case ti_Bne:
					rsType = 4;
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

		if(rsType == 0) { //Put instruction into integer add reservation station if there is a spot available, otherwise stall and wait until there is one available
			for(i = 0; i < intAdd_rs; i++) {
				if(iRS[i].isBusy == 0) {
					rsStall = 0;
					iRSFill(iRS, reOrder, &iR, int_rat_Table, i, &forResStat, ROB_Entries, MAX_ROB);
					iRS[i].cyclesLeft = intAdd_EX_Cycles;
					IS[iRS[i].address/4] = cycle_number;
					if(noFetch == 1) {
						rsType = 5;
					}
					break;
				}
				rsStall = 1;
			}
		}else if(rsType == 1) { //Put instruction into floating point add reservation station if there is a spot available, otherwise stall and wait until there is one available
			for(i = 0; i < FPAdd_rs; i++) {
				if(fARS[i].isBusy == 0) {
					rsStall = 0;
					fARSFill(fARS, reOrder, &fR, float_rat_Table, i, &forResStat, ROB_Entries, MAX_ROB);
					fARS[i].cyclesLeft = FPAdd_EX_Cycles;
					IS[fARS[i].address/4] = cycle_number;
					if(noFetch == 1) {
						rsType = 5;
					}
					break;
				}
				rsStall = 1;				
			}			
		}else if(rsType == 2){ //Put instruction into floating point multiplication reservation station if there is a spot available, otherwise stall and wait until there is one available
			for(i = 0; i < FPMult_rs; i++) {
				if(fMRS[i].isBusy == 0) {
					rsStall = 0;
					fMRSFill(fMRS, reOrder, &fR, float_rat_Table, i, &forResStat, ROB_Entries, MAX_ROB);
					fMRS[i].cyclesLeft = FPMult_EX_Cycles;
					IS[fMRS[i].address/4] = cycle_number;
					if(noFetch == 1) {
						rsType = 5;
					}
					break;
				}
				rsStall = 1;				
			}				
		}else if(rsType == 3) { //Put a load/store in the load/store queue if there is a spot available, otherwise stall and wait until there is one available
			for(i = 0; i < ld_sd_rs; i++) {
				if(lsq_Table[i].isBusy == 0) {
					rsStall = 0;
					lsqFill(lsq_Table, reOrder, &iR, &fR, int_rat_Table, float_rat_Table, i, &forResStat, ROB_Entries, MAX_ROB);
					lsq_Table[i].ex_cyclesLeft = ld_sd_EX_Cycles;
					lsq_Table[i].mem_cyclesLeft = ld_sd_MEM_cycles;
					IS[lsq_Table[i].address/4] = cycle_number;
					if(noFetch == 1) {
						rsType = 5;
					}
					break;
				}
				rsStall = 1;
			}
		}else if(rsType == 4) { //Put a branch instruction into the integer add reservation station if there is a spot available, otherwise stall and wait until there is one available
			for(i = 0; i < intAdd_rs; i++) {
				if(iRS[i].isBusy == 0) {
					rsStall = 0;
					iRSBranchFill(iRS, &iR, i, &forResStat, int_rat_Table, reOrder);
					iRS[i].cyclesLeft = intAdd_EX_Cycles;
					IS[iRS[i].address/4] = cycle_number;
					if(noFetch == 1) {
						rsType = 5;
					}
					copyProgramCDB(&RIP[iRS[i].address/4], iBuffer, fABuffer, fMBuffer); //save program state if a branch instruciton
					copyProgramRS(&RIP[iRS[i].address/4], iRS, fARS, fMRS);
					copyProgramLSQ(&RIP[iRS[i].address/4], lsq_Table);
					copyProgramRATROB(&RIP[iRS[i].address/4], int_rat_Table, float_rat_Table, reOrder);
					copyProgramREG(&RIP[iRS[i].address/4], iR, fR);
					copyProgramINT(&RIP[iRS[i].address/4], cycle_number, pcAddrQueue, noFetch);
					copyProgramMEM(&RIP[iRS[i].address/4], memData);
					copyProgramTAB(&RIP[iRS[i].address/4], IS, EX, MEM, WB, COM);
					break;
				}
				rsStall = 1;
			}			
		}
		
		//Execute the instructions in the reservation stations if they are ready
		predAddr = RS_Execute(bHist, memData, MEM, ld_sd_MEM_cycles, ld_sd_EX_Cycles, lsq_Table, ld_sd_rs, reOrder, ROB_Entries, int_rat_Table, float_rat_Table, iRS, fARS, fMRS, INPUT_SIZE, iBuffer, fABuffer, fMBuffer, cycle_number, intAdd_EX_Cycles, FPAdd_EX_Cycles, FPMult_EX_Cycles, EX);
		if(predAddr != -1) { //if branch instruct just executed
			//printf("Pred Address: %d\n", predAddr);
			if(reset[predAddr] == 1) {
				reset[predAddr] = 0;
			}else{
				int j;
				for(j = 0; j < INPUT_SIZE; j++) { //look up entry in the branch history table
					if(bHist[j].isValid == 1) {
						if(bHist[j].takenAddr == predAddr) {
							break;
						}
					}
				}
				uint32_t getBTB = bHist[j].br.address & 0x6;
				if(bHist[j].isTaken == 1) {
					if(predAddr != bHist[j].predAddr) { //branch not taken, predicted taken
						reset[predAddr] = 1;
						btbBuffer[getBTB].BTB_array.type = 10;
						resetProgramCDB(&RIP[bHist[j].br.address/4], iBuffer, fABuffer, fMBuffer); //reset the program values if there was a misprediction
						resetProgramRS(&RIP[bHist[j].br.address/4], iRS, fARS, fMRS);
						resetProgramLSQ(&RIP[bHist[j].br.address/4], lsq_Table);
						resetProgramRATROB(&RIP[bHist[j].br.address/4], int_rat_Table, float_rat_Table, reOrder);
						resetProgramREG(&RIP[bHist[j].br.address/4], &iR, &fR);
						resetProgramINT(&RIP[bHist[j].br.address/4], &cycle_number, &pcAddrQueue, &noFetch);
						resetProgramMEM(&RIP[bHist[j].br.address/4], memData);
						resetProgramTAB(&RIP[bHist[j].br.address/4], IS, EX, MEM, WB, COM);
						}
				}else {
					if(predAddr != bHist[j].pcPlusFour) { //branch taken, predicted not taken
						reset[predAddr] = 1;
						btbBuffer[getBTB].nextAddr = predAddr;
						btbBuffer[getBTB].BTB_array = bHist[j].br;
						resetProgramCDB(&RIP[bHist[j].br.address/4], iBuffer, fABuffer, fMBuffer); //reset the program values if there was a misprediction
						resetProgramRS(&RIP[bHist[j].br.address/4], iRS, fARS, fMRS);
						resetProgramLSQ(&RIP[bHist[j].br.address/4], lsq_Table);
						resetProgramRATROB(&RIP[bHist[j].br.address/4], int_rat_Table, float_rat_Table, reOrder);
						resetProgramREG(&RIP[bHist[j].br.address/4], &iR, &fR);
						resetProgramINT(&RIP[bHist[j].br.address/4], &cycle_number, &pcAddrQueue, &noFetch);
						resetProgramMEM(&RIP[bHist[j].br.address/4], memData);
						resetProgramTAB(&RIP[bHist[j].br.address/4], IS, EX, MEM, WB, COM);
						pcAddrQueue = predAddr;
						}
				}
				bHist[j].isValid = 0;
				bHist[j].takenAddr = -1;
			}
		}
		
		//Some branch instructions cause some issue with the integer reservation station (and possibly some other unknown FUs)
		//causing the entry to wait on a value in the reorder buffer that doesn't exist (i.e., the instruction waits for 
		//a reorder buffer value that will never be computed as there is no entry in that particular spot in the reorder buffer)
		//If this happens this for loop just updates the values with values in the ARF
		for(i = 0; i < INPUT_SIZE; i++) {
			if(iRS[i].isBusy == 1) {
				if(iRS[i].tag1 != -1) {
					if(reOrder[iRS[i].tag1].type == 10) {
						iRS[i].tag1 = -1;
						iRS[i].iVal1 = iR.R_num[regLookup(entry[iRS[i].address/4].Rs)];
					}
				}
				if(iRS[i].tag2 != -1) {
					if(reOrder[iRS[i].tag2].type == 10) {
						iRS[i].tag2 = -1;
						switch(entry[iRS[i].address/4].type) {
							case ti_Add:
								iRS[i].iVal2 = iR.R_num[regLookup(entry[iRS[i].address/4].Rs)];
								break;
							case ti_Sub:
								iRS[i].iVal2 = iR.R_num[regLookup(entry[iRS[i].address/4].Rs)];
								break;
							case ti_Addi:
								iRS[i].iVal2 = entry[iRS[i].address/4].offset;
								break;
						}
					}
				}
			}
		}
		
		buf_choice = cdb_Execute(iBuffer, fABuffer, fMBuffer); //choose which cdb buffer will broadcast on the cdb next based on arrival cycle, then broadcast the value on the cdb
		switch(buf_choice) {
			case 0:
				broadCastCDBVal(iBuffer, iRS, fARS, fMRS, reOrder, INPUT_SIZE, lsq_Table, ld_sd_rs);
				clearBufEntry(iBuffer, INPUT_SIZE);
				break;
			case 1:
				broadCastCDBVal(fABuffer, iRS, fARS, fMRS, reOrder, INPUT_SIZE, lsq_Table, ld_sd_rs);
				clearBufEntry(fABuffer, INPUT_SIZE);
				break;
			case 2:
				broadCastCDBVal(fMBuffer, iRS, fARS, fMRS, reOrder, INPUT_SIZE, lsq_Table, ld_sd_rs);
				clearBufEntry(fMBuffer, INPUT_SIZE);
				break;
		}
		
		
		//Check if Reservation Stations are empty
		for(i = 0; i < intAdd_rs; i++) {
			if(iRS[i].isBusy == 1) {
				iRS_Done = 0;
				break;
			}
		}
	
		for(i = 0; i < FPAdd_rs; i++) {
			if(fARS[i].isBusy == 1) {
				fARS_Done = 0;
				break;
			}
		}
	
		for(i = 0; i < FPMult_rs; i++) {
			if(fMRS[i].isBusy == 1) {
				fMRS_Done = 0;
				break;
			}
		}

		//check ROB table
		for(i = 0; i < ROB_Entries; i++) {
			if(reOrder[i].type != 10) {
				rob_Done = 0;
				break;
			}
		}
		
		//check cdb buffer entries
		for(i = 0; i < CDB_Buffer_Entries; i++) {
			if(iBuffer[i].cdbBuffer.isBusy == 1) {
				cdbDone = 0;
				break;
			}
			if(fABuffer[i].cdbBuffer.isBusy == 1) {
				cdbDone = 0;
				break;
			}
			if(fMBuffer[i].cdbBuffer.isBusy == 1) {
				cdbDone = 0;
				break;
			}
		}
		
		//Check load/store queue
		for(i = 0; i < ld_sd_rs; i++) {
			if(lsq_Table[i].isBusy == 1) {
				lsqDone = 0;
				break;
			}
		}
		
		//if the pc decreases due to a branch, reset the noFetch flag
		if(pcAddrQueue < entry[numInstr - 1].address) {
			noFetch = 0;
		}
		
		//print_ROB_Table(reOrder,ROB_Entries);
		//print_RS_Table(iRS, intAdd_rs);
		//print_LSQ_Queue(lsq_Table, ld_sd_rs);
		//printf("cycle: %d\n", cycle_number);
		printf("Address: %d\n", pcAddrQueue);
		
		//If there is nothing more to fetch from the instruction queue, all of the reservation stations are empty,
		//and all of the ROB entries are empty, there is nothing in the cdb buffers, and the load/store queue is empty, then
		//there is nothing left to do
		if(noFetch == 1 && iRS_Done == 1 && fARS_Done == 1 && fMRS_Done == 1 && rob_Done == 1 && cdbDone == 1 && lsqDone == 1) {
			break;
		}
		
		if(cycle_number == 125) {
			break;
		}
		
		cycle_number++;
	}
  
	//Show results of program and nonzero values for the integer registers, floating point registers, and memory
	printResults(entry, IS, EX, MEM, WB, COM, numInstr);
	showIntReg(&iR);
	showFPReg(&fR);
	showMemory(memData);
	
	return 1;
  
} //end main