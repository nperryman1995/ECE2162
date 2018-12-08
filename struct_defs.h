#define true 1
#define false 0

typedef unsigned char boolean;

/*Instruction struct to take in all of the instruction types*/
struct instruction {
	unsigned char type;
	char Ra[4];
	char Fa[4];
	char Rs[4];
	char Fs[4];
	char Rt[4];
	char Ft[4];
	uint32_t offset; //For both offsets and immediates
	uint32_t address;
	unsigned char isValid;
	//uint32_t PC_Addr;
};

/*Encoding for instruction struct type*/
enum opcode_type {
	ti_Ld = 0, //Load a single precision floating point value
	ti_Sd, //Store a single precision floating point value to memory
	ti_Beq, //Branch if equal
	ti_Bne, //Branch if not equal
	ti_Add, //Integer addition
	ti_Addd, //Floating point addition
	ti_Addi, //Integer addition with immediate
	ti_Sub, //Integer subtraction
	ti_Subd, //Floating point subtraction
	ti_Multd //Floating point multiplication
};

/*32 integer register files*/
struct int_Reg {
	uint32_t R_num[32];
	char canWrite[32];
};

/*32 floating point register files*/
struct float_Reg {
	float F_num[32];
};

struct RAT_entry { // Register Alias Table entry. 
    unsigned char rType; //0 = ARF, 1 = ROB, 2 = not initialized yet
	unsigned char iOrf; //integer or floating point register
	uint32_t irNumber; //destination number for int
	uint32_t frNumber; //destination number for float
};

struct RS_entry { // Reservation Station entry.
	uint32_t address; //address of instruction
	unsigned char type; //type of operation
	uint32_t dst_tag; //ROB entry number
	uint32_t tag1; //ROB entry number for operand one
	uint32_t tag2; //ROB entry number for operand two
	int iVal1; //Integer Value 1 if Integer RS
	int iVal2; //Integer Value 2 if Integer RS
	float fVal1; //FP Value 1 if FP RS
	float fVal2; //FP Value 2 if FP RS
	unsigned char isBusy; //0 = available, 1 = busy, 2 = not initialized
	uint32_t cyclesLeft; //number of cycles left for execution
};

struct ROB_entry { // ReOrder Buffer entry.
	uint32_t address; //address of instruction
	unsigned char type; //type of operation; 0-9 = operation, 10 = not currently used, 11 = not valid
	char destReg[4]; //destination register
	uint32_t intVal; //integer value if integer operation
	uint32_t floatVal; //floating point value if FP operation
	unsigned char finOp; //0 means not finished yet, 1 means finished, but not ready to commmit, 2 means finished and can commit
};

struct LSQ_entry { // Load/Store Queue entry. Array of them makes the entire LSQ
    boolean valid; // is this entry used
    boolean type; // is it a load or a store
    void *mem_address; // address this instruction is going to access
    int ROB_number;
    void *value;
    boolean value_valid; // is the data stored in value valid (we might not need this field)
} LSQ_entry;

struct CDB_buffer { // Buffer for storing data when Common Data Bus is busy
    struct RS_entry cdbBuffer;
	uint32_t arrival_cycle;
	unsigned char isValid; //0 = valid, 1 = not valid
} CDB_buffer;

// Branch Target Buffer. Contains 8 entries as per the Branch Unit description
struct instruction *BTB_array[8]; // Stores the instruction that is predicted to be fetched after a branch

struct branch_pred_entry { // One entry of the branch predictor. Array of them makes the entire predictor
    boolean prediction;
} branch_pred_entry;

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
void storeMemory(float *memData, int regtempVal, float regtempValFP) {
	memData[regtempVal] = regtempValFP;
} //end storeMemory

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
float memRetr(float *memData, int memNum) {
	float ret = memData[memNum];
	return ret;
} //end memRetr

/*Given the register string, find the register number*/
int regLookup(char *reg) {
	char *p = reg;
	int ret = 0;
	while (*p) { // While there are more characters to process
		if (isdigit(*p)) {
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
void showMemory(float *memData) {
	int i;
	for(i = 0; i < 64; i++) {
		if(memData[i] != 0) {
			printf("Mem[%d] = %.2f\n", i*4, memData[i]);
		} //end if
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
	stages[0] = instr[pc_Addr/4];
} //end instFetch

/*Print the contents of the pipeline*/
void printPipeline(uint32_t cycle_number, struct instruction *stages) {
	printf("------------------------------------------------\n");
	printf("Cycle Number = %d\n", cycle_number);
	printf("Issue: ");
	printInstr(stages[0]);
	printf("\n");
	printf("Ex: ");
	printInstr(stages[1]);
	printf("\n");
	printf("Mem: ");
	printInstr(stages[2]);
	printf("\n");
	printf("WB: ");
	printInstr(stages[3]);
	printf("\n");
	printf("Commit: ");
	printInstr(stages[4]);
	printf("\n");
	printf("------------------------------------------------\n");
} //end printPipeline

/*Load a single precision floating point value to Fa*/
void inst_load(struct instruction instr, struct int_Reg *iR, struct float_Reg *fR, float *mU) {
	int fa = regLookup(instr.Fa);
	int ra = regLookup(instr.Ra);
	int offs = instr.offset;
	fR->F_num[fa] = mU[(iR->R_num[ra]+offs)/4];
} //end inst_load

/*Store a single precision floating point value to memory*/
void inst_store(struct instruction instr, struct int_Reg *iR, struct float_Reg *fR, float *mU) {
	int fa = regLookup(instr.Fa);
	int ra = regLookup(instr.Ra);
	int offs = instr.offset;
	mU[(iR->R_num[ra]+offs)/4] = fR->F_num[fa];
} //end inst_store

/*If Rs==Rt then branch to PC+4+offset<<2*/
uint32_t eBranch(struct instruction instr, struct int_Reg *iR) {
	int offs = instr.offset;
	int rs = regLookup(instr.Rs); //decode register number
	int rt = regLookup(instr.Rt); //decode register number
	uint32_t pcA = instr.address;
	if(iR->R_num[rs] == iR->R_num[rt]) {
		pcA = pcA + 4 + (offs<<2);
	}
	return pcA;
} //end eBranch

/*If Rs!=Rt then branch to PC+4+offset<<2*/
uint32_t nBranch(struct instruction instr, struct int_Reg *iR) {
	int offs = instr.offset;
	int rs = regLookup(instr.Rs); //decode register number
	int rt = regLookup(instr.Rt); //decode register number
	uint32_t pcA = instr.address;
	if(iR->R_num[rs] != iR->R_num[rt]) {
		pcA = pcA + 4 + (offs<<2);
	}
	return pcA;
} //end nBranch

/*Integer Addition*/
void intAdd(struct instruction instr, struct int_Reg *iR) {
	int ra = regLookup(instr.Ra); //decode register number
	int rs = regLookup(instr.Rs); //decode register number
	int rt = regLookup(instr.Rt); //decode register number

	int temp = iR->R_num[rs] + iR->R_num[rt];
	if(temp > 2147483647) { //cap to maximum/minimum int value
		temp = 2147483647;
	}else if(temp < -2147483648) {
		temp = -2147483648;
	}
	iR->R_num[ra] = temp;
} //end intAdd

/*Floating Point Addition*/
void FPAdd(struct instruction instr, struct float_Reg *fR) {
	int ra = regLookup(instr.Fa); //decode register number
	int rs = regLookup(instr.Fs); //decode register number
	int rt = regLookup(instr.Ft); //decode register number
	
	float temp = fR->F_num[rs] + fR->F_num[rt];
	if(temp > 3.402823466e+38F) { //cap to maximum/minimum floating point value
		temp = 3.402823466e+38F;
	}else if(temp < -3.402823466e+38F) {
		temp = -3.402823466e+38F;
	}
	fR->F_num[ra] = temp;
} //end FPAdd

/*Integer Addition with immediate*/
void immAdd(struct instruction instr, struct int_Reg *iR) {
	int immed = instr.offset;
	int rs = regLookup(instr.Rs); //decode register number
	int rt = regLookup(instr.Rt); //decode register number
	
	int temp = iR->R_num[rs] + immed;
	if(temp > 2147483647) { //cap to maximum/minimum int value
		temp = 2147483647;
	}else if(temp < -2147483648) {
		temp = -2147483648;
	}
	iR->R_num[rt] = temp;
} //end immAdd

/*Integer Subtraction*/
void intSub(struct instruction instr, struct int_Reg *iR) {
	int ra = regLookup(instr.Ra); //decode register number
	int rs = regLookup(instr.Rs); //decode register number
	int rt = regLookup(instr.Rt); //decode register number

	int temp = iR->R_num[rs] - iR->R_num[rt];
	if(temp > 2147483647) { //cap to maximum/minimum int value
		temp = 2147483647;
	}else if(temp < -2147483648) {
		temp = -2147483648;
	}
	iR->R_num[ra] = temp;
} //end intSub

/*Floating Point subtraction*/
void FPSub(struct instruction instr, struct float_Reg *fR) {
	int ra = regLookup(instr.Fa); //decode register number
	int rs = regLookup(instr.Fs); //decode register number
	int rt = regLookup(instr.Ft); //decode register number
	
	float temp = fR->F_num[rs] - fR->F_num[rt];
	if(temp > 3.402823466e+38F) { //cap to maximum/minimum floating point value
		temp = 3.402823466e+38F;
	}else if(temp < -3.402823466e+38F) {
		temp = -3.402823466e+38F;
	}
	fR->F_num[ra] = temp;
} //end FPSub

/*Floating point multiplication*/
void FPMult(struct instruction instr, struct float_Reg *fR) {
	int ra = regLookup(instr.Fa); //decode register number
	int rs = regLookup(instr.Fs); //decode register number
	int rt = regLookup(instr.Ft); //decode register number
	
	float temp = fR->F_num[rs] * fR->F_num[rt];
	if(temp > 3.402823466e+38F) { //cap to maximum floating point value
		temp = 3.402823466e+38F;
	}else if(temp < -3.402823466e+38F) {
		temp = -3.402823466e+38F;
	}
	fR->F_num[ra] = temp;
} //end FPMult

instExecute(struct instruction instr, struct int_Reg *iR, struct float_Reg *fR, float *memData) {
	switch(instr.type) {
		case ti_Ld: //Load case
			inst_load(instr, iR, fR, memData);
			break;
		case ti_Sd: //Store case
			inst_store(instr, iR, fR, memData);
			break;		
		case ti_Beq: //Branch if equal case
			eBranch(instr, iR);
			break;		
		case ti_Bne: //Branch if not equal case
			nBranch(instr, iR);
			break;		
		case ti_Add: //Integer addition case
			intAdd(instr, iR);
			break;		
		case ti_Addd: //Floating point addition case
			FPAdd(instr, fR);
			break;		
		case ti_Addi: //Integer Immediate addition case
			immAdd(instr, iR);
			break;		
		case ti_Sub: //Integer subtraction
			intSub(instr, iR);
			break;		
		case ti_Subd: //Floating point subtraction
			FPSub(instr, fR);
			break;		
		case ti_Multd: //Floating point multiplication
			FPMult(instr, fR);
			break;		
	} //end switch
}

/*Check to see if the pipeline still has any contents*/
int checkPipeline(struct instruction *instr) {
	int i;
	for(i = 0; i < 5; i++) {
		if(instr[i].isValid == 1) {
			return 1;
		}
	}
	return 0;
}

void exExecution(struct instruction instr, struct int_Reg *iR, struct float_Reg *fR, float *memData) {
	switch(instr.type) {
		case ti_Beq: //Branch if equal case
			instExecute(instr, iR, fR, memData);
			break;		
		case ti_Bne: //Branch if not equal case
			instExecute(instr, iR, fR, memData);
			break;		
		case ti_Add: //Integer addition case
			instExecute(instr, iR, fR, memData);
			break;		
		case ti_Addd: //Floating point addition case
			instExecute(instr, iR, fR, memData);
			break;		
		case ti_Addi: //Integer Immediate addition case
			instExecute(instr, iR, fR, memData);
			break;		
		case ti_Sub: //Integer subtraction
			instExecute(instr, iR, fR, memData);
			break;		
		case ti_Subd: //Floating point subtraction
			instExecute(instr, iR, fR, memData);
			break;		
		case ti_Multd: //Floating point multiplication
			instExecute(instr, iR, fR, memData);
			break;		
	} //end switch
}

void memExecution(struct instruction instr, struct int_Reg *iR, struct float_Reg *fR, float *memData) {
	switch(instr.type) {
		case ti_Ld: //Load case
			instExecute(instr, iR, fR, memData);
			break;
		case ti_Sd: //Store case
			instExecute(instr, iR, fR, memData);
			break;			
	}
}

void printResults(struct instruction *entry, int *IS, int *EX, int *MEM, int *WB, int *COM, int numInstr) {
	int i;
	printf("\t\t\tISSUE\tEX\tMEM\tWB\tCOMMIT\n");
	for(i = 0; i < numInstr; i++) {
		printInstr(entry[i]);
		printf("\n");
		printf("\t\t\t%d\t%d\t%d\t%d\t%d\n", IS[i], EX[i], MEM[i], WB[i], COM[i]);
	}
}

uint32_t find_ROB_Head(struct ROB_entry *reOrder, uint32_t ROB_Entries) {
	uint32_t tag = 0;
	for(i = 0; i < ROB_Entries; i++) {
		if(reOrder[i].isHead == 1) {
			tag = i;
			break;
		}
	}
	return tag;
}

uint32_t get_int_ROB_tag(struct ROB_entry *reOrder, uint32_t ROB_Entries, struct instruction *forResStat, struct RAT_entry *rat_Table) {
	uint32_t tag = 0;
	unsigned char decVal = 0;
	int i;
	uint32_t robHead = find_ROB_Head(reOrder, ROB_Entries);
	for(i = robHead; i < ROB_Entries; i++) {
		if(i == robHead && decVal == 1) {
			break;
		}
		if(reOrder[i].type == 10) {
			tag = i;
			reOrder[i].type = forResStat->type;
			reOrder[i].address = forResStat->address;
			reOrder[i].intVal = -1;
			reOrder[i].finOp = 0;
			switch(forResStat->type) {		
				case ti_Add: //Integer addition case
					reOrder[i].destReg = forResStat->Ra;
					break;				
				case ti_Addi: //Integer Immediate addition case
					reOrder[i].destReg = forResStat->Rt;
					break;		
				case ti_Sub: //Integer subtraction
					reOrder[i].destReg = forResStat->Ra;
					break;		
			}
			int rd = regLookup(reOrder[i].destReg);
			rat_Table[rd].rType = 1;
			rat_Table[rd].iOrf = 0;
			rat_Table[rd].irNumber = i;
			return tag;
		}
		if((i == ROB_Entries - 1) && (robHead != 0)) {
			i = 0;
			decVal = 1;
		}
	}
	return tag;
}

uint32_t get_float_ROB_tag(struct ROB_entry *reOrder, uint32_t ROB_Entries, struct instruction *forResStat, struct RAT_entry *rat_Table) {
	uint32_t tag = 0;
	unsigned char decVal = 0;
	int i;
	uint32_t robHead = find_ROB_Head(reOrder, ROB_Entries);
	for(i = robHead; i < ROB_Entries; i++) {
		if(i == robHead && decVal == 1) {
			break;
		}
		if(reOrder[i].type == 10) {
			tag = i;
			reOrder[i].type = forResStat->type;
			reOrder[i].address = forResStat->address;
			reOrder[i].floatVal = -1;
			reOrder[i].finOp = 0;
			reOrder[i].destReg = forResStat->Fa;
			int rd = regLookup(reOrder[i].destReg);
			rat_Table[rd].rType = 1;
			rat_Table[rd].iOrf = 1;
			rat_Table[rd].frNumber = i;
			return tag;
		}
		if((i == ROB_Entries - 1) && (robHead != 0)) {
			i = 0;
			decVal = 1;
		}
	}
	return tag;
}

uint32_t get_RAT_tag(struct RAT_entry *rat_Table, char *fd) {
	uint32_t tag = 0;
	int rd = regLookup(fd); //decode register number
	if(rat_Table[rd].rType == 2) { //if not initialized, just grab from architectured register file
		tag = -1;
	}else if (rat_Table[rd].rType == 0) { //if pointing to the ARF, grab from there
		tag = -1;
	}else {
		if(rat_Table[rd].iOrf == 0) {
			tag = rat_Table[rd].irNumber;
		}else {
			tag = rat_Table[rd].frNumber;
		}
	}
	return tag;
}

uint32_t get_int_RAT_Value(char *fd, struct int_reg *iR) {
	int rd = regLookup(fd);
	uint32_t retVal = iR[rd].R_num;
	return retVal;
}

float get_float_RAT_Value(char *fd, struct float_Reg *fR) {
	int rd = regLookup(fd);
	float retVal = fR[rd].F_num;
	return retVal;	
}

void iRSFill(struct RS_entry *iRS, struct ROB_entry *reOrder, struct int_reg *iR, struct RAT_entry *rat_Table, int i, struct instruction *forResStat, uint32_t ROB_Entries, uint32_t maxRB) {
	iRS[i].address = forResStat->address;
	iRS[i].type = forResStat->type;
	iRS[i].isBusy = 1;
	switch(forResStat->type) {		
		case ti_Add: //Integer addition case
			iRS[i].tag1 = get_RAT_tag(rat_Table, forResStat->Rs);
			if(iRS[i].tag1 == -1) {
				iRS[i].iVal1 = get_int_RAT_Value(forResStat->Rs, iR);
			}else {
				if(reOrder[iRS[i].tag1].finOp == 1 || reOrder[iRS[i].tag1].finOp == 2) {
					iRS[i].tag1 = -1;
					iRS[i].iVal1 = reOrder[iRS[i].tag1].intVal;
				}				
			}
			iRS[i].tag2 = get_RAT_tag(rat_Table, forResStat->Rt);
			if(iRS[i].tag2 == -1) {
				iRS[i].iVal2 = get_int_RAT_Value(forResStat->Rt, iR);
			}else {
				if(reOrder[iRS[i].tag2].finOp == 1 || reOrder[iRS[i].tag2].finOp == 2) {
					iRS[i].tag2 = -1;
					iRS[i].iVal2 = reOrder[iRS[i].tag2].intVal;
				}				
			}
			break;				
		case ti_Addi: //Integer Immediate addition case
			iRS[i].tag1 = get_RAT_tag(rat_Table, forResStat->Rs);
			if(iRS[i].tag1 == -1) {
				iRS[i].iVal1 = get_int_RAT_Value(forResStat->Rs, iR);
			}else {
				if(reOrder[iRS[i].tag1].finOp == 1 || reOrder[iRS[i].tag1].finOp == 2) {
					iRS[i].tag1 = -1;
					iRS[i].iVal1 = reOrder[iRS[i].tag1].intVal;
				}				
			}
			iRS[i].tag2 = -1;
			iRS[i].iVal2 =	forResStat->offset;
			break;		
		case ti_Sub: //Integer subtraction
			iRS[i].tag1 = get_RAT_tag(rat_Table, forResStat->Rs);
			if(iRS[i].tag1 == -1) {
				iRS[i].iVal1 = get_int_RAT_Value(forResStat->Rs, iR);
			}else {
				if(reOrder[iRS[i].tag1].finOp == 1 || reOrder[iRS[i].tag1].finOp == 2) {
					iRS[i].tag1 = -1;
					iRS[i].iVal1 = reOrder[iRS[i].tag1].intVal;
				}				
			}
			iRS[i].tag2 = get_RAT_tag(rat_Table, forResStat->Rt);
			if(iRS[i].tag2 == -1) {
				iRS[i].iVal2 = get_int_RAT_Value(forResStat->Rt, iR);
			}else {
				if(reOrder[iRS[i].tag2].finOp == 1 || reOrder[iRS[i].tag2].finOp == 2) {
					iRS[i].tag2 = -1;
					iRS[i].iVal2 = reOrder[iRS[i].tag2].intVal;
				}				
			}		
			break;		
	}
	iRS[i].dst_tag = get_int_ROB_tag(reOrder, ROB_Entries, forResStat, rat_Table);
}

void fARSFill(struct RS_entry *fARS, struct ROB_entry *reOrder, struct float_Reg *fR, struct RAT_entry *rat_Table, int i, struct instruction *forResStat, uint32_t ROB_Entries, uint32_t MAX_ROB) {
	fARS[i].address = forResStat->address;
	fARS[i].type = forResStat->type;
	fARS[i].isBusy = 1;
	switch(forResStat->type) {		
		case ti_Addd: //Floating point addition case
			fARS[i].tag1 = get_RAT_tag(rat_Table, forResStat->Fs);
			if(fARS[i].tag1 == -1) {
				fARS[i].fVal1 = get_float_RAT_Value(forResStat->Fs, fR);
			}else {
				if(reOrder[fARS[i].tag1].finOp == 1 || reOrder[fARS[i].tag1].finOp == 2) {
					fARS[i].tag1 = -1;
					fARS[i].fVal1 = reOrder[fARS[i].tag1].floatVal;
				}				
			}
			fARS[i].tag2 = get_RAT_tag(rat_Table, forResStat->Ft);
			if(fARS[i].tag2 == -1) {
				fARS[i].fVal2 = get_float_RAT_Value(forResStat->Ft, fR);
			}else {
				if(reOrder[fARS[i].tag2].finOp == 1 || reOrder[fARS[i].tag2].finOp == 2) {
					fARS[i].tag2 = -1;
					fARS[i].fVal2 = reOrder[fARS[i].tag2].floatVal;
				}				
			}
			break;			
		case ti_Subd: //Floating point subtraction
			fARS[i].tag1 = get_RAT_tag(rat_Table, forResStat->Fs);
			if(fARS[i].tag1 == -1) {
				fARS[i].fVal1 = get_float_RAT_Value(forResStat->Fs, fR);
			}else {
				if(reOrder[fARS[i].tag1].finOp == 1 || reOrder[fARS[i].tag1].finOp == 2) {
					fARS[i].tag1 = -1;
					fARS[i].fVal1 = reOrder[fARS[i].tag1].floatVal;
				}				
			}
			fARS[i].tag2 = get_RAT_tag(rat_Table, forResStat->Ft);
			if(fARS[i].tag2 == -1) {
				fARS[i].fVal2 = get_float_RAT_Value(forResStat->Ft, fR);
			}else {
				if(reOrder[fARS[i].tag2].finOp == 1 || reOrder[fARS[i].tag2].finOp == 2) {
					fARS[i].tag2 = -1;
					fARS[i].fVal2 = reOrder[fARS[i].tag2].floatVal;
				}				
			}
			break;	
	}
	fARS[i].dst_tag = get_float_ROB_tag(reOrder, ROB_Entries, forResStat, rat_Table);
}

void fMRSFill(struct RS_entry *fMRS, struct ROB_entry *reOrder, struct float_Reg *fR, struct RAT_entry *rat_Table, int i, struct instruction *forResStat, uint32_t ROB_Entries, uint32_t MAX_ROB) {
	fMRS[i].address = forResStat->address;
	fMRS[i].type = forResStat->type;
	fMRS[i].isBusy = 1;
	fMRS[i].tag1 = get_RAT_tag(rat_Table, forResStat->Fs);
	if(fMRS[i].tag1 == -1) {
		fMRS[i].fVal1 = get_float_RAT_Value(forResStat->Fs, fR);
	}else {
		if(reOrder[fMRS[i].tag1].finOp == 1 || reOrder[fMRS[i].tag1].finOp == 2) {
			fMRS[i].tag1 = -1;
			fMRS[i].fVal1 = reOrder[fMRS[i].tag1].floatVal;
		}				
	}
	fMRS[i].tag2 = get_RAT_tag(rat_Table, forResStat->Ft);
	if(fMRS[i].tag2 == -1) {
		fMRS[i].fVal2 = get_float_RAT_Value(forResStat->Ft, fR);
	}else {
		if(reOrder[fMRS[i].tag2].finOp == 1 || reOrder[fMRS[i].tag2].finOp == 2) {
			fMRS[i].tag2 = -1;
			fMRS[i].fVal2 = reOrder[fMRS[i].tag2].floatVal;
		}				
	}
	fMRS[i].dst_tag = get_float_ROB_tag(reOrder, ROB_Entries, forResStat, rat_Table);
}

uint32_t isCDBBufEmpty(struct CDB_buffer *buf, uint32_t iSize) {
	unsigned char isEmpty = -1; //not empty assumption
	int i;
	for(i = 0; i < iSize; i++) {
		if(buf[i].isValid == 0) {
			if(buf[i].cdbBuffer.isBusy == 0) {
				isEmpty = i;
				break;
			}
		}
	}
	return isEmpty;
}

void putCDBEntry(struct RS_entry *rsEnt, struct CDB_buffer *cBuf, uint32_t rEntry, uint32_t cEntry, uint32_t cycle_num) {
	cBuf[cEntry].cdbBuffer.address = rsEnt[rEntry].address;
	cBuf[cEntry].cdbBuffer.type = rsEnt[rEntry].type;
	cBuf[cEntry].cdbBuffer.dst_tag = rsEnt[rEntry].dst_tag;
	cBuf[cEntry].cdbBuffer.tag1 = rsEnt[rEntry].tag1;
	cBuf[cEntry].cdbBuffer.tag2 = rsEnt[rEntry].tag2;
	cBuf[cEntry].cdbBuffer.iVal1 = rsEnt[rEntry].iVal1;
	cBuf[cEntry].cdbBuffer.iVal2 = rsEnt[rEntry].iVal2;
	cBuf[cEntry].cdbBuffer.fVal1 = rsEnt[rEntry].fVal1;
	cBuf[cEntry].cdbBuffer.fVal2 = rsEnt[rEntry].fVal2;
	cBuf[cEntry].cdbBuffer.isBusy = rsEnt[rEntry].isBusy;
	cBuf[cEntry].cdbBuffer.cyclesLeft = rsEnt[rEntry].cyclesLeft;
	cBuf[cEntry].arrival_cycle = cycle_num;
	cBuf[cEntry].isValid = 0;
}

void clearRSEntry(struct RS_entry *rsEnt, uint32_t rEntry) {
	rsEnt[rEntry].address = 0;
	rsEnt[rEntry].type = 10;
	rsEnt[rEntry].dst_tag = 0;
	rsEnt[rEntry].tag1 = 0;
	rsEnt[rEntry].tag2 = 0;
	rsEnt[rEntry].iVal1 = 0;
	rsEnt[rEntry].iVal2 = 0;
	rsEnt[rEntry].fVal1 = 0;
	rsEnt[rEntry].fVal2 = 0;
	rsEnt[rEntry].isBusy = 0;
	rsEnt[rEntry].cyclesLeft = 0;
}

void clearBufEntry(struct CDB_buffer *cBuf) {
	cBuf[0].cdbBuffer.address = 0;
	cBuf[0].cdbBuffer.type = 10;
	cBuf[0].cdbBuffer.dst_tag = 0;
	cBuf[0].cdbBuffer.tag1 = 0;
	cBuf[0].cdbBuffer.tag2 = 0;
	cBuf[0].cdbBuffer.iVal1 = 0;
	cBuf[0].cdbBuffer.iVal2 = 0;
	cBuf[0].cdbBuffer.fVal1 = 0;
	cBuf[0].cdbBuffer.fVal2 = 0;
	cBuf[0].cdbBuffer.isBusy = 0;
	cBuf[0].cdbBuffer.cyclesLeft = 0;
	cBuf[0].arrival_cycle = 0;
	int i;
	for(i = 0; i < iSize; i++) {
		if(cbuf[i].isValid == 0) {
			if(i != 0) {
				if(cbuf[i+1].isValid == 0) {
					cbuf[i] = cbuf[i+1];
				}else {
					cBuf[i].cdbBuffer.address = 0;
					cBuf[i].cdbBuffer.type = 10;
					cBuf[i].cdbBuffer.dst_tag = 0;
					cBuf[i].cdbBuffer.tag1 = 0;
					cBuf[i].cdbBuffer.tag2 = 0;
					cBuf[i].cdbBuffer.iVal1 = 0;
					cBuf[i].cdbBuffer.iVal2 = 0;
					cBuf[i].cdbBuffer.fVal1 = 0;
					cBuf[i].cdbBuffer.fVal2 = 0;
					cBuf[i].cdbBuffer.isBusy = 0;
					cBuf[i].cdbBuffer.cyclesLeft = 0;
					cBuf[i].arrival_cycle = 0;
				}
			}
		}
	}
}

void broadCastCDBVal(struct CDB_buffer *cBuffer, struct RS_entry *iRS, struct RS_entry *fARS, struct RS_entry *fMRS, struct ROB_entry *reOrder, uint32_t iSize) {
	uint32_t intVal = 0;
	float floatVal = 0;
	switch(cBuffer[0].cdbBuffer.type) {
		case ti_Add:
			intVal = cBuffer[0].cdbBuffer.iVal1 + cBuffer[0].cdbBuffer.iVal2;
			break;		
		case ti_Addd:
			floatVal = cBuffer[0].cdbBuffer.fVal1 + cBuffer[0].cdbBuffer.fVal2;
			break;		
		case ti_Addi:
			intVal = cBuffer[0].cdbBuffer.iVal1 + cBuffer[0].cdbBuffer.iVal2;
			break;		
		case ti_Sub:
			intVal = cBuffer[0].cdbBuffer.iVal1 - cBuffer[0].cdbBuffer.iVal2;
			break;		
		case ti_Subd:
			floatVal = cBuffer[0].cdbBuffer.fVal1 - cBuffer[0].cdbBuffer.fVal2;
			break;		
		case ti_Multd:
			floatVal = cBuffer[0].cdbBuffer.fVal1 * cBuffer[0].cdbBuffer.fVal2;
			break;	
	}
	int i;
	for(i = 0; i < iSize; i++) {
		if(iRS[i].isBusy == 1) {
			if(iRS[i].tag1 == cBuffer[0].cdbBuffer.dst_tag) {
				iRS[i].tag1 = -1;
				iRS[i].iVal1 = intVal;
			}else if(iRS[i].tag2 == cBuffer[0].cdbBuffer.dst_tag) {
				iRS[i].tag2 = -1;
				iRS[i].iVal2 = intVal;	
			}
		}
	}
	for(i = 0; i < iSize; i++) {
		if(fARS[i].isBusy == 1) {
			if(fARS[i].tag1 == cBuffer[0].cdbBuffer.dst_tag) {
				fARS[i].tag1 = -1;
				fARS[i].fVal1 = floatVal;
			}else if(fARS[i].tag2 == cBuffer[0].cdbBuffer.dst_tag) {
				fARS[i].tag2 = -1;
				fARS[i].fVal2 = floatVal;	
			}
		}
	}
	for(i = 0; i < iSize; i++) {
		if(fMRS[i].isBusy == 1) {
			if(fMRS[i].tag1 == cBuffer[0].cdbBuffer.dst_tag) {
				fMRS[i].tag1 = -1;
				fMRS[i].fVal1 = floatVal;
			}else if(fMRS[i].tag2 == cBuffer[0].cdbBuffer.dst_tag) {
				fMRS[i].tag2 = -1;
				fMRS[i].fVal2 = floatVal;	
			}
		}
	}
	reOrder[cBuffer[0].cdbBuffer.dst_tag].finOp = 1;
	reOrder[cBuffer[0].cdbBuffer.dst_tag].intVal = intVal;
	reOrder[cBuffer[0].cdbBuffer.dst_tag].floatVal = floatVal;
}

unsigned char cdb_Execute(struct CDB_buffer *iBuffer, struct CDB_buffer *fABuffer, struct CDB_buffer *fMBuffer) {
	unsigned char buf_choice; //0 = int buf, 1 = float add buf, 2 = float multiply buf, 3 = nothing is ready
	int i;
	if(iBuffer[0].cdbBuffer.isBusy == 1 && fABuffer[0].cdbBuffer.isBusy == 1 && fMBuffer[0].cdbBuffer.isBusy == 1) {//all have values
		if(iBuffer[0].arrival_cycle < fABuffer[0].arrival_cycle && iBuffer[0].arrival_cycle < fMBuffer[0].arrival_cycle) {
			buf_choice = 0;
		}else if(fABuffer[0].arrival_cycle < iBuffer[0].arrival_cycle && fABuffer[0].arrival_cycle < fMBuffer[0].arrival_cycle) {
			buf_choice = 1;
		}else if(fMBuffer[0].arrival_cycle < iBuffer[0].arrival_cycle && fMBuffer[0].arrival_cycle < fABuffer[0].arrival_cycle) {
			buf_choice = 2;
		}else if(iBuffer[0].arrival_cycle == fABuffer[0].arrival_cycle && iBuffer[0].arrival_cycle < fMBuffer[0].arrival_cycle) {
			buf_choice = 1;
		}else if(iBuffer[0].arrival_cycle < fABuffer[0].arrival_cycle && iBuffer[0].arrival_cycle == fMBuffer[0].arrival_cycle) {
			buf_choice = 2;
		}else if(fABuffer[0].arrival_cycle < iBuffer[0].arrival_cycle && fABuffer[0].arrival_cycle == fMBuffer[0].arrival_cycle) {
			buf_choice = 2;
		}else {
			buf_choice = 2;
		}
	}else if(iBuffer[0].cdbBuffer.isBusy == 1 && fABuffer[0].cdbBuffer.isBusy == 1) { //int and float Add have values
		if(fABuffer[0].arrival_cycle <= iBuffer[0].arrival_cycle) {
			buf_choice = 1;
		}else{
			buf_choice = 0;
		}
	}else if(iBuffer[0].cdbBuffer.isBusy == 1 && fMBuffer[0].cdbBuffer.isBusy == 1) { //int and float multiply have values
		if(fMBuffer[0].arrival_cycle <= iBuffer[0].arrival_cycle) {
			buf_choice = 2;
		}else{
			buf_choice = 0;
		}
	}else if(fABuffer[0].cdbBuffer.isBusy == 1 && fMBuffer[0].cdbBuffer.isBusy == 1) { //float Add and float Multiply have values
		if(fMBuffer[0].arrival_cycle <= fABuffer[0].arrival_cycle) {
			buf_choice = 2;
		}else{
			buf_choice = 1;
		}
	}else if(iBuffer[0].cdbBuffer.isBusy == 1) { //only int has a value
		buf_choice = 0;
	}else if(fABuffer[0].cdbBuffer.isBusy == 1) { //only float add has a value
		buf_choice = 1;
	}else if(fMBuffer[0].cdbBuffer.isBusy == 1) { //only float multiply has a value
		buf_choice = 2;
	}else{
		buf_choice = 3;
	}
	return buf_choice;
}

void RS_Execute(struct RS_entry *iRS, struct RS_entry *fARS, struct RS_entry *fMRS, uint32_t iSize, struct CDB_buffer *iBuffer, struct CDB_buffer *fABuffer, struct CDB_buffer *fMBuffer, uint32_t cycle_num) {
	int i;
	for(i = 0; i < iSize; i++) {
		if(iRS[i].isBusy == 1) { //if occupied by instruction
			if(iRS[i].tag1 == -1 && iRS[i].tag2 == -1) { //if values in value fields have valid values and not tags, execute
				if(iRS[i].cyclesLeft != 0) {
					iRS[i].cyclesLeft = iRS[i].cyclesLeft - 1;
				}
				if(iRS[i].cyclesLeft == 0)  {
					uint32_t cEntry = isCDBBufEmpty(iBuffer, iSize);
					if(cEntry != -1) {
						putCDBEntry(iRS, iBuffer, i, cEntry, cycle_num);
						clearRSEntry(iRS, i);
					}
				}
			}
		}
	}
	
	for(i = 0; i < iSize; i++) {
		if(fARS[i].isBusy == 1) { //if occupied by instruction
			if(fARS[i].tag1 == -1 && fARS[i].tag2 == -1) { //if values in value fields have valid values and not tags, execute
				if(fARS[i].cyclesLeft != 0) {
					fARS[i].cyclesLeft = fARS[i].cyclesLeft - 1;
				}
				if(fARS[i].cyclesLeft == 0)  {
					uint32_t cEntry = isCDBBufEmpty(fABuffer, iSize);
					if(cEntry != -1) {
						putCDBEntry(fARS, fABuffer, i, cEntry, cycle_num);
						clearRSEntry(fARS, i);
					}
				}
			}
		}
	}
	
	for(i = 0; i < iSize; i++) {
		if(fMRS[i].isBusy == 1) { //if occupied by instruction
			if(fMRS[i].tag1 == -1 && fMRS[i].tag2 == -1) { //if values in value fields have valid values and not tags, execute
				if(fMRS[i].cyclesLeft != 0) {
					fMRS[i].cyclesLeft = fMRS[i].cyclesLeft - 1;
				}
				if(fMRS[i].cyclesLeft == 0)  {
					uint32_t cEntry = isCDBBufEmpty(fMBuffer, iSize);
					if(cEntry != -1) {
						putCDBEntry(fMRS, fMBuffer, i, cEntry, cycle_num);
						clearRSEntry(fMRS, i);
					}
				}
			}
		}
	}
}

void updateROB(struct ROB_entry *reOrder, uint32_t ROB_Entries, struct RAT_entry *rat_Table, struct int_Reg *iR, struct float_Reg *fR) {
	int i;
	unsigned char int_float; //0 = int, 1 = float
	for(i = 0; i < ROB_Entries; i++) {
		if(reOrder[i].finOp == 2 && reOrder[i].isHead == 1) {
			switch(reOrder[i].type) {
				case ti_Add:
					int_float = 0;
					break;		
				case ti_Addd:
					int_float = 1;
					break;		
				case ti_Addi:
					int_float = 0;
					break;		
				case ti_Sub:
					int_float = 0;
					break;		
				case ti_Subd:
					int_float = 1;
					break;		
				case ti_Multd:
					int_float = 1;
					break;				
			}
			if(int_float == 0) {
				iR[regLookup(ReOrder[i].dstReg)] = reOrder[i].intVal;
				if(rat_Table[regLookup(ReOrder[i].dstReg)].iOrf == 0 && rat_Table[regLookup(ReOrder[i].dstReg)].irNumber == i) {
					rat_Table[regLookup(ReOrder[i].dstReg)].rType = 0;
				}
			}else{
				fR[regLookup(ReOrder[i].dstReg)] = reOrder[i].floatVal;
				if(rat_Table[regLookup(ReOrder[i].dstReg)].iOrf == 1 && rat_Table[regLookup(ReOrder[i].dstReg)].frNumber == i) {
					rat_Table[regLookup(ReOrder[i].dstReg)].rType = 0;
				}
			}
			ReOrder[i].address = 0;
			ReOrder[i].type = 10;
			ReOrder[i].destReg = 0;
			ReOrder[i].intVal = 0;
			ReOrder[i].floatVal = 0;
			ReOrder[i].finOp = 0;
			ReOrder[i].isHead = 0;
			if(i == ROB_Entries - 1) {
				ReOrder[0].isHead = 1;
			}else {
				ReOrder[i+1].isHead = 1;
			}
		}
	}
	
	for(i = 0; i < ROB_Entries; i++) {
		if(reOrder[i].finOp == 1) {
			finOp = 2;
		}
	}
}