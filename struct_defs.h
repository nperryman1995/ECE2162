/*Definitions*/
#define INPUT_SIZE 100 //amount of instructions that can be taken in by a file
#define MAX_ROB 1000 //maximum size of ROB Table

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

struct recover_Program { //struct that contains all important information for program. Used to reset program if a branch is mispredicted
	struct CDB_buffer iBuffer[INPUT_SIZE];
	struct CDB_buffer fABuffer[INPUT_SIZE];
	struct CDB_buffer fMBuffer[INPUT_SIZE];
	struct RS_entry iRS[INPUT_SIZE];
	struct RS_entry fARS[INPUT_SIZE];
	struct RS_entry fMRS[INPUT_SIZE];
	struct ROB_entry reOrder[MAX_ROB];
	struct RAT_entry int_rat_Table[MAX_ROB];
	struct RAT_entry float_rat_Table[MAX_ROB];
	struct LSQ_entry lsq_Table[INPUT_SIZE];
	struct int_Reg iR;
	struct float_Reg fR;
	uint32_t cycle_number;
	uint32_t pcAddr;
	unsigned char noFetch;
	uint32_t IS[INPUT_SIZE];
	uint32_t EX[INPUT_SIZE];
	uint32_t MEM[INPUT_SIZE];
	uint32_t WB[INPUT_SIZE];
	uint32_t COM[INPUT_SIZE];
	float memData[64];
};

struct branchHistory { //branch history values
	struct instruction br;
	uint32_t pcPlusFour;
	uint32_t predAddr;
	unsigned char isTaken;
	unsigned char isValid;
	uint32_t takenAddr;
};

struct BTB { //branch target buffer
	struct instruction BTB_array; // Stores the instruction that is predicted to be fetched after a branch
	uint32_t nextAddr; //stores the predicted address
};

//Save the program cdb buffers
void copyProgramCDB(struct recover_Program *RIP, struct CDB_buffer *iBuffer, struct CDB_buffer *fABuffer, struct CDB_buffer *fMBuffer) {
	int i;
	for(i = 0; i < INPUT_SIZE; i++) {
		RIP->iBuffer[i].cdbBuffer.address = iBuffer[i].cdbBuffer.address;
		RIP->iBuffer[i].cdbBuffer.type = iBuffer[i].cdbBuffer.type;
		RIP->iBuffer[i].cdbBuffer.dst_tag = iBuffer[i].cdbBuffer.dst_tag;
		RIP->iBuffer[i].cdbBuffer.tag1 = iBuffer[i].cdbBuffer.tag1;
		RIP->iBuffer[i].cdbBuffer.tag2 = iBuffer[i].cdbBuffer.tag2;
		RIP->iBuffer[i].cdbBuffer.iVal1 = iBuffer[i].cdbBuffer.iVal1;
		RIP->iBuffer[i].cdbBuffer.iVal2 = iBuffer[i].cdbBuffer.iVal2;
		RIP->iBuffer[i].cdbBuffer.fVal1 = iBuffer[i].cdbBuffer.fVal1;
		RIP->iBuffer[i].cdbBuffer.fVal2 = iBuffer[i].cdbBuffer.fVal2;
		RIP->iBuffer[i].cdbBuffer.isBusy = iBuffer[i].cdbBuffer.isBusy;
		RIP->iBuffer[i].cdbBuffer.cyclesLeft = iBuffer[i].cdbBuffer.cyclesLeft;
		RIP->iBuffer[i].cdbBuffer.destReg[0] = iBuffer[i].cdbBuffer.destReg[0];
		RIP->iBuffer[i].cdbBuffer.destReg[1] = iBuffer[i].cdbBuffer.destReg[1];
		RIP->iBuffer[i].cdbBuffer.destReg[2] = iBuffer[i].cdbBuffer.destReg[2];
		RIP->iBuffer[i].cdbBuffer.destReg[3] = iBuffer[i].cdbBuffer.destReg[3];
		RIP->iBuffer[i].arrival_cycle = iBuffer[i].arrival_cycle;
		RIP->iBuffer[i].isValid = iBuffer[i].isValid;
		RIP->fABuffer[i].cdbBuffer.address = fABuffer[i].cdbBuffer.address;
		RIP->fABuffer[i].cdbBuffer.type = fABuffer[i].cdbBuffer.type;
		RIP->fABuffer[i].cdbBuffer.dst_tag = fABuffer[i].cdbBuffer.dst_tag;
		RIP->fABuffer[i].cdbBuffer.tag1 = fABuffer[i].cdbBuffer.tag1;
		RIP->fABuffer[i].cdbBuffer.tag2 = fABuffer[i].cdbBuffer.tag2;
		RIP->fABuffer[i].cdbBuffer.iVal1 = fABuffer[i].cdbBuffer.iVal1;
		RIP->fABuffer[i].cdbBuffer.iVal2 = fABuffer[i].cdbBuffer.iVal2;
		RIP->fABuffer[i].cdbBuffer.fVal1 = fABuffer[i].cdbBuffer.fVal1;
		RIP->fABuffer[i].cdbBuffer.fVal2 = fABuffer[i].cdbBuffer.fVal2;
		RIP->fABuffer[i].cdbBuffer.isBusy = fABuffer[i].cdbBuffer.isBusy;
		RIP->fABuffer[i].cdbBuffer.cyclesLeft = fABuffer[i].cdbBuffer.cyclesLeft;
		RIP->fABuffer[i].cdbBuffer.destReg[0] = fABuffer[i].cdbBuffer.destReg[0];
		RIP->fABuffer[i].cdbBuffer.destReg[1] = fABuffer[i].cdbBuffer.destReg[1];
		RIP->fABuffer[i].cdbBuffer.destReg[2] = fABuffer[i].cdbBuffer.destReg[2];
		RIP->fABuffer[i].cdbBuffer.destReg[3] = fABuffer[i].cdbBuffer.destReg[3];
		RIP->fABuffer[i].arrival_cycle = fABuffer[i].arrival_cycle;
		RIP->fABuffer[i].isValid = fABuffer[i].isValid;
		RIP->fMBuffer[i].cdbBuffer.address = fMBuffer[i].cdbBuffer.address;
		RIP->fMBuffer[i].cdbBuffer.type = fMBuffer[i].cdbBuffer.type;
		RIP->fMBuffer[i].cdbBuffer.dst_tag = fMBuffer[i].cdbBuffer.dst_tag;
		RIP->fMBuffer[i].cdbBuffer.tag1 = fMBuffer[i].cdbBuffer.tag1;
		RIP->fMBuffer[i].cdbBuffer.tag2 = fMBuffer[i].cdbBuffer.tag2;
		RIP->fMBuffer[i].cdbBuffer.iVal1 = fMBuffer[i].cdbBuffer.iVal1;
		RIP->fMBuffer[i].cdbBuffer.iVal2 = fMBuffer[i].cdbBuffer.iVal2;
		RIP->fMBuffer[i].cdbBuffer.fVal1 = fMBuffer[i].cdbBuffer.fVal1;
		RIP->fMBuffer[i].cdbBuffer.fVal2 = fMBuffer[i].cdbBuffer.fVal2;
		RIP->fMBuffer[i].cdbBuffer.isBusy = fMBuffer[i].cdbBuffer.isBusy;
		RIP->fMBuffer[i].cdbBuffer.cyclesLeft = fMBuffer[i].cdbBuffer.cyclesLeft;
		RIP->fMBuffer[i].cdbBuffer.destReg[0] = fMBuffer[i].cdbBuffer.destReg[0];
		RIP->fMBuffer[i].cdbBuffer.destReg[1] = fMBuffer[i].cdbBuffer.destReg[1];
		RIP->fMBuffer[i].cdbBuffer.destReg[2] = fMBuffer[i].cdbBuffer.destReg[2];
		RIP->fMBuffer[i].cdbBuffer.destReg[3] = fMBuffer[i].cdbBuffer.destReg[3];
		RIP->fMBuffer[i].arrival_cycle = fMBuffer[i].arrival_cycle;
		RIP->fMBuffer[i].isValid = fMBuffer[i].isValid;		
	}
}

//Save the program reservation station entries
void copyProgramRS(struct recover_Program *RIP, struct RS_entry *iRS, struct RS_entry *fARS, struct RS_entry *fMRS) {
	int i;
	for(i = 0; i < INPUT_SIZE; i++) {
		RIP->iRS[i].address = iRS[i].address;
		RIP->iRS[i].type = iRS[i].type;
		RIP->iRS[i].dst_tag = iRS[i].dst_tag;
		RIP->iRS[i].tag1 = iRS[i].tag1;
		RIP->iRS[i].tag2 = iRS[i].tag2;
		RIP->iRS[i].iVal1 = iRS[i].iVal1;
		RIP->iRS[i].iVal2 = iRS[i].iVal2;
		RIP->iRS[i].fVal1 = iRS[i].fVal1;
		RIP->iRS[i].fVal2 = iRS[i].fVal2;
		RIP->iRS[i].isBusy = iRS[i].isBusy;
		RIP->iRS[i].cyclesLeft = iRS[i].cyclesLeft;
		RIP->iRS[i].destReg[0] = iRS[i].destReg[0];
		RIP->iRS[i].destReg[1] = iRS[i].destReg[1];
		RIP->iRS[i].destReg[2] = iRS[i].destReg[2];
		RIP->iRS[i].destReg[3] = iRS[i].destReg[3];
		RIP->fARS[i].address = fARS[i].address;
		RIP->fARS[i].type = fARS[i].type;
		RIP->fARS[i].dst_tag = fARS[i].dst_tag;
		RIP->fARS[i].tag1 = fARS[i].tag1;
		RIP->fARS[i].tag2 = fARS[i].tag2;
		RIP->fARS[i].iVal1 = fARS[i].iVal1;
		RIP->fARS[i].iVal2 = fARS[i].iVal2;
		RIP->fARS[i].fVal1 = fARS[i].fVal1;
		RIP->fARS[i].fVal2 = fARS[i].fVal2;
		RIP->fARS[i].isBusy = fARS[i].isBusy;
		RIP->fARS[i].cyclesLeft = fARS[i].cyclesLeft;
		RIP->fARS[i].destReg[0] = fARS[i].destReg[0];
		RIP->fARS[i].destReg[1] = fARS[i].destReg[1];
		RIP->fARS[i].destReg[2] = fARS[i].destReg[2];
		RIP->fARS[i].destReg[3] = fARS[i].destReg[3];
		RIP->fMRS[i].address = fMRS[i].address;
		RIP->fMRS[i].type = fMRS[i].type;
		RIP->fMRS[i].dst_tag = fMRS[i].dst_tag;
		RIP->fMRS[i].tag1 = fMRS[i].tag1;
		RIP->fMRS[i].tag2 = fMRS[i].tag2;
		RIP->fMRS[i].iVal1 = fMRS[i].iVal1;
		RIP->fMRS[i].iVal2 = fMRS[i].iVal2;
		RIP->fMRS[i].fVal1 = fMRS[i].fVal1;
		RIP->fMRS[i].fVal2 = fMRS[i].fVal2;
		RIP->fMRS[i].isBusy = fMRS[i].isBusy;
		RIP->fMRS[i].cyclesLeft = fMRS[i].cyclesLeft;
		RIP->fMRS[i].destReg[0] = fMRS[i].destReg[0];
		RIP->fMRS[i].destReg[1] = fMRS[i].destReg[1];
		RIP->fMRS[i].destReg[2] = fMRS[i].destReg[2];
		RIP->fMRS[i].destReg[3] = fMRS[i].destReg[3];
	}
}

//Save the program load/store queue
void copyProgramLSQ(struct recover_Program *RIP, struct LSQ_entry *lsq_Table) {
	int i;
	for(i = 0; i < INPUT_SIZE; i++) {
		RIP->lsq_Table[i].address = lsq_Table[i].address;
		RIP->lsq_Table[i].type = lsq_Table[i].type;
		RIP->lsq_Table[i].dst_tag = lsq_Table[i].dst_tag;
		RIP->lsq_Table[i].dst_Val = lsq_Table[i].dst_Val;
		RIP->lsq_Table[i].tag = lsq_Table[i].tag;
		RIP->lsq_Table[i].offset = lsq_Table[i].offset;
		RIP->lsq_Table[i].ex_cyclesLeft = lsq_Table[i].ex_cyclesLeft;
		RIP->lsq_Table[i].mem_cyclesLeft = lsq_Table[i].mem_cyclesLeft;
		RIP->lsq_Table[i].Fa[0] = lsq_Table[i].Fa[0];
		RIP->lsq_Table[i].Fa[1] = lsq_Table[i].Fa[1];
		RIP->lsq_Table[i].Fa[2] = lsq_Table[i].Fa[2];
		RIP->lsq_Table[i].Fa[3] = lsq_Table[i].Fa[3];
		RIP->lsq_Table[i].isHead = lsq_Table[i].isHead;
		RIP->lsq_Table[i].isBusy = lsq_Table[i].isBusy;
		RIP->lsq_Table[i].memVal = lsq_Table[i].memVal;
		RIP->lsq_Table[i].ROB_tag = lsq_Table[i].ROB_tag;
		RIP->lsq_Table[i].forwardedMEMValue = lsq_Table[i].forwardedMEMValue;
	}
}

//Save the programs RAT entries and ROB table
void copyProgramRATROB(struct recover_Program *RIP, struct RAT_entry *int_rat_Table, struct RAT_entry *float_rat_Table, struct ROB_entry *reOrder) {
	int i;
	for(i = 0; i < MAX_ROB; i++) {
		RIP->int_rat_Table[i].rType = int_rat_Table[i].rType;
		RIP->int_rat_Table[i].iOrf = int_rat_Table[i].iOrf;
		RIP->int_rat_Table[i].irNumber = int_rat_Table[i].irNumber;
		RIP->int_rat_Table[i].frNumber = int_rat_Table[i].frNumber;
		RIP->float_rat_Table[i].rType = float_rat_Table[i].rType;
		RIP->float_rat_Table[i].iOrf = float_rat_Table[i].iOrf;
		RIP->float_rat_Table[i].irNumber = float_rat_Table[i].irNumber;
		RIP->float_rat_Table[i].frNumber = float_rat_Table[i].frNumber;
		RIP->reOrder[i].address = reOrder[i].address;
		RIP->reOrder[i].type = reOrder[i].type;
		RIP->reOrder[i].destReg[0] = reOrder[i].destReg[0];
		RIP->reOrder[i].destReg[1] = reOrder[i].destReg[1];
		RIP->reOrder[i].destReg[2] = reOrder[i].destReg[2];
		RIP->reOrder[i].destReg[3] = reOrder[i].destReg[3];
		RIP->reOrder[i].intVal = reOrder[i].intVal;
		RIP->reOrder[i].floatVal = reOrder[i].floatVal;
		RIP->reOrder[i].finOp = reOrder[i].finOp;
		RIP->reOrder[i].isHead = reOrder[i].isHead;
	}
}

//Save the program integer registers and floating point registers
void copyProgramREG(struct recover_Program *RIP, struct int_Reg iR, struct float_Reg fR) {
	int i;
	for(i = 0; i < 32; i++) {
		RIP->iR.R_num[i] = iR.R_num[i];
		RIP->iR.canWrite[i] = iR.canWrite[i];
		RIP->fR.F_num[i] = fR.F_num[i];
	}
}

//Save the program cycle number, pc address, and noFetch flag
void copyProgramINT(struct recover_Program *RIP, uint32_t cycle, uint32_t addr, unsigned char netch) {
	RIP->cycle_number = cycle;
	RIP->pcAddr = addr;
	RIP->noFetch = netch;
}

//Save the program memory values
void copyProgramMEM(struct recover_Program *RIP, float *mU) {
	int i;
	for(i = 0; i < 64; i++) {
		RIP->memData[i] = mU[i];
	}
}

//Save the program cycle number Tables, mainly for program output
void copyProgramTAB(struct recover_Program *RIP, uint32_t *IS, uint32_t *EX, uint32_t *MEM, uint32_t *WB, uint32_t *COM) {
	int i;
	for(i = 0; i < INPUT_SIZE; i++) {
		RIP->IS[i] = IS[i];
		RIP->EX[i] = EX[i];
		RIP->MEM[i] = MEM[i];
		RIP->WB[i] = WB[i];
		RIP->COM[i] = COM[i];
	}
}

//Reset the cdb buffers to a previously saved state
void resetProgramCDB(struct recover_Program *RIP, struct CDB_buffer *iBuffer, struct CDB_buffer *fABuffer, struct CDB_buffer *fMBuffer) {
	int i;
	for(i = 0; i < INPUT_SIZE; i++) {
		iBuffer[i].cdbBuffer.address = RIP->iBuffer[i].cdbBuffer.address;
		iBuffer[i].cdbBuffer.type = RIP->iBuffer[i].cdbBuffer.type;
		iBuffer[i].cdbBuffer.dst_tag = RIP->iBuffer[i].cdbBuffer.dst_tag;
		iBuffer[i].cdbBuffer.tag1 = RIP->iBuffer[i].cdbBuffer.tag1;
		iBuffer[i].cdbBuffer.tag2 = RIP->iBuffer[i].cdbBuffer.tag2;
		iBuffer[i].cdbBuffer.iVal1 = RIP->iBuffer[i].cdbBuffer.iVal1;
		iBuffer[i].cdbBuffer.iVal2 = RIP->iBuffer[i].cdbBuffer.iVal2;
		iBuffer[i].cdbBuffer.fVal1 = RIP->iBuffer[i].cdbBuffer.fVal1;
		iBuffer[i].cdbBuffer.fVal2 = RIP->iBuffer[i].cdbBuffer.fVal2;
		iBuffer[i].cdbBuffer.isBusy = RIP->iBuffer[i].cdbBuffer.isBusy;
		iBuffer[i].cdbBuffer.cyclesLeft = RIP->iBuffer[i].cdbBuffer.cyclesLeft;
		iBuffer[i].cdbBuffer.destReg[0] = RIP->iBuffer[i].cdbBuffer.destReg[0];
		iBuffer[i].cdbBuffer.destReg[1] = RIP->iBuffer[i].cdbBuffer.destReg[1];
		iBuffer[i].cdbBuffer.destReg[2] = RIP->iBuffer[i].cdbBuffer.destReg[2];
		iBuffer[i].cdbBuffer.destReg[3] = RIP->iBuffer[i].cdbBuffer.destReg[3];
		iBuffer[i].arrival_cycle = RIP->iBuffer[i].arrival_cycle;
		iBuffer[i].isValid = RIP->iBuffer[i].isValid;
		fABuffer[i].cdbBuffer.address = RIP->fABuffer[i].cdbBuffer.address;
		fABuffer[i].cdbBuffer.type = RIP->fABuffer[i].cdbBuffer.type;
		fABuffer[i].cdbBuffer.dst_tag = RIP->fABuffer[i].cdbBuffer.dst_tag;
		fABuffer[i].cdbBuffer.tag1 = RIP->fABuffer[i].cdbBuffer.tag1;
		fABuffer[i].cdbBuffer.tag2 = RIP->fABuffer[i].cdbBuffer.tag2;
		fABuffer[i].cdbBuffer.iVal1 = RIP->fABuffer[i].cdbBuffer.iVal1;
		fABuffer[i].cdbBuffer.iVal2 = RIP->fABuffer[i].cdbBuffer.iVal2;
		fABuffer[i].cdbBuffer.fVal1 = RIP->fABuffer[i].cdbBuffer.fVal1;
		fABuffer[i].cdbBuffer.fVal2 = RIP->fABuffer[i].cdbBuffer.fVal2;
		fABuffer[i].cdbBuffer.isBusy = RIP->fABuffer[i].cdbBuffer.isBusy;
		fABuffer[i].cdbBuffer.cyclesLeft = RIP->fABuffer[i].cdbBuffer.cyclesLeft;
		fABuffer[i].cdbBuffer.destReg[0] = RIP->fABuffer[i].cdbBuffer.destReg[0];
		fABuffer[i].cdbBuffer.destReg[1] = RIP->fABuffer[i].cdbBuffer.destReg[1];
		fABuffer[i].cdbBuffer.destReg[2] = RIP->fABuffer[i].cdbBuffer.destReg[2];
		fABuffer[i].cdbBuffer.destReg[3] = RIP->fABuffer[i].cdbBuffer.destReg[3];
		fABuffer[i].arrival_cycle = RIP->fABuffer[i].arrival_cycle;
		fABuffer[i].isValid = RIP->fABuffer[i].isValid;
		fMBuffer[i].cdbBuffer.address = RIP->fMBuffer[i].cdbBuffer.address;
		fMBuffer[i].cdbBuffer.type = RIP->fMBuffer[i].cdbBuffer.type;
		fMBuffer[i].cdbBuffer.dst_tag = RIP->fMBuffer[i].cdbBuffer.dst_tag;
		fMBuffer[i].cdbBuffer.tag1 = RIP->fMBuffer[i].cdbBuffer.tag1;
		fMBuffer[i].cdbBuffer.tag2 = RIP->fMBuffer[i].cdbBuffer.tag2;
		fMBuffer[i].cdbBuffer.iVal1 = RIP->fMBuffer[i].cdbBuffer.iVal1;
		fMBuffer[i].cdbBuffer.iVal2 = RIP->fMBuffer[i].cdbBuffer.iVal2;
		fMBuffer[i].cdbBuffer.fVal1 = RIP->fMBuffer[i].cdbBuffer.fVal1;
		fMBuffer[i].cdbBuffer.fVal2 = RIP->fMBuffer[i].cdbBuffer.fVal2;
		fMBuffer[i].cdbBuffer.isBusy = RIP->fMBuffer[i].cdbBuffer.isBusy;
		fMBuffer[i].cdbBuffer.cyclesLeft = RIP->fMBuffer[i].cdbBuffer.cyclesLeft;
		fMBuffer[i].cdbBuffer.destReg[0] = RIP->fMBuffer[i].cdbBuffer.destReg[0];
		fMBuffer[i].cdbBuffer.destReg[1] = RIP->fMBuffer[i].cdbBuffer.destReg[1];
		fMBuffer[i].cdbBuffer.destReg[2] = RIP->fMBuffer[i].cdbBuffer.destReg[2];
		fMBuffer[i].cdbBuffer.destReg[3] = RIP->fMBuffer[i].cdbBuffer.destReg[3];
		fMBuffer[i].arrival_cycle = RIP->fMBuffer[i].arrival_cycle;
		fMBuffer[i].isValid = RIP->fMBuffer[i].isValid;		
	}
}

//Reset the reservation stations to a previously saved state
void resetProgramRS(struct recover_Program *RIP, struct RS_entry *iRS, struct RS_entry *fARS, struct RS_entry *fMRS) {
	int i;
	for(i = 0; i < INPUT_SIZE; i++) {
		iRS[i].address = RIP->iRS[i].address;
		iRS[i].type = RIP->iRS[i].type;
		iRS[i].dst_tag = RIP->iRS[i].dst_tag;
		iRS[i].tag1 = RIP->iRS[i].tag1;
		iRS[i].tag2 = RIP->iRS[i].tag2;
		iRS[i].iVal1 = RIP->iRS[i].iVal1;
		iRS[i].iVal2 = RIP->iRS[i].iVal2;
		iRS[i].fVal1 = RIP->iRS[i].fVal1;
		iRS[i].fVal2 = RIP->iRS[i].fVal2;
		iRS[i].isBusy = RIP->iRS[i].isBusy;
		iRS[i].cyclesLeft = RIP->iRS[i].cyclesLeft;
		iRS[i].destReg[0] = RIP->iRS[i].destReg[0];
		iRS[i].destReg[1] = RIP->iRS[i].destReg[1];
		iRS[i].destReg[2] = RIP->iRS[i].destReg[2];
		iRS[i].destReg[3] = RIP->iRS[i].destReg[3];
		fARS[i].address = RIP->fARS[i].address;
		fARS[i].type = RIP->fARS[i].type;
		fARS[i].dst_tag = RIP->fARS[i].dst_tag;
		fARS[i].tag1 = RIP->fARS[i].tag1;
		fARS[i].tag2 = RIP->fARS[i].tag2;
		fARS[i].iVal1 = RIP->fARS[i].iVal1;
		fARS[i].iVal2 = RIP->fARS[i].iVal2;
		fARS[i].fVal1 = RIP->fARS[i].fVal1;
		fARS[i].fVal2 = RIP->fARS[i].fVal2;
		fARS[i].isBusy = RIP->fARS[i].isBusy;
		fARS[i].cyclesLeft = RIP->fARS[i].cyclesLeft;
		fARS[i].destReg[0] = RIP->fARS[i].destReg[0];
		fARS[i].destReg[1] = RIP->fARS[i].destReg[1];
		fARS[i].destReg[2] = RIP->fARS[i].destReg[2];
		fARS[i].destReg[3] = RIP->fARS[i].destReg[3];
		fMRS[i].address = RIP->fMRS[i].address;
		fMRS[i].type = RIP->fMRS[i].type;
		fMRS[i].dst_tag = RIP->fMRS[i].dst_tag;
		fMRS[i].tag1 = RIP->fMRS[i].tag1;
		fMRS[i].tag2 = RIP->fMRS[i].tag2;
		fMRS[i].iVal1 = RIP->fMRS[i].iVal1;
		fMRS[i].iVal2 = RIP->fMRS[i].iVal2;
		fMRS[i].fVal1 = RIP->fMRS[i].fVal1;
		fMRS[i].fVal2 = RIP->fMRS[i].fVal2;
		fMRS[i].isBusy = RIP->fMRS[i].isBusy;
		fMRS[i].cyclesLeft = RIP->fMRS[i].cyclesLeft;
		fMRS[i].destReg[0] = RIP->fMRS[i].destReg[0];
		fMRS[i].destReg[1] = RIP->fMRS[i].destReg[1];
		fMRS[i].destReg[2] = RIP->fMRS[i].destReg[2];
		fMRS[i].destReg[3] = RIP->fMRS[i].destReg[3];
	}
}

//Reset the load/store queue to a previously saved state
void resetProgramLSQ(struct recover_Program *RIP, struct LSQ_entry *lsq_Table) {
	int i;
	for(i = 0; i < INPUT_SIZE; i++) {
		lsq_Table[i].address = RIP->lsq_Table[i].address;
		lsq_Table[i].type = RIP->lsq_Table[i].type;
		lsq_Table[i].dst_tag = RIP->lsq_Table[i].dst_tag;
		lsq_Table[i].dst_Val = RIP->lsq_Table[i].dst_Val;
		lsq_Table[i].tag = RIP->lsq_Table[i].tag;
		lsq_Table[i].offset = RIP->lsq_Table[i].offset;
		lsq_Table[i].ex_cyclesLeft = RIP->lsq_Table[i].ex_cyclesLeft;
		lsq_Table[i].mem_cyclesLeft = RIP->lsq_Table[i].mem_cyclesLeft;
		lsq_Table[i].Fa[0] = RIP->lsq_Table[i].Fa[0];
		lsq_Table[i].Fa[1] = RIP->lsq_Table[i].Fa[1];
		lsq_Table[i].Fa[2] = RIP->lsq_Table[i].Fa[2];
		lsq_Table[i].Fa[3] = RIP->lsq_Table[i].Fa[3];
		RIP->lsq_Table[i].isHead = lsq_Table[i].isHead = RIP->lsq_Table[i].isHead;
		lsq_Table[i].isBusy = RIP->lsq_Table[i].isBusy;
		lsq_Table[i].memVal = RIP->lsq_Table[i].memVal;
		lsq_Table[i].ROB_tag = RIP->lsq_Table[i].ROB_tag;
		lsq_Table[i].forwardedMEMValue = RIP->lsq_Table[i].forwardedMEMValue;
	}
}

//Reset the RAT Entries and ROB table to a previously saved state
void resetProgramRATROB(struct recover_Program *RIP, struct RAT_entry *int_rat_Table, struct RAT_entry *float_rat_Table, struct ROB_entry *reOrder) {
	int i;
	for(i = 0; i < MAX_ROB; i++) {
		int_rat_Table[i].rType = RIP->int_rat_Table[i].rType;
		int_rat_Table[i].iOrf = RIP->int_rat_Table[i].iOrf;
		int_rat_Table[i].irNumber = RIP->int_rat_Table[i].irNumber;
		int_rat_Table[i].frNumber = RIP->int_rat_Table[i].frNumber;
		float_rat_Table[i].rType = RIP->float_rat_Table[i].rType;
		float_rat_Table[i].iOrf = RIP->float_rat_Table[i].iOrf;
		float_rat_Table[i].irNumber = RIP->float_rat_Table[i].irNumber;
		float_rat_Table[i].frNumber = RIP->float_rat_Table[i].frNumber;
		reOrder[i].address = RIP->reOrder[i].address;
		reOrder[i].type = RIP->reOrder[i].type;
		reOrder[i].destReg[0] = RIP->reOrder[i].destReg[0];
		reOrder[i].destReg[1] = RIP->reOrder[i].destReg[1];
		reOrder[i].destReg[2] = RIP->reOrder[i].destReg[2];
		reOrder[i].destReg[3] = RIP->reOrder[i].destReg[3];
		reOrder[i].intVal = RIP->reOrder[i].intVal;
		reOrder[i].floatVal = RIP->reOrder[i].floatVal;
		reOrder[i].finOp = RIP->reOrder[i].finOp;
		reOrder[i].isHead = RIP->reOrder[i].isHead;
	}
}

//Reset the integer registers and floating point registers to a previously saved state
void resetProgramREG(struct recover_Program *RIP, struct int_Reg *iR, struct float_Reg *fR) {
	int i;
	for(i = 0; i < 32; i++) {
		iR->R_num[i] = RIP->iR.R_num[i];
		iR->canWrite[i]= RIP->iR.canWrite[i];
		fR->F_num[i]= RIP->fR.F_num[i];
	}
}

//Reset the program cycle number, pc address, and noFetch flag to a previously saved state
void resetProgramINT(struct recover_Program *RIP, uint32_t *cycle, uint32_t *addr, unsigned char *netch) {
	*cycle = RIP->cycle_number;
	*addr = RIP->pcAddr;
	*netch = RIP->noFetch;
}

//reset the program memory to a previously saved state
void resetProgramMEM(struct recover_Program *RIP, float *mU) {
	int i;
	for(i = 0; i < 64; i++) {
		mU[i] = RIP->memData[i];
	}
}

//Reset the program cycle number tables to a previously saved state
void resetProgramTAB(struct recover_Program *RIP, uint32_t *IS, uint32_t *EX, uint32_t *MEM, uint32_t *WB, uint32_t *COM) {
	int i;
	for(i = 0; i < INPUT_SIZE; i++) {
		IS[i] = RIP->IS[i];
		EX[i] = RIP->EX[i];
		MEM[i] = RIP->MEM[i];
		WB[i] = RIP->WB[i];
		COM[i] = RIP->COM[i];
	}
}

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
				printf("\t\t\t%d\t%d\tN/A\tN/A\tN/A\n", IS[i], EX[i]);
				break;		
			case ti_Bne: //Branch if not equal case
				printf("\t\t\t%d\t%d\tN/A\tN/A\tN/A\n", IS[i], EX[i]);
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

//Find the entry number of the ROB head (i.e. what instruction can commit next when ready)
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

//Find the entry of a free ROB entry to put a RS result into. Update the ROB entry with the reservation station info. For integer reservation station entries
uint32_t get_int_ROB_tag(struct ROB_entry *reOrder, uint32_t ROB_Entries, struct instruction *forResStat, struct RAT_entry *rat_Table) {
	uint32_t tag = 0;
	unsigned char decVal = 0;
	int i;
	uint32_t robHead = find_ROB_Head(reOrder, ROB_Entries); //find the head of the reorder buffer and start there
	for(i = robHead; i < ROB_Entries; i++) {
		if(i == robHead && decVal == 1) { //if at the head and have already wrapped around the table, finish
			break;
		}
		if(reOrder[i].type == 10) { //if entry is empty, update values
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
			int rd = regLookup(reOrder[i].destReg); //update the rat_table to point to this ROB entry for the destination reg
			rat_Table[rd].rType = 1;
			rat_Table[rd].iOrf = 0;
			rat_Table[rd].irNumber = i;
			return tag;
		}
		if((i == ROB_Entries - 1) && (robHead != 0)) { //wrap around to zero if at end and did not start at zero
			i = -1;
			decVal = 1;
		}
	}
	return tag;
}

//Find the entry of a free ROB entry to put a RS result into. Update the ROB entry with the reservation station info. For floating point reservation station entries
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

//If a previous attempt to find a free ROB entry failed, check again 
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

//If a previous attempt to find a free ROB entry failed, check again
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

//If the RAT points to the ARF, grab the register number. If it points to the ROB, grab the ROB entry number
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

//Get a value from the integer register files
uint32_t get_int_RAT_Value(char *fd, struct int_Reg *iR) {
	int rd = regLookup(fd);
	uint32_t retVal = iR->R_num[rd];
	return retVal;
}

//Get a value from the floating point register files
float get_float_RAT_Value(char *fd, struct float_Reg *fR) {
	int rd = regLookup(fd);
	float retVal = fR->F_num[rd];
	return retVal;	
}

//Check if the ROB is full or not
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

//Fill the integer adder reservation station with a branch entry
void iRSBranchFill(struct RS_entry *iRS, struct int_Reg *iR, int i, struct instruction *forResStat, struct RAT_entry *int_rat_Table, struct ROB_entry *reOrder) {
	iRS[i].address = forResStat->address;
	iRS[i].type = forResStat->type;
	iRS[i].isBusy = 3;
	iRS[i].tag1 = get_RAT_tag(int_rat_Table, forResStat->Rs);
	iRS[i].dst_tag = forResStat->offset;
	if(iRS[i].tag1 == -1) { //if first operand is in ARF, grab it. Else go to ROB entry
		iRS[i].iVal1 = get_int_RAT_Value(forResStat->Rs, iR);
	}else {
		if(reOrder[iRS[i].tag1].finOp == 1 || reOrder[iRS[i].tag1].finOp == 2) { //if ROB has a finished value, grab it, otherwise wait
			iRS[i].iVal1 = reOrder[iRS[i].tag1].intVal;
			iRS[i].tag1 = -1;
		}				
	}
	iRS[i].tag2 = get_RAT_tag(int_rat_Table, forResStat->Rt);
	if(iRS[i].tag2 == -1) {
		iRS[i].iVal2 = get_int_RAT_Value(forResStat->Rt, iR);
	}else {
		if(reOrder[iRS[i].tag2].finOp == 1 || reOrder[iRS[i].tag2].finOp == 2) {
			iRS[i].iVal2 = reOrder[iRS[i].tag2].intVal;
			iRS[i].tag2 = -1;
		}				
	}
}

//Fill the integer adder reservation station with an integer operation
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
					iRS[i].iVal1 = reOrder[iRS[i].tag1].intVal;
					iRS[i].tag1 = -1;
				}				
			}
			iRS[i].tag2 = get_RAT_tag(int_rat_Table, forResStat->Rt);
			if(iRS[i].tag2 == -1) {
				iRS[i].iVal2 = get_int_RAT_Value(forResStat->Rt, iR);
			}else {
				if(reOrder[iRS[i].tag2].finOp == 1 || reOrder[iRS[i].tag2].finOp == 2) {
					iRS[i].iVal2 = reOrder[iRS[i].tag2].intVal;
					iRS[i].tag2 = -1;
				}				
			}
			break;				
		case ti_Addi: //Integer Immediate addition case
			iRS[i].tag1 = get_RAT_tag(int_rat_Table, forResStat->Rs);
			if(iRS[i].tag1 == -1) {
				iRS[i].iVal1 = get_int_RAT_Value(forResStat->Rs, iR);
			}else {
				if(reOrder[iRS[i].tag1].finOp == 1 || reOrder[iRS[i].tag1].finOp == 2) {
					iRS[i].iVal1 = reOrder[iRS[i].tag1].intVal;
					iRS[i].tag1 = -1;
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
					iRS[i].iVal1 = reOrder[iRS[i].tag1].intVal;
					iRS[i].tag1 = -1;
				}				
			}
			iRS[i].tag2 = get_RAT_tag(int_rat_Table, forResStat->Rt);
			if(iRS[i].tag2 == -1) {
				iRS[i].iVal2 = get_int_RAT_Value(forResStat->Rt, iR);
			}else {
				if(reOrder[iRS[i].tag2].finOp == 1 || reOrder[iRS[i].tag2].finOp == 2) {
					iRS[i].iVal2 = reOrder[iRS[i].tag2].intVal;
					iRS[i].tag2 = -1;
				}				
			}		
			break;		
	}
	if(isROBFull(reOrder, ROB_Entries) == 0) { //if ROB is full, wait. Otherwise grab a ROB destination entry for the reservation station
		iRS[i].dst_tag = get_int_ROB_tag(reOrder, ROB_Entries, forResStat, int_rat_Table);
	}else {
		iRS[i].isBusy = 4;
		switch(forResStat->type) { //copy destination register if a wait is needed (instruction is discarded here and there needs to be some way to tell the ROB where to put its value)
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

//Fill the floating point adder reservation station with a floating point add or subtract
void fARSFill(struct RS_entry *fARS, struct ROB_entry *reOrder, struct float_Reg *fR, struct RAT_entry *float_rat_Table, int i, struct instruction *forResStat, uint32_t ROB_Entries, uint32_t mROB) {
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
					fARS[i].fVal1 = reOrder[fARS[i].tag1].floatVal;
					fARS[i].tag1 = -1;
				}				
			}
			fARS[i].tag2 = get_RAT_tag(float_rat_Table, forResStat->Ft);
			if(fARS[i].tag2 == -1) {
				fARS[i].fVal2 = get_float_RAT_Value(forResStat->Ft, fR);
			}else {
				if(reOrder[fARS[i].tag2].finOp == 1 || reOrder[fARS[i].tag2].finOp == 2) {
					fARS[i].fVal2 = reOrder[fARS[i].tag2].floatVal;
					fARS[i].tag2 = -1;
				}				
			}
			break;			
		case ti_Subd: //Floating point subtraction
			fARS[i].tag1 = get_RAT_tag(float_rat_Table, forResStat->Fs);
			if(fARS[i].tag1 == -1) {
				fARS[i].fVal1 = get_float_RAT_Value(forResStat->Fs, fR);
			}else {
				if(reOrder[fARS[i].tag1].finOp == 1 || reOrder[fARS[i].tag1].finOp == 2) {
					fARS[i].fVal1 = reOrder[fARS[i].tag1].floatVal;
					fARS[i].tag1 = -1;
				}				
			}
			fARS[i].tag2 = get_RAT_tag(float_rat_Table, forResStat->Ft);
			if(fARS[i].tag2 == -1) {
				fARS[i].fVal2 = get_float_RAT_Value(forResStat->Ft, fR);
			}else {
				if(reOrder[fARS[i].tag2].finOp == 1 || reOrder[fARS[i].tag2].finOp == 2) {
					fARS[i].fVal2 = reOrder[fARS[i].tag2].floatVal;
					fARS[i].tag2 = -1;
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

//File the floating point multiplication reservation station
void fMRSFill(struct RS_entry *fMRS, struct ROB_entry *reOrder, struct float_Reg *fR, struct RAT_entry *float_rat_Table, int i, struct instruction *forResStat, uint32_t ROB_Entries, uint32_t mROB) {
	fMRS[i].address = forResStat->address;
	fMRS[i].type = forResStat->type;
	fMRS[i].isBusy = 3;
	fMRS[i].tag1 = get_RAT_tag(float_rat_Table, forResStat->Fs);
	if(fMRS[i].tag1 == -1) {
		fMRS[i].fVal1 = get_float_RAT_Value(forResStat->Fs, fR);
	}else {
		if(reOrder[fMRS[i].tag1].finOp == 1 || reOrder[fMRS[i].tag1].finOp == 2) {
			fMRS[i].fVal1 = reOrder[fMRS[i].tag1].floatVal;
			fMRS[i].tag1 = -1;
		}				
	}
	fMRS[i].tag2 = get_RAT_tag(float_rat_Table, forResStat->Ft);
	if(fMRS[i].tag2 == -1) {
		fMRS[i].fVal2 = get_float_RAT_Value(forResStat->Ft, fR);
	}else {
		if(reOrder[fMRS[i].tag2].finOp == 1 || reOrder[fMRS[i].tag2].finOp == 2) {
			fMRS[i].fVal2 = reOrder[fMRS[i].tag2].floatVal;
			fMRS[i].tag2 = -1;
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

//Fill the load/store queue
void lsqFill(struct LSQ_entry *lsq_Table, struct ROB_entry *reOrder, struct int_Reg *iR, struct float_Reg *fR, struct RAT_entry *int_rat_Table, struct RAT_entry *float_rat_Table, int i, struct instruction *forResStat, uint32_t ROB_Entries, uint32_t mROB) {
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
	if(lsq_Table[i].tag == -1) { //grab integer register if in ARF, otherwise try and grab from Reorder buffer
		lsq_Table[i].memVal = lsq_Table[i].offset + get_int_RAT_Value(forResStat->Ra, iR);
	}else {
		if(reOrder[lsq_Table[i].tag].finOp == 1 || reOrder[lsq_Table[i].tag].finOp == 2) {
			lsq_Table[i].memVal = lsq_Table[i].offset + reOrder[lsq_Table[i].tag].intVal;
			lsq_Table[i].tag = -1;
		}
	}
	switch(forResStat->type) {
		case ti_Ld:
			lsq_Table[i].dst_tag = regLookup(forResStat->Fa); //a load only needs the destination register to load to
			break;
		case ti_Sd:
			lsq_Table[i].dst_tag = get_RAT_tag(float_rat_Table, forResStat->Fa); //store needs a value to load into memory. Attemp to grab it from the ARF or the ROB
			if(lsq_Table[i].dst_tag == -1) {
				lsq_Table[i].dst_Val = get_float_RAT_Value(forResStat->Fa, fR);
			}else {
				if(reOrder[lsq_Table[i].dst_tag].finOp == 1 || reOrder[lsq_Table[i].dst_tag].finOp == 2) {
					lsq_Table[i].dst_Val = reOrder[lsq_Table[i].dst_tag].floatVal;
					lsq_Table[i].dst_tag = -1;
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

//check if the specified cdb buffer is empty
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

//Put an entry into the cdb buffer 
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

//Clear a reservation station entry
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

//clear a cdb buffer entry, then shift it down to maintain the property that the first entry in the cdb buffer is the first in line to request access from the cdb
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

//Send the executed value to each reservation station, the load/store queue, and the ROB
void broadCastCDBVal(struct CDB_buffer *cBuffer, struct RS_entry *iRS, struct RS_entry *fARS, struct RS_entry *fMRS, struct ROB_entry *reOrder, uint32_t iSize, struct LSQ_entry *lsq_Table, uint32_t lsq_Entries) {
	uint32_t intVal = 0;
	float floatVal = 0;
	switch(cBuffer[0].cdbBuffer.type) { //Grab value to broadcast
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
	for(i = 0; i < iSize; i++) { //broadcast to integer adder reservation station. If something is waiting for it, update its value
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
	for(i = 0; i < iSize; i++) { //broadcast to floating point adder reservation station. If something is waiting for it, update its value
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
	for(i = 0; i < iSize; i++) { //broadcast to floating point multiplication reservation station. If something is waiting for it, update its value
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
	for(i = 0; i < lsq_Entries; i++) { //broadcast to load/store queue. If something is waiting for it, update its value
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
	reOrder[cBuffer[0].cdbBuffer.dst_tag].finOp = 1; //put into ROB
	reOrder[cBuffer[0].cdbBuffer.dst_tag].intVal = intVal;
	reOrder[cBuffer[0].cdbBuffer.dst_tag].floatVal = floatVal;
}

//Decide which cdb buffer to broadcast to the cdb based on arrival cycle
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

//dequeue an entry in the load/store queue
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

//broadcast the load value on the cdb once it returns from memory
void broadCastLoad(struct RS_entry *iRS, struct RS_entry *fARS, struct RS_entry *fMRS, struct ROB_entry *reOrder, uint32_t iSize, struct LSQ_entry *lsq_Table, uint32_t entNum, uint32_t lsq_Entries) {
	uint32_t intVal = lsq_Table[entNum].memVal;
	float floatVal = lsq_Table[entNum].dst_Val;
	int i;
	for(i = 0; i < iSize; i++) { //update floating point add reservation station
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
	for(i = 0; i < iSize; i++) { //update floating point multiplication reservation station
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
	for(i = 0; i < lsq_Entries; i++) { //update entries in the load/store queue
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
	reOrder[lsq_Table[entNum].ROB_tag].finOp = 1; //update the ROB
	reOrder[lsq_Table[entNum].ROB_tag].intVal = intVal;
	reOrder[lsq_Table[entNum].ROB_tag].floatVal = floatVal;
}

//Execute entires in the reservation stations that are ready
uint32_t RS_Execute(struct branchHistory *bHist, float *mU, uint32_t *MEM, uint32_t mem_cycles, uint32_t exLSCycles, struct LSQ_entry *lsq_Table, uint32_t lsq_Entries, struct ROB_entry *reOrder, uint32_t ROB_Entries, struct RAT_entry *int_rat_Table, struct RAT_entry *float_rat_Table, struct RS_entry *iRS, struct RS_entry *fARS, struct RS_entry *fMRS, uint32_t iSize, struct CDB_buffer *iBuffer, struct CDB_buffer *fABuffer, struct CDB_buffer *fMBuffer, uint32_t cycle_num, uint32_t intCycles, uint32_t fACycles, uint32_t fMCycles, uint32_t *EX) {
	int i;
	uint32_t resAdd = -1;
	for(i = 0; i < iSize; i++) {
		if(iRS[i].isBusy == 1) { //if occupied by instruction
			if(iRS[i].tag1 == -1 && iRS[i].tag2 == -1) { //if values in value fields have valid values and not tags, execute
				if(iRS[i].cyclesLeft == intCycles) { //Starting execution cycle
					EX[iRS[i].address/4] = cycle_num;
				}
				if(iRS[i].cyclesLeft != 0) { //decrease the cycles left to execute if not already zero
					iRS[i].cyclesLeft = iRS[i].cyclesLeft - 1;
				}
				if(iRS[i].cyclesLeft == 0)  { //if finished execution
					if(iRS[i].type == ti_Beq || iRS[i].type == ti_Bne) { //If branch, calculate branch address and update branch history table
						int j;
						for(j = 0; j < INPUT_SIZE; j++) {
							if(bHist[j].isValid == 1) {
								if(bHist[j].br.address == iRS[i].address) {
									break;
								}
							}
						}
						if(iRS[i].type == ti_Beq) {
							if(iRS[i].iVal1 == iRS[i].iVal2) {
								resAdd = iRS[i].address + 4 + (iRS[i].dst_tag<<2);
							}else {
								resAdd = iRS[i].address + 4;
							}
						}else {
							if(iRS[i].iVal1 != iRS[i].iVal2) {
								resAdd = iRS[i].address + 4 + (iRS[i].dst_tag<<2);
							}else {
								resAdd = iRS[i].address + 4;
							}							
						}
						bHist[j].takenAddr = resAdd;
						clearRSEntry(iRS, i); //clear from reservation station
					}else { //if not a branch, it is ready to broadcast on the cdb. If there is already an entry in the cdb, wait
						uint32_t cEntry = isCDBBufEmpty(iBuffer, iSize);
						if(cEntry != -1) {
							putCDBEntry(iRS, iBuffer, i, cEntry, cycle_num);
							clearRSEntry(iRS, i);
						}
					}
				}
			}
		}
		if(iRS[i].isBusy == 3) { //if just put into the reservation station, begin execution the next cycle
			iRS[i].isBusy = 1;
		}
		if(iRS[i].isBusy == 4) { //if waiting on an ROB entry number, attempt to grab one. Attempt to until one is available
			if(isROBFull(reOrder, ROB_Entries) == 0) {
				iRS[i].dst_tag = update_int_ROB_tag(reOrder, ROB_Entries, int_rat_Table, iRS, i);
				iRS[i].isBusy = 3;
			}
		}
	}
	
	for(i = 0; i < iSize; i++) { //floating point add reservation station
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
	
	for(i = 0; i < iSize; i++) { //floating point multiplication reservation station
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
	
	for(i = 0; i < lsq_Entries; i++) { //load/store queue
		if(lsq_Table[i].isBusy == 1) {
			switch(lsq_Table[i].type) {
				case ti_Ld:
					if(lsq_Table[i].tag == -1) { //record execution cycle for load, decrease each cycle until finished execution
						if(lsq_Table[i].ex_cyclesLeft == exLSCycles) {
							EX[lsq_Table[i].address/4] = cycle_num;
						}
						if(lsq_Table[i].ex_cyclesLeft != 0) {
							lsq_Table[i].ex_cyclesLeft = lsq_Table[i].ex_cyclesLeft - 1;
						}
						if(lsq_Table[i].ex_cyclesLeft == 0 && lsq_Table[i].isHead == 1) { //upon finished execution, load value from memory if at the front of the queue
							if(EX[lsq_Table[i].address/4] != cycle_num) {
								if(lsq_Table[i].mem_cyclesLeft == mem_cycles) { //record memory execution start cycle
									MEM[lsq_Table[i].address/4] = cycle_num;
								}
								if(lsq_Table[i].mem_cyclesLeft != 0) {
									lsq_Table[i].mem_cyclesLeft = lsq_Table[i].mem_cyclesLeft - 1;
								}
								if(lsq_Table[i].mem_cyclesLeft == 0) { //when finished in memory, or when a store forwards the value to the load, broadcast the load on the cdb and dequeue when finished
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
													lsq_Table[j].forwardedMEMValue = lsq_Table[i].dst_Val; //upon finished execution, if storing to a memory value that a load points to, forward it to the load
												}
											}
										}
										if(lsq_Table[j].type == ti_Sd) {
											if(lsq_Table[j].tag == -1) {
												if(lsq_Table[j].memVal == lsq_Table[i].memVal) { //if reached a store with the same memory alue to store to, stop forwarding as there will be an updated value at this upcoming load
													break;
												}
											}
										}
									}
								}
							}
							if(reOrder[lsq_Table[i].ROB_tag].finOp != 2) { //put in ROB
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
	return resAdd;
}

//Commit the head entry in the ROB if it is ready
void updateROB(struct LSQ_entry *lsq_Table, uint32_t lsq_Entries, uint32_t memcycles, uint32_t *MEM, struct ROB_entry *reOrder, uint32_t ROB_Entries, struct RAT_entry *int_rat_Table, struct RAT_entry *float_rat_Table, struct int_Reg *iR, struct float_Reg *fR, uint32_t *WB, uint32_t *COM, uint32_t cycle_number, float *mU) {
	int i;
	unsigned char int_float; //0 = int, 1 = float
	for(i = 0; i < ROB_Entries; i++) {
		if(reOrder[i].finOp == 2 && reOrder[i].isHead == 1) {
			COM[reOrder[i].address/4] = cycle_number; //record commit cycle number if it is ready to commit and is in the correct order
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
			if(int_float == 0) { //update integer register file if not R0. If RAT entry points to ROB entry that just committed, point RAT to ARF
				if(regLookup(reOrder[i].destReg) == 0) {
					iR->R_num[regLookup(reOrder[i].destReg)] = 0;
				}else {
					iR->R_num[regLookup(reOrder[i].destReg)] = reOrder[i].intVal;
				}
				if(int_rat_Table[regLookup(reOrder[i].destReg)].iOrf == 0 && int_rat_Table[regLookup(reOrder[i].destReg)].irNumber == i) {
					int_rat_Table[regLookup(reOrder[i].destReg)].rType = 0;
				}					
			}else if(int_float == 1){ //update floating point register file if not R0. If RAT entry points to ROB entry that just committed, point RAT to ARF
				fR->F_num[regLookup(reOrder[i].destReg)] = reOrder[i].floatVal;
				if(float_rat_Table[regLookup(reOrder[i].destReg)].iOrf == 1 && float_rat_Table[regLookup(reOrder[i].destReg)].frNumber == i) {
					float_rat_Table[regLookup(reOrder[i].destReg)].rType = 0;
				}
			}else { //store operation in memory
				MEM[reOrder[i].address/4] = cycle_number + memcycles;
				mU[reOrder[i].intVal/4] = reOrder[i].floatVal;
				if(float_rat_Table[regLookup(reOrder[i].destReg)].iOrf == 1 && float_rat_Table[regLookup(reOrder[i].destReg)].frNumber == i) {
					float_rat_Table[regLookup(reOrder[i].destReg)].rType = 0;
				}
				clearLSQEntry(lsq_Table, lsq_Entries);
			}
			reOrder[i].address = 0; //clear ROB entry and shift head position
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
	for(i = 0; i < ROB_Entries; i++) { //if something was just finished in the ROB, wait until the next cycle to be ready to commit
		if(reOrder[i].finOp == 1) {
			WB[reOrder[i].address/4] = cycle_number;
			reOrder[i].finOp = 2;
			break;
		}
	}
}

//Print contents of ROB table
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

//Print contents of specified reservation station
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

//print contents of load/store queue
void print_LSQ_Queue(struct LSQ_entry *lsq_Table, uint32_t lsq_Entries) {
	printf("address\ttype\tdst_tag\tdst_Val\ttag\toffset\tex_cyclesLeft\tmem_cyclesLeft\tReg\tisHead\tisBusy\tmemVal\tROB_tag\tforwardedVal\n");
	int i;
	for(i = 0; i < lsq_Entries; i++) {
		printf("%d\t%d\t%d\t%.2f\t%d\t%d\t%d\t\t%d\t\t%s\t%d\t%d\t%d\t%d\t%.2f\n", lsq_Table[i].address, lsq_Table[i].type, lsq_Table[i].dst_tag, lsq_Table[i].dst_Val, lsq_Table[i].tag, lsq_Table[i].offset, lsq_Table[i].ex_cyclesLeft, lsq_Table[i].mem_cyclesLeft, lsq_Table[i].Fa, lsq_Table[i].isHead, lsq_Table[i].isBusy, lsq_Table[i].memVal, lsq_Table[i].ROB_tag, lsq_Table[i].forwardedMEMValue);
	}
}