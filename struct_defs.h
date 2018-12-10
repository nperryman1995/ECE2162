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
    unsigned char rType; //0 = ARF, 1 = int ROB, 2 = not initialized yet
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
	unsigned char isBusy; //0 = available, 1 = busy, 2 = not initialized, 3 = just put in, don't execute yet, 4 = no ROB entry available, wait until one is available
	uint32_t cyclesLeft; //number of cycles left for execution
	char destReg[4];
};

struct ROB_entry { // ReOrder Buffer entry.
	uint32_t address; //address of instruction
	unsigned char type; //type of operation; 0-9 = operation, 10 = not currently used, 11 = not valid
	char destReg[4]; //destination register
	uint32_t intVal; //integer value if integer operation
	float floatVal; //floating point value if FP operation
	unsigned char finOp; //0 means not finished yet, 1 means finished, but not ready to commmit, 2 means finished and can commit
	unsigned char isHead; //0 means not head, 1 means head
};

struct LSQ_entry { // Load/Store Queue entry. Array of them makes the entire LSQ
	uint32_t address; //address of instruction
	unsigned char type; //load or store
	uint32_t dst_tag; //destination tag if store
	float dst_Val; //ROB entry value if not pulled from ARF
	uint32_t tag; //ROB entry number for register containing store address or load address 
	uint32_t offset; //offset in memory
	uint32_t ex_cyclesLeft; //execution cycles left
	uint32_t mem_cyclesLeft; //memory cycles left
	char Fa[4]; //destination register if store
	unsigned char isHead; //is it at the front of the queue, 0 = no, 1 = yes
	unsigned char isBusy; //0 = available, 1 = busy, 2 = not initialized, 3 = just put in, don't execute yet
	uint32_t memVal;
	uint32_t ROB_tag;
	float forwardedMEMValue;
};

struct CDB_buffer { // Buffer for storing data when Common Data Bus is busy
    struct RS_entry cdbBuffer;
	uint32_t arrival_cycle;
	unsigned char isValid; //0 = valid, 1 = not valid
};

// Branch Target Buffer. Contains 8 entries as per the Branch Unit description
struct instruction *BTB_array[8]; // Stores the instruction that is predicted to be fetched after a branch

struct branch_pred_entry { // One entry of the branch predictor. Array of them makes the entire predictor
    boolean prediction;
} branch_pred_entry;

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

void instExecute(struct instruction instr, struct int_Reg *iR, struct float_Reg *fR, float *memData) {
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
		switch(entry[i].type) {
			case ti_Ld: //Load case
				printf("\t\t\t%d\t%d\t%d\t%d\t%d\n", IS[i], EX[i], MEM[i], WB[i], COM[i]);
				break;
			case ti_Sd: //Store case
				printf("\t\t\t%d\t%d\t%d\t%d\t%d\n", IS[i], EX[i], MEM[i], WB[i], COM[i]);
				break;	
			case ti_Beq: //Branch if equal case
				printf("\t\t\t%d\t%d\t%d\t%d\t%d\n", IS[i], EX[i], MEM[i], WB[i], COM[i]);
				break;		
			case ti_Bne: //Branch if not equal case
				printf("\t\t\t%d\t%d\t%d\t%d\t%d\n", IS[i], EX[i], MEM[i], WB[i], COM[i]);
				break;		
			case ti_Add: //Integer addition case
				printf("\t\t\t%d\t%d\tN/A\t%d\t%d\n", IS[i], EX[i], WB[i], COM[i]);
				break;		
			case ti_Addd: //Floating point addition case
				printf("\t\t\t%d\t%d\tN/A\t%d\t%d\n", IS[i], EX[i], WB[i], COM[i]);
				break;		
			case ti_Addi: //Integer Immediate addition case
				printf("\t\t\t%d\t%d\tN/A\t%d\t%d\n", IS[i], EX[i], WB[i], COM[i]);
				break;		
			case ti_Sub: //Integer subtraction
				printf("\t\t\t%d\t%d\tN/A\t%d\t%d\n", IS[i], EX[i], WB[i], COM[i]);
				break;		
			case ti_Subd: //Floating point subtraction
				printf("\t\t\t%d\t%d\tN/A\t%d\t%d\n", IS[i], EX[i], WB[i], COM[i]);
				break;		
			case ti_Multd: //Floating point multiplication
				printf("\t\t\t%d\t%d\tN/A\t%d\t%d\n", IS[i], EX[i], WB[i], COM[i]);
				break;		
		} //end switch
	}
}

uint32_t find_ROB_Head(struct ROB_entry *reOrder, uint32_t ROB_Entries) {
	uint32_t tag = 0;
	int i;
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
					reOrder[i].destReg[0] = forResStat->Ra[0];
					reOrder[i].destReg[1] = forResStat->Ra[1];
					reOrder[i].destReg[2] = forResStat->Ra[2];
					reOrder[i].destReg[3] = forResStat->Ra[3];
					break;				
				case ti_Addi: //Integer Immediate addition case
					reOrder[i].destReg[0] = forResStat->Rt[0];
					reOrder[i].destReg[1] = forResStat->Rt[1];
					reOrder[i].destReg[2] = forResStat->Rt[2];
					reOrder[i].destReg[3] = forResStat->Rt[3];
					break;		
				case ti_Sub: //Integer subtraction
					reOrder[i].destReg[0] = forResStat->Ra[0];
					reOrder[i].destReg[1] = forResStat->Ra[1];
					reOrder[i].destReg[2] = forResStat->Ra[2];
					reOrder[i].destReg[3] = forResStat->Ra[3];
					break;		
			}
			int rd = regLookup(reOrder[i].destReg);
			rat_Table[rd].rType = 1;
			rat_Table[rd].iOrf = 0;
			rat_Table[rd].irNumber = i;
			return tag;
		}
		if((i == ROB_Entries - 1) && (robHead != 0)) {
			i = -1;
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
			reOrder[i].destReg[0] = forResStat->Fa[0];
			reOrder[i].destReg[1] = forResStat->Fa[1];
			reOrder[i].destReg[2] = forResStat->Fa[2];
			reOrder[i].destReg[3] = forResStat->Fa[3];
			int rd = regLookup(reOrder[i].destReg);
			rat_Table[rd].rType = 1;
			rat_Table[rd].iOrf = 1;
			rat_Table[rd].frNumber = i;
			return tag;
		}
		if((i == ROB_Entries - 1) && (robHead != 0)) {
			i = -1;
			decVal = 1;
		}
	}
	return tag;
}

uint32_t update_int_ROB_tag(struct ROB_entry *reOrder, uint32_t ROB_Entries, struct RAT_entry *rat_Table, struct RS_entry *iRS, uint32_t ent) {
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
			reOrder[i].type = iRS[ent].type;
			reOrder[i].address = iRS[ent].address;
			reOrder[i].intVal = -1;
			reOrder[i].finOp = 0;
			reOrder[i].destReg[0] = iRS[ent].destReg[0];
			reOrder[i].destReg[1] = iRS[ent].destReg[1];
			reOrder[i].destReg[2] = iRS[ent].destReg[2];
			reOrder[i].destReg[3] = iRS[ent].destReg[3];
			int rd = regLookup(reOrder[i].destReg);
			rat_Table[rd].rType = 1;
			rat_Table[rd].iOrf = 0;
			rat_Table[rd].irNumber = i;
			return tag;
		}
		if((i == ROB_Entries - 1) && (robHead != 0)) {
			i = -1;
			decVal = 1;
		}
	}
	return tag;
}

uint32_t update_float_ROB_tag(struct ROB_entry *reOrder, uint32_t ROB_Entries, struct RAT_entry *rat_Table, struct RS_entry *fRS, uint32_t ent, struct LSQ_entry *lsq_Table, unsigned char lsq_or_rs) {
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
			reOrder[i].floatVal = -1;
			reOrder[i].finOp = 0;
			int rd = regLookup(reOrder[i].destReg);
			if(lsq_or_rs == 0) {
				reOrder[i].type = fRS[ent].type;
				reOrder[i].address = fRS[ent].address;
				reOrder[i].destReg[0] = fRS[ent].destReg[0];
				reOrder[i].destReg[1] = fRS[ent].destReg[1];
				reOrder[i].destReg[2] = fRS[ent].destReg[2];
				reOrder[i].destReg[3] = fRS[ent].destReg[3];
				rat_Table[rd].rType = 1;
			}else {
				reOrder[i].type = lsq_Table[ent].type;
				reOrder[i].address = lsq_Table[ent].address;
				reOrder[i].destReg[0] = lsq_Table[ent].Fa[0];
				reOrder[i].destReg[1] = lsq_Table[ent].Fa[1];
				reOrder[i].destReg[2] = lsq_Table[ent].Fa[2];
				reOrder[i].destReg[3] = lsq_Table[ent].Fa[3];
				if(lsq_Table[ent].type == ti_Ld){
					rat_Table[rd].rType = 1;	
				}else {
					rat_Table[rd].rType = 0;	
				}			
			}
			rat_Table[rd].rType = 1;
			rat_Table[rd].iOrf = 1;
			rat_Table[rd].frNumber = i;
			return tag;
		}
		if((i == ROB_Entries - 1) && (robHead != 0)) {
			i = -1;
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
		if(rat_Table[rd].iOrf == 0) { //if contains int entry and an int is accessing it, point to the right ROB entry
			tag = rat_Table[rd].irNumber;
		}else { //if contains float entry and a float is accessing it, point to the right ROB entry
			tag = rat_Table[rd].frNumber;
		}
	}
	return tag;
}

uint32_t get_int_RAT_Value(char *fd, struct int_Reg *iR) {
	int rd = regLookup(fd);
	uint32_t retVal = iR->R_num[rd];
	return retVal;
}

float get_float_RAT_Value(char *fd, struct float_Reg *fR) {
	int rd = regLookup(fd);
	float retVal = fR->F_num[rd];
	return retVal;	
}

unsigned char isROBFull(struct ROB_entry *reOrder, uint32_t ROB_Entries) {
	unsigned char isFull = 1; //Assume there are no free spots to begin with
	int i;
	for(i = 0; i < ROB_Entries; i++) {
		if(reOrder[i].type == 10) {
			isFull = 0;
			break;
		}
	}
	return isFull;
}

void iRSFill(struct RS_entry *iRS, struct ROB_entry *reOrder, struct int_Reg *iR, struct RAT_entry *int_rat_Table, int i, struct instruction *forResStat, uint32_t ROB_Entries, uint32_t maxRB) {
	iRS[i].address = forResStat->address;
	iRS[i].type = forResStat->type;
	iRS[i].isBusy = 3;
	switch(forResStat->type) {		
		case ti_Add: //Integer addition case
			iRS[i].tag1 = get_RAT_tag(int_rat_Table, forResStat->Rs);
			if(iRS[i].tag1 == -1) {
				iRS[i].iVal1 = get_int_RAT_Value(forResStat->Rs, iR);
			}else {
				if(reOrder[iRS[i].tag1].finOp == 1 || reOrder[iRS[i].tag1].finOp == 2) {
					iRS[i].tag1 = -1;
					iRS[i].iVal1 = reOrder[iRS[i].tag1].intVal;
				}				
			}
			iRS[i].tag2 = get_RAT_tag(int_rat_Table, forResStat->Rt);
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
			iRS[i].tag1 = get_RAT_tag(int_rat_Table, forResStat->Rs);
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
			iRS[i].tag1 = get_RAT_tag(int_rat_Table, forResStat->Rs);
			if(iRS[i].tag1 == -1) {
				iRS[i].iVal1 = get_int_RAT_Value(forResStat->Rs, iR);
			}else {
				if(reOrder[iRS[i].tag1].finOp == 1 || reOrder[iRS[i].tag1].finOp == 2) {
					iRS[i].tag1 = -1;
					iRS[i].iVal1 = reOrder[iRS[i].tag1].intVal;
				}				
			}
			iRS[i].tag2 = get_RAT_tag(int_rat_Table, forResStat->Rt);
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
	if(isROBFull(reOrder, ROB_Entries) == 0) {
		iRS[i].dst_tag = get_int_ROB_tag(reOrder, ROB_Entries, forResStat, int_rat_Table);
	}else {
		iRS[i].isBusy = 4;
		switch(forResStat->type) {		
			case ti_Add: //Integer addition case
				iRS[i].destReg[0] = forResStat->Ra[0];
				iRS[i].destReg[1] = forResStat->Ra[1];
				iRS[i].destReg[2] = forResStat->Ra[2];
				iRS[i].destReg[3] = forResStat->Ra[3];
				break;				
			case ti_Addi: //Integer Immediate addition case
				iRS[i].destReg[0] = forResStat->Rt[0];
				iRS[i].destReg[1] = forResStat->Rt[1];
				iRS[i].destReg[2] = forResStat->Rt[2];
				iRS[i].destReg[3] = forResStat->Rt[3];
				break;		
			case ti_Sub: //Integer subtraction
				iRS[i].destReg[0] = forResStat->Ra[0];
				iRS[i].destReg[1] = forResStat->Ra[1];
				iRS[i].destReg[2] = forResStat->Ra[2];
				iRS[i].destReg[3] = forResStat->Ra[3];
				break;		
		}		
	}
}

void fARSFill(struct RS_entry *fARS, struct ROB_entry *reOrder, struct float_Reg *fR, struct RAT_entry *float_rat_Table, int i, struct instruction *forResStat, uint32_t ROB_Entries, uint32_t MAX_ROB) {
	fARS[i].address = forResStat->address;
	fARS[i].type = forResStat->type;
	fARS[i].isBusy = 3;
	switch(forResStat->type) {		
		case ti_Addd: //Floating point addition case
			fARS[i].tag1 = get_RAT_tag(float_rat_Table, forResStat->Fs);
			if(fARS[i].tag1 == -1) {
				fARS[i].fVal1 = get_float_RAT_Value(forResStat->Fs, fR);
			}else {
				if(reOrder[fARS[i].tag1].finOp == 1 || reOrder[fARS[i].tag1].finOp == 2) {
					fARS[i].tag1 = -1;
					fARS[i].fVal1 = reOrder[fARS[i].tag1].floatVal;
				}				
			}
			fARS[i].tag2 = get_RAT_tag(float_rat_Table, forResStat->Ft);
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
			fARS[i].tag1 = get_RAT_tag(float_rat_Table, forResStat->Fs);
			if(fARS[i].tag1 == -1) {
				fARS[i].fVal1 = get_float_RAT_Value(forResStat->Fs, fR);
			}else {
				if(reOrder[fARS[i].tag1].finOp == 1 || reOrder[fARS[i].tag1].finOp == 2) {
					fARS[i].tag1 = -1;
					fARS[i].fVal1 = reOrder[fARS[i].tag1].floatVal;
				}				
			}
			fARS[i].tag2 = get_RAT_tag(float_rat_Table, forResStat->Ft);
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
	if(isROBFull(reOrder, ROB_Entries) == 0) {
		fARS[i].dst_tag = get_float_ROB_tag(reOrder, ROB_Entries, forResStat, float_rat_Table);
	}else {
		fARS[i].isBusy = 4;
		fARS[i].destReg[0] = forResStat->Fa[0];
		fARS[i].destReg[1] = forResStat->Fa[1];
		fARS[i].destReg[2] = forResStat->Fa[2];
		fARS[i].destReg[3] = forResStat->Fa[3];
	}
}

void fMRSFill(struct RS_entry *fMRS, struct ROB_entry *reOrder, struct float_Reg *fR, struct RAT_entry *float_rat_Table, int i, struct instruction *forResStat, uint32_t ROB_Entries, uint32_t MAX_ROB) {
	fMRS[i].address = forResStat->address;
	fMRS[i].type = forResStat->type;
	fMRS[i].isBusy = 3;
	fMRS[i].tag1 = get_RAT_tag(float_rat_Table, forResStat->Fs);
	if(fMRS[i].tag1 == -1) {
		fMRS[i].fVal1 = get_float_RAT_Value(forResStat->Fs, fR);
	}else {
		if(reOrder[fMRS[i].tag1].finOp == 1 || reOrder[fMRS[i].tag1].finOp == 2) {
			fMRS[i].tag1 = -1;
			fMRS[i].fVal1 = reOrder[fMRS[i].tag1].floatVal;
		}				
	}
	fMRS[i].tag2 = get_RAT_tag(float_rat_Table, forResStat->Ft);
	if(fMRS[i].tag2 == -1) {
		fMRS[i].fVal2 = get_float_RAT_Value(forResStat->Ft, fR);
	}else {
		if(reOrder[fMRS[i].tag2].finOp == 1 || reOrder[fMRS[i].tag2].finOp == 2) {
			fMRS[i].tag2 = -1;
			fMRS[i].fVal2 = reOrder[fMRS[i].tag2].floatVal;
		}				
	}
	if(isROBFull(reOrder, ROB_Entries) == 0) {
		fMRS[i].dst_tag = get_float_ROB_tag(reOrder, ROB_Entries, forResStat, float_rat_Table);
	}else {
		fMRS[i].isBusy = 4;
		fMRS[i].destReg[0] = forResStat->Fa[0];
		fMRS[i].destReg[1] = forResStat->Fa[1];
		fMRS[i].destReg[2] = forResStat->Fa[2];
		fMRS[i].destReg[3] = forResStat->Fa[3];
	}
}

void lsqFill(struct LSQ_entry *lsq_Table, struct ROB_entry *reOrder, struct int_Reg *iR, struct float_Reg *fR, struct RAT_entry *int_rat_Table, struct RAT_entry *float_rat_Table, int i, struct instruction *forResStat, uint32_t ROB_Entries, uint32_t MAX_ROB) {
	lsq_Table[i].address = forResStat->address;
	lsq_Table[i].type = forResStat->type;
	lsq_Table[i].isBusy = 3;
	lsq_Table[i].forwardedMEMValue = -1;
	lsq_Table[i].Fa[0] = forResStat->Fa[0];
	lsq_Table[i].Fa[1] = forResStat->Fa[1];
	lsq_Table[i].Fa[2] = forResStat->Fa[2];
	lsq_Table[i].Fa[3] = forResStat->Fa[3];
	lsq_Table[i].offset = forResStat->offset;
	lsq_Table[i].tag = get_RAT_tag(int_rat_Table, forResStat->Ra);
	if(lsq_Table[i].tag == -1) {
		lsq_Table[i].memVal = lsq_Table[i].offset + get_int_RAT_Value(forResStat->Ra, iR);
	}else {
		if(reOrder[lsq_Table[i].tag].finOp == 1 || reOrder[lsq_Table[i].tag].finOp == 2) {
			lsq_Table[i].tag = -1;
			lsq_Table[i].memVal = lsq_Table[i].offset + reOrder[lsq_Table[i].tag].intVal;
		}
	}
	switch(forResStat->type) {
		case ti_Ld:
			lsq_Table[i].dst_tag = regLookup(forResStat->Fa);
			break;
		case ti_Sd:
			lsq_Table[i].dst_tag = get_RAT_tag(float_rat_Table, forResStat->Fa);
			if(lsq_Table[i].dst_tag == -1) {
				lsq_Table[i].dst_Val = get_float_RAT_Value(forResStat->Fa, fR);
			}else {
				if(reOrder[lsq_Table[i].dst_tag].finOp == 1 || reOrder[lsq_Table[i].dst_tag].finOp == 2) {
					lsq_Table[i].dst_tag = -1;
					lsq_Table[i].dst_Val = reOrder[lsq_Table[i].dst_tag].floatVal;
				}
			}
			break;
	}
	if(isROBFull(reOrder, ROB_Entries) == 0) {
		lsq_Table[i].ROB_tag = get_float_ROB_tag(reOrder, ROB_Entries, forResStat, float_rat_Table);
	}else {
		lsq_Table[i].isBusy = 4;
	}
}

uint32_t isCDBBufEmpty(struct CDB_buffer *buf, uint32_t iSize) {
	uint32_t isEmpty = -1; //not empty assumption
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

void clearBufEntry(struct CDB_buffer *cBuf, uint32_t iSize) {
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
		if(cBuf[i].isValid == 0) {
			if(cBuf[i+1].isValid == 0) {
				cBuf[i] = cBuf[i+1];
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

void broadCastCDBVal(struct CDB_buffer *cBuffer, struct RS_entry *iRS, struct RS_entry *fARS, struct RS_entry *fMRS, struct ROB_entry *reOrder, uint32_t iSize, struct LSQ_entry *lsq_Table, uint32_t lsq_Entries) {
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
		if(iRS[i].isBusy == 1 || iRS[i].isBusy == 4) {
			if(iRS[i].tag1 == cBuffer[0].cdbBuffer.dst_tag) {
				iRS[i].tag1 = -1;
				iRS[i].iVal1 = intVal;
				if(iRS[i].tag2 == -1) {
					iRS[i].isBusy = 3;
				}
			}else if(iRS[i].tag2 == cBuffer[0].cdbBuffer.dst_tag) {
				iRS[i].tag2 = -1;
				iRS[i].iVal2 = intVal;
				if(iRS[i].tag1 == -1) {
					iRS[i].isBusy = 3;
				}				
			}
		}
	}
	for(i = 0; i < iSize; i++) {
		if(fARS[i].isBusy == 1 || fARS[i].isBusy == 4) {
			if(fARS[i].tag1 == cBuffer[0].cdbBuffer.dst_tag) {
				fARS[i].tag1 = -1;
				fARS[i].fVal1 = floatVal;
				if(fARS[i].tag2 == -1) {
					fARS[i].isBusy = 3;
				}
			}else if(fARS[i].tag2 == cBuffer[0].cdbBuffer.dst_tag) {
				fARS[i].tag2 = -1;
				fARS[i].fVal2 = floatVal;
				if(fARS[i].tag1 == -1) {
					fARS[i].isBusy = 3;
				}				
			}
		}
	}
	for(i = 0; i < iSize; i++) {
		if(fMRS[i].isBusy == 1 || fMRS[i].isBusy == 4) {
			if(fMRS[i].tag1 == cBuffer[0].cdbBuffer.dst_tag) {
				fMRS[i].tag1 = -1;
				fMRS[i].fVal1 = floatVal;
				if(fMRS[i].tag2 == -1) {
					fMRS[i].isBusy = 3;
				}
			}else if(fMRS[i].tag2 == cBuffer[0].cdbBuffer.dst_tag) {
				fMRS[i].tag2 = -1;
				fMRS[i].fVal2 = floatVal;
				if(fMRS[i].tag1 == -1) {
					fMRS[i].isBusy = 3;
				}				
			}
		}
	}
	for(i = 0; i < lsq_Entries; i++) {
		if(lsq_Table[i].isBusy == 1 || lsq_Table[i].isBusy == 4) {
			if(lsq_Table[i].tag == cBuffer[0].cdbBuffer.dst_tag) {
				lsq_Table[i].tag = -1;
				lsq_Table[i].memVal = lsq_Table[i].offset + intVal;
				switch(lsq_Table[i].type) {
					case ti_Ld:
						lsq_Table[i].isBusy = 3;
						break;
					case ti_Sd:
						if(lsq_Table[i].dst_tag == -1) {
							lsq_Table[i].isBusy = 3;
						}
						break;
				}
			}
			if(lsq_Table[i].dst_tag == cBuffer[0].cdbBuffer.dst_tag) {
				lsq_Table[i].dst_tag = -1;
				lsq_Table[i].dst_Val = floatVal;
				if(lsq_Table[i].tag == -1) {
					lsq_Table[i].isBusy = 3;
				}
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

void clearLSQEntry(struct LSQ_entry *lsq_Table, uint32_t lsq_Entries) {
	int i;
	for(i = 0; i < lsq_Entries - 1; i++) {
		lsq_Table[i] = lsq_Table[i+1];
	}
	lsq_Table[lsq_Entries-1].forwardedMEMValue = -1;
	lsq_Table[lsq_Entries-1].address = 0;
	lsq_Table[lsq_Entries-1].type = 10;
	lsq_Table[lsq_Entries-1].dst_tag = 0;
	lsq_Table[lsq_Entries-1].dst_Val = 0;
	lsq_Table[lsq_Entries-1].tag = 0;
	lsq_Table[lsq_Entries-1].offset = 0;
	lsq_Table[lsq_Entries-1].ex_cyclesLeft = 0;
	lsq_Table[lsq_Entries-1].mem_cyclesLeft = 0;
	lsq_Table[lsq_Entries-1].Fa[0] = 0;
	lsq_Table[lsq_Entries-1].Fa[1] = 0;
	lsq_Table[lsq_Entries-1].Fa[2] = 0;
	lsq_Table[lsq_Entries-1].Fa[3] = 0;
	lsq_Table[lsq_Entries-1].isHead = 0;
	lsq_Table[lsq_Entries-1].isBusy = 0;
	lsq_Table[lsq_Entries-1].memVal = 0;
	lsq_Table[0].isHead = 1;
}

void broadCastLoad(struct RS_entry *iRS, struct RS_entry *fARS, struct RS_entry *fMRS, struct ROB_entry *reOrder, uint32_t iSize, struct LSQ_entry *lsq_Table, uint32_t entNum, uint32_t lsq_Entries) {
	uint32_t intVal = lsq_Table[entNum].memVal;
	float floatVal = lsq_Table[entNum].dst_Val;
	int i;
	for(i = 0; i < iSize; i++) {
		if(fARS[i].isBusy == 1 || fARS[i].isBusy == 4) {
			if(fARS[i].tag1 == lsq_Table[entNum].ROB_tag) {
				fARS[i].tag1 = -1;
				fARS[i].fVal1 = floatVal;
				if(fARS[i].tag2 == -1) {
					fARS[i].isBusy = 3;
				}
			}else if(fARS[i].tag2 == lsq_Table[entNum].ROB_tag) {
				fARS[i].tag2 = -1;
				fARS[i].fVal2 = floatVal;
				if(fARS[i].tag1 == -1) {
					fARS[i].isBusy = 3;
				}				
			}
		}
	}
	for(i = 0; i < iSize; i++) {
		if(fMRS[i].isBusy == 1 || fMRS[i].isBusy == 4) {
			if(fMRS[i].tag1 == lsq_Table[entNum].ROB_tag) {
				fMRS[i].tag1 = -1;
				fMRS[i].fVal1 = floatVal;
				if(fMRS[i].tag2 == -1) {
					fMRS[i].isBusy = 3;
				}
			}else if(fMRS[i].tag2 == lsq_Table[entNum].ROB_tag) {
				fMRS[i].tag2 = -1;
				fMRS[i].fVal2 = floatVal;
				if(fMRS[i].tag1 == -1) {
					fMRS[i].isBusy = 3;
				}				
			}
		}
	}
	for(i = 0; i < lsq_Entries; i++) {
		if(lsq_Table[i].isBusy == 1 || lsq_Table[i].isBusy == 4) {
			if(lsq_Table[i].dst_tag == lsq_Table[entNum].ROB_tag) {
				lsq_Table[i].dst_tag = -1;
				lsq_Table[i].dst_Val = floatVal;
				if(lsq_Table[i].tag == -1) {
					lsq_Table[i].isBusy = 3;
				}
			}	
		}			
	}
	reOrder[lsq_Table[entNum].ROB_tag].finOp = 1;
	reOrder[lsq_Table[entNum].ROB_tag].intVal = intVal;
	reOrder[lsq_Table[entNum].ROB_tag].floatVal = floatVal;
}

void RS_Execute(float *mU, uint32_t *MEM, uint32_t mem_cycles, uint32_t exLSCycles, struct LSQ_entry *lsq_Table, uint32_t lsq_Entries, struct ROB_entry *reOrder, uint32_t ROB_Entries, struct RAT_entry *int_rat_Table, struct RAT_entry *float_rat_Table, struct RS_entry *iRS, struct RS_entry *fARS, struct RS_entry *fMRS, uint32_t iSize, struct CDB_buffer *iBuffer, struct CDB_buffer *fABuffer, struct CDB_buffer *fMBuffer, uint32_t cycle_num, uint32_t intCycles, uint32_t fACycles, uint32_t fMCycles, uint32_t *EX) {
	int i;
	for(i = 0; i < iSize; i++) {
		if(iRS[i].isBusy == 1) { //if occupied by instruction
			if(iRS[i].tag1 == -1 && iRS[i].tag2 == -1) { //if values in value fields have valid values and not tags, execute
				if(iRS[i].cyclesLeft == intCycles) {
					EX[iRS[i].address/4] = cycle_num;
				}
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
		if(iRS[i].isBusy == 3) {
			iRS[i].isBusy = 1;
		}
		if(iRS[i].isBusy == 4) {
			if(isROBFull(reOrder, ROB_Entries) == 0) {
				iRS[i].dst_tag = update_int_ROB_tag(reOrder, ROB_Entries, int_rat_Table, iRS, i);
				iRS[i].isBusy = 3;
			}
		}
	}
	
	for(i = 0; i < iSize; i++) {
		if(fARS[i].isBusy == 1) { //if occupied by instruction
			if(fARS[i].tag1 == -1 && fARS[i].tag2 == -1) { //if values in value fields have valid values and not tags, execute
				if(fARS[i].cyclesLeft == fACycles) {
					EX[fARS[i].address/4] = cycle_num;
				}
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
		if(fARS[i].isBusy == 3) {
			fARS[i].isBusy = 1;
		}
		if(fARS[i].isBusy == 4) {
			if(isROBFull(reOrder, ROB_Entries) == 0) {
				fARS[i].dst_tag = update_float_ROB_tag(reOrder, ROB_Entries, float_rat_Table, fARS, i, lsq_Table, 0);
				fARS[i].isBusy = 3;
			}
		}
	}
	
	for(i = 0; i < iSize; i++) {
		if(fMRS[i].isBusy == 1) { //if occupied by instruction
			if(fMRS[i].tag1 == -1 && fMRS[i].tag2 == -1) { //if values in value fields have valid values and not tags, execute
				if(fMRS[i].cyclesLeft == fMCycles) {
					EX[fMRS[i].address/4] = cycle_num;
				}
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
		if(fMRS[i].isBusy == 3) {
			fMRS[i].isBusy = 1;
		}
		if(fMRS[i].isBusy == 4) {
			if(isROBFull(reOrder, ROB_Entries) == 0) {
				fMRS[i].dst_tag = update_float_ROB_tag(reOrder, ROB_Entries, float_rat_Table, fMRS, i, lsq_Table, 0);
				fMRS[i].isBusy = 3;
			}
		}
	}
	
	for(i = 0; i < lsq_Entries; i++) {
		if(lsq_Table[i].isBusy == 1) {
			switch(lsq_Table[i].type) {
				case ti_Ld:
					if(lsq_Table[i].tag == -1) {
						if(lsq_Table[i].ex_cyclesLeft == exLSCycles) {
							EX[lsq_Table[i].address/4] = cycle_num;
						}
						if(lsq_Table[i].ex_cyclesLeft != 0) {
							lsq_Table[i].ex_cyclesLeft = lsq_Table[i].ex_cyclesLeft - 1;
						}
						if(lsq_Table[i].ex_cyclesLeft == 0 && lsq_Table[i].isHead == 1) {
							if(EX[lsq_Table[i].address/4] != cycle_num) {
								if(lsq_Table[i].mem_cyclesLeft == mem_cycles) {
									MEM[lsq_Table[i].address/4] = cycle_num;
								}
								if(lsq_Table[i].mem_cyclesLeft != 0) {
									lsq_Table[i].mem_cyclesLeft = lsq_Table[i].mem_cyclesLeft - 1;
								}
								if(lsq_Table[i].mem_cyclesLeft == 0) {
									if(lsq_Table[i].forwardedMEMValue != -1) {
										lsq_Table[i].dst_Val = lsq_Table[i].forwardedMEMValue;
									}else {
										lsq_Table[i].dst_Val = mU[lsq_Table[i].memVal/4];
									}
									broadCastLoad(iRS, fARS, fMRS, reOrder, iSize, lsq_Table, i, lsq_Entries);
									clearLSQEntry(lsq_Table, lsq_Entries);
								}
							}
						}
					}	
					break;
				case ti_Sd:
					if(lsq_Table[i].tag == -1 && lsq_Table[i].dst_tag == -1) {
						if(lsq_Table[i].ex_cyclesLeft == exLSCycles) {
							EX[lsq_Table[i].address/4] = cycle_num;
						}
						if(lsq_Table[i].ex_cyclesLeft != 0) {
							lsq_Table[i].ex_cyclesLeft = lsq_Table[i].ex_cyclesLeft - 1;
						}
						if(lsq_Table[i].ex_cyclesLeft == 0 && lsq_Table[i].isHead == 1) {
							if(EX[lsq_Table[i].address/4] != cycle_num) {
								int j;
								for(j = 1; j < lsq_Entries; j++) {
									if(lsq_Table[j].isBusy != 0) {
										if(lsq_Table[j].type == ti_Ld){
											if(lsq_Table[j].tag == -1) {
												if(lsq_Table[j].memVal == lsq_Table[i].memVal) {
													lsq_Table[j].forwardedMEMValue = lsq_Table[i].dst_Val;
												}
											}
										}
										if(lsq_Table[j].type == ti_Sd) {
											if(lsq_Table[j].tag == -1) {
												if(lsq_Table[j].memVal == lsq_Table[i].memVal) {
													break;
												}
											}
										}
									}
								}
							}
							if(reOrder[lsq_Table[i].ROB_tag].finOp != 2) {
								reOrder[lsq_Table[i].ROB_tag].finOp = 1;
								reOrder[lsq_Table[i].ROB_tag].intVal = lsq_Table[i].memVal;
								reOrder[lsq_Table[i].ROB_tag].floatVal = lsq_Table[i].dst_Val;
							}
						}
					}	
					break;
			}		
		}
		if(lsq_Table[i].isBusy == 3) {
			lsq_Table[i].isBusy = 1;
		}
		if(lsq_Table[i].isBusy == 4) {
			if(isROBFull(reOrder, ROB_Entries) == 0) {
				lsq_Table[i].ROB_tag = update_float_ROB_tag(reOrder, ROB_Entries, float_rat_Table, fMRS, i, lsq_Table, 1);
				fMRS[i].isBusy = 3;
			}
		}	
	}
}

void updateROB(struct LSQ_entry *lsq_Table, uint32_t lsq_Entries, uint32_t memcycles, uint32_t *MEM, struct ROB_entry *reOrder, uint32_t ROB_Entries, struct RAT_entry *int_rat_Table, struct RAT_entry *float_rat_Table, struct int_Reg *iR, struct float_Reg *fR, uint32_t *WB, uint32_t *COM, uint32_t cycle_number, float *mU) {
	int i;
	unsigned char int_float; //0 = int, 1 = float
	for(i = 0; i < ROB_Entries; i++) {
		if(reOrder[i].finOp == 2 && reOrder[i].isHead == 1) {
			COM[reOrder[i].address/4] = cycle_number;
			switch(reOrder[i].type) {
				case ti_Ld:
					int_float = 1;
					break;
				case ti_Sd:
					int_float = 2;
					break;
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
				if(regLookup(reOrder[i].destReg) == 0) {
					iR->R_num[regLookup(reOrder[i].destReg)] = 0;
				}else {
					iR->R_num[regLookup(reOrder[i].destReg)] = reOrder[i].intVal;
				}
				if(int_rat_Table[regLookup(reOrder[i].destReg)].iOrf == 0 && int_rat_Table[regLookup(reOrder[i].destReg)].irNumber == i) {
					int_rat_Table[regLookup(reOrder[i].destReg)].rType = 0;
				}					
			}else if(int_float == 1){
				fR->F_num[regLookup(reOrder[i].destReg)] = reOrder[i].floatVal;
				if(float_rat_Table[regLookup(reOrder[i].destReg)].iOrf == 1 && float_rat_Table[regLookup(reOrder[i].destReg)].frNumber == i) {
					float_rat_Table[regLookup(reOrder[i].destReg)].rType = 0;
				}
			}else {
				MEM[reOrder[i].address/4] = cycle_number + memcycles;
				mU[reOrder[i].intVal/4] = reOrder[i].floatVal;
				if(float_rat_Table[regLookup(reOrder[i].destReg)].iOrf == 1 && float_rat_Table[regLookup(reOrder[i].destReg)].frNumber == i) {
					float_rat_Table[regLookup(reOrder[i].destReg)].rType = 0;
				}
				clearLSQEntry(lsq_Table, lsq_Entries);
			}
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
			if(i == ROB_Entries - 1) {
				reOrder[0].isHead = 1;
			}else {
				reOrder[i+1].isHead = 1;
			}
			break;
		}
	}
	for(i = 0; i < ROB_Entries; i++) {
		if(reOrder[i].finOp == 1) {
			WB[reOrder[i].address/4] = cycle_number;
			reOrder[i].finOp = 2;
			break;
		}
	}
}

void print_ROB_Table(struct ROB_entry *reOrder, uint32_t ROB_Entries) {
	printf("address\ttype\tRegister\tValue\tfinOp\tisHead\n");
	int i;
	unsigned char int_float = 0;
	for(i = 0; i < ROB_Entries; i++) {
		switch(reOrder[i].type) {
			case ti_Ld:
				int_float = 1;
				break;
			case ti_Sd:
				int_float = 1;
				break;
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
			printf("%d\t%d\t%s\t\t%d\t%d\t%d\n",reOrder[i].address, reOrder[i].type, reOrder[i].destReg, reOrder[i].intVal, reOrder[i].finOp, reOrder[i].isHead);
		}else {
			printf("%d\t%d\t%s\t\t%.2f\t%d\t%d\n",reOrder[i].address, reOrder[i].type, reOrder[i].destReg, reOrder[i].floatVal, reOrder[i].finOp, reOrder[i].isHead);
		}
	}
}

void print_RS_Table(struct RS_entry *rsTable, uint32_t rEntries) {
	printf("address\ttype\tdst_tag\ttag1\ttag2\tVal1\tVal2\tisBusy\n");
	int i;
	unsigned char int_float = 0;
	for(i = 0; i < rEntries; i++) {
		switch(rsTable[i].type) {
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
			printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", rsTable[i].address, rsTable[i].type, rsTable[i].dst_tag, rsTable[i].tag1, rsTable[i].tag2, rsTable[i].iVal1, rsTable[i].iVal2, rsTable[i].isBusy);
		}else {
			printf("%d\t%d\t%d\t%d\t%d\t%.2f\t%.2f\t%d\n", rsTable[i].address, rsTable[i].type, rsTable[i].dst_tag, rsTable[i].tag1, rsTable[i].tag2, rsTable[i].fVal1, rsTable[i].fVal2, rsTable[i].isBusy);
		}
	}	
}

	uint32_t address; //address of instruction
	unsigned char type; //load or store
	uint32_t dst_tag; //destination tag if store
	float dst_Val; //ROB entry value if not pulled from ARF
	uint32_t tag; //ROB entry number for register containing store address or load address 
	uint32_t offset; //offset in memory
	uint32_t ex_cyclesLeft; //execution cycles left
	uint32_t mem_cyclesLeft; //memory cycles left
	char Fa[4]; //destination register if store
	unsigned char isHead; //is it at the front of the queue, 0 = no, 1 = yes
	unsigned char isBusy; //0 = available, 1 = busy, 2 = not initialized, 3 = just put in, don't execute yet
	uint32_t memVal;
	uint32_t ROB_tag;
	float forwardedMEMValue;

void print_LSQ_Queue(struct LSQ_entry *lsq_Table, uint32_t lsq_Entries) {
	printf("address\ttype\tdst_tag\tdst_Val\ttag\toffset\tex_cyclesLeft\tmem_cyclesLeft\tReg\tisHead\tisBusy\tmemVal\tROB_tag\tforwardedVal\n");
	int i;
	for(i = 0; i < lsq_Entries; i++) {
		printf("%d\t%d\t%d\t%.2f\t%d\t%d\t%d\t\t%d\t\t%s\t%d\t%d\t%d\t%d\t%.2f\n", lsq_Table[i].address, lsq_Table[i].type, lsq_Table[i].dst_tag, lsq_Table[i].dst_Val, lsq_Table[i].tag, lsq_Table[i].offset, lsq_Table[i].ex_cyclesLeft, lsq_Table[i].mem_cyclesLeft, lsq_Table[i].Fa, lsq_Table[i].isHead, lsq_Table[i].isBusy, lsq_Table[i].memVal, lsq_Table[i].ROB_tag, lsq_Table[i].forwardedMEMValue);
	}
}