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

struct RAT_entry { // Register Alias Table entry. Array of them makes the entire RAT
    int ROB_number;
} RAT_entry;

struct RS_entry { // Reservation Station entry. Array of them makes the entire RS
    int valid; // is this entry used
    enum opcode_type op;
    int ROB_number;
    int src1; // where is data operand 1 coming from (this will be an ROB entry number)
    int src2;
    void *src1_value; // the value of data operand 1
    void *src2_value;
    boolean src1_valid; // do we have valid data in value pointer
    boolean src2_valid;
} RS_entry;

struct ROB_entry { // ReOrder Buffer entry. Array of them makes the entire ROB
    int dest_reg;
    void *result;
    boolean ready; // ready to commit
} ROB_entry;

struct LSQ_entry { // Load/Store Queue entry. Array of them makes the entire LSQ
    boolean valid; // is this entry used
    boolean type; // is it a load or a store
    void *mem_address; // address this instruction is going to access
    int ROB_number;
    void *value;
    boolean value_valid; // is the data stored in value valid (we might not need this field)
} LSQ_entry;

struct CDB_buffer { // Buffer for storing data when Common Data Bus is busy
    int ROB_number;
    void *value;
    int arrival_cycle; // what cycle did this data arrive (used to resolve conflicts on the bus)
    boolean valid;
} CDB_buffer;

struct int_adder { // This is the entire int adder. It is single cycle so it just needs to store data for one cycle
    int value;
    boolean valid;
    int ROB_number;
} int_adder;

struct float_adder_stage { // Float adder is pipelined. Need an array of stages to sim pipeline
    float value;
    boolean valid;
    int ROB_number;
} float_adder_stage;

struct float_mult_stage { // Float mult is pipelined. Need an array of stages to sim pipeline
    float value;
    boolean valid;
    int ROB_number;
} float_mult_stage;

// Branch Target Buffer. Contains 8 entries as per the Branch Unit description
struct instruction BTB_array[8]; // Stores the instruction that is predicted to be fetched after a branch


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

	iR->R_num[ra] = iR->R_num[rs] + iR->R_num[rt];
} //end intAdd

/*Floating Point Addition*/
void FPAdd(struct instruction instr, struct float_Reg *fR) {
	int ra = regLookup(instr.Fa); //decode register number
	int rs = regLookup(instr.Fs); //decode register number
	int rt = regLookup(instr.Ft); //decode register number
	
	float temp = fR->F_num[rs] + fR->F_num[rt];
	if(temp > 3.402823466e+38F) { //cap to maximum floating point value
		temp = 3.402823466e+38F;
	}
	fR->F_num[ra] = temp;
} //end FPAdd

/*Integer Addition with immediate*/
void immAdd(struct instruction instr, struct int_Reg *iR) {
	int immed = instr.offset;
	int rs = regLookup(instr.Rs); //decode register number
	int rt = regLookup(instr.Rt); //decode register number
	
	iR->R_num[rt] = iR->R_num[rs] + immed;
} //end immAdd

/*Integer Subtraction*/
void intSub(struct instruction instr, struct int_Reg *iR) {
	int ra = regLookup(instr.Ra); //decode register number
	int rs = regLookup(instr.Rs); //decode register number
	int rt = regLookup(instr.Rt); //decode register number

	iR->R_num[ra] = iR->R_num[rs] - iR->R_num[rt];
} //end intSub

/*Floating Point subtraction*/
void FPSub(struct instruction instr, struct float_Reg *fR) {
	int ra = regLookup(instr.Fa); //decode register number
	int rs = regLookup(instr.Fs); //decode register number
	int rt = regLookup(instr.Ft); //decode register number
	
	float temp = fR->F_num[rs] - fR->F_num[rt];
	if(temp > 3.402823466e+38F) { //cap to maximum floating point value
		temp = 3.402823466e+38F;
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
		printf("\t\t\t%d\t%d\t%d\t%d\t%d\n", IS[i], EX[i], MEM[i], WB[i], COM[i]);
	}
}