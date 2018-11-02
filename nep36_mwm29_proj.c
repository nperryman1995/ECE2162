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

/*Function declarations*/
void initRegs(struct int_Reg *iR, struct float_Reg *fR);
void writeIntReg(struct int_Reg *iR, char *reg, uint32_t val);
void writeFloatReg(struct float_Reg *fR, char *reg, float val);
uint32_t getIntReg(struct int_Reg *iR, char *reg);
float getFloatReg(struct float_Reg *fR, char *reg);
int regLookup(char *reg);
void assignInstr(struct instruction *instr);
void printInstr(struct instruction *instr);
void showIntReg(struct int_Reg *iR);
void showFPReg(struct float_Reg *fR);

/*Instruction struct to take in all of the instruction types*/
struct instruction {
	unsigned char type;
	char* Ra;
	char* Fa;
	char* Rs;
	char* Fs;
	char* Rt;
	char* Ft;
	uint32_t offset; //For both offsets and immediates
	//uint32_t PC_Addr;
};

/*Encoding for instruction struct type*/
enum opcode_type {
	ti_Ld, //Load a single precision floating point value
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

int main(int argc, char **argv)
{
	unsigned int cycle_number = 0;
	struct int_Reg iR;
	struct float_Reg fR;
	initRegs(&iR, &fR); //initialize integer and floating point regsiters
	
	struct instruction *entry; //instruction struct for taking in new instructions
	struct instruction IS, EX, MEM, WB, COM; //instruction structs for pipeline stages
	
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
	
	double memData[256];
	int i;
	for(i = 0; i < 256; i++) {
		memData[i] = 0;
	}
	
	if (argc == 1) {
		exit(0);
	}
	
	fileName = argv[1]; //Retrieve file name and open it
	trace_fd = fopen(fileName, "rb");
	if (!trace_fd) { //if file cannot be open, exit program
		fprintf(stdout, "\ntrace file %s not opened.\n\n", fileName);
		exit(0);
	}
	
	//read text file inputs
	fscanf(trace_fd, "%d %d %d %d", &intAdd_rs, &intAdd_EX_Cycles, &intAdd_MEM_cycles, &intAdd_FUs);
	fscanf(trace_fd, "%d %d %d %d", &FPAdd_rs, &FPAdd_EX_Cycles, &FPAdd_MEM_cycles, &FPAdd_FUs);
	fscanf(trace_fd, "%d %d %d %d", &FPMult_rs, &FPMult_EX_Cycles, &FPMult_MEM_cycles, &FPMult_FUs);
	fscanf(trace_fd, "%d %d %d %d", &ld_sd_rs, &ld_sd_EX_Cycles, &ld_sd_MEM_cycles, &ld_sd_FUs);
	fscanf(trace_fd, "%d %d", &ROB_Entries, &CDB_Buffer_Entries);
	fscanf(trace_fd, "%d %d %d", &IntregInits, &FPregInits, &MemInits);
	fscanf(trace_fd, "%d", &numInstr);
	
	char *regtemp;
	int regtempVal;
	float regtempValFP;
	for(i = 0; i < IntregInits; i++) {
		fscanf(trace_fd, "%s %d", regtemp, &regtempVal);
		writeIntReg(iR, regtemp, regtempVal);
	}
	
	for(i = 0; i < FPregInits; i++) {
		fscanf(trace_fd, "%s, %f", regtempValFP);
		writeFloatReg(fR, regtemp, regtempValFP);
	}
	
	for(i = 0; i < MemInits; i++) {
		fscanf(trace_fd, "%s %d %lf", regtemp, &regtempVal, &regtempValFP);
		memData[regtempVal] = regtempValFP;
	}
	
	entry = malloc(sizeof(struct instruction) * numInstr);
	for(i = 0; i < numInstr; i++) {
		fscanf(trace_fd, "%s", regtemp);
		assignInstr(entry[i], regtemp);
		switch(entry[i].type) {
			case ti_Ld:
				fscanf(trace_fd, "%s, %d(%s)", entry[i].Fa, entry[i].offset, entry[i].Ra);
				break;
			case ti_Sd:
				fscanf(trace_fd, "%s, %d(%s)", entry[i].Fa, entry[i].offset, entry[i].Ra);
				break;		
			case ti_Beq:
				fscanf(trace_fd, "%s, %s, %d", entry[i].Rs, entry[i].Rt, entry[i].offset);
				break;		
			case ti_Bne:
				fscanf(trace_fd, "%s, %s, %d", entry[i].Rs, entry[i].Rt, entry[i].offset);
				break;		
			case ti_Add:
				fscanf(trace_fd, "%s, %s, %s", entry[i].Rd, entry[i].Rs, entry[i].Rt);
				break;		
			case ti_Addd:
				fscanf(trace_fd, "%s, %s, %s", entry[i].Fd, entry[i].Fs, entry[i].Ft);
				break;		
			case ti_Addi:
				fscanf(trace_fd, "%s, %s, %d", entry[i].Rt, entry[i].Rs, entry[i].offset);
				break;		
			case ti_Sub:
				fscanf(trace_fd, "%s, %s, %s", entry[i].Rd, entry[i].Rs, entry[i].Rt);
				break;		
			case ti_Subd:
				fscanf(trace_fd, "%s, %s, %s", entry[i].Fd, entry[i].Fs, entry[i].Ft);
				break;		
			case ti_Multd:
				fscanf(trace_fd, "%s, %s, %s", entry[i].Fd, entry[i].Fs, entry[i].Ft);
				break;		
		}
	}
  
	free(entry);
	fclose(trace_fd);
	return 1;
  
}

//Initialize Integer and floating point register files
void initRegs(struct int_Reg *iR, struct float_Reg *fR) {
	int i;
	for(i = 0; i < 32; i++) {
		iR->R_num[i] = 0;
		fR->F_num[i] = 0;
		if(i==0) { //R0 = 0, all others can be written to
			iR->canWrite[i] = 0;
		}else {
			iR->canWrite[i] = 1;
		}
	}
}

/*Write to the specified integer register*/
void writeIntReg(struct int_Reg *iR, char *reg, uint32_t val) {
	int rN = regLookup(reg);
	if(iR->canWrite[rN]==1) {
		iR->R_num[rN] = val;
	}
}

/*Write to the specified floating point register*/
void writeFloatReg(struct float_Reg *fR, char *reg, float val) {
	int rN = regLookup(reg);
	fR->f_num[rN] = val;
}

/*Get the specified integer register*/
uint32_t getIntReg (struct int_Reg *iR, char *reg) {
	int rN = regLookup(reg);
	return iR->R_num[rN];
}

/*Get the specified floating point register*/
float getFloatReg(struct float_Reg *fR, char *reg) {
	int rN = regLookup(reg);
	return fR->R_num[rN];
}

/*Given the register string, find the register number*/
int regLookup(char *reg) {
	char *p = reg;
	int ret = 0;
	while (*p) { // While there are more characters to process
		if ( isdigit(*p) || ( (*p=='-'||*p=='+') && isdigit(*(p+1)) )) {
			ret = strtol(p, &p, 10); // Read number
		} else { // Otherwise, move on to the next character.
			p++;
		}
	}
	return ret;
}

/*Assign the type field of the instruction*/
void assignInstr(struct instruction *instr, char *temp) {
	if(!strcmp(???, "Ld")){
		instr->type = ti_Ld;
	}else if(!strcmp(???,"Sd")) {
		instr->type = ti_Sd;
	}else if(!strcmp(???,"Beq")) {
		instr->type = ti_Beq;
	}else if(!strcmp(???,"Bne")) {
		instr->type = ti_Bne;
	}else if(!strcmp(???,"Add")) {
		instr->type = ti_Add;
	}else if(!strcmp(???,"Add.d")) {
		instr->type = ti_Addd;
	}else if(!strcmp(???,"Addi")) {
		instr->type = ti_Addi;
	}else if(!strcmp(???,"Sub")) {
		instr->type = ti_Sub;
	}else if(!strcmp(???,"Sub.d")) {
		instr->type = ti_Subd;
	}else {
		instr->type = ti_Multd;
	}
}

/*print the instruction*/
void printInstr(struct instruction *instr) {
	switch(instr->type) {
		case ti_Ld:
			printf("Ld %s, %d(%s)",instr->Fa, instr->offset, instr->Ra);
			break;
		case ti_Sd:
			printf("Sd %s, %d(%s)",instr->Fa, instr->offset, instr->Ra);
			break;		
		case ti_Beq:
			printf("Beq %s, %s, %d",instr->Rs, instr->Rt, instr->offset);
			break;		
		case ti_Bne:
			printf("Bne %s, %s, %d",instr->Rs, instr->Rt, instr->offset);
			break;		
		case ti_Add:
			printf("Add %s, %s, %s", instr->Rd, instr->Rs, instr->Rt);
			break;		
		case ti_Addd:
			printf("Add.d %s, %s, %s", instr->Fd, instr->Fs, instr->Ft);
			break;		
		case ti_Addi:
			printf("Addi %s, %s, %d", instr->Rd, instr->Rs, instr->offset);
			break;		
		case ti_Sub:
			printf("Sub %s, %s, %s", instr->Rd, instr->Rs, instr->Rt);
			break;		
		case ti_Subd:
			printf("Sub.d %s, %s, %s", instr->Fd, instr->Fs, instr->Ft);
			break;		
		case ti_Multd:
			printf("Mult.d %s, %s, %s", instr->Fd, instr->Fs, instr->Ft);
			break;		
	}
}

/*Show contents of integer register files*/
void showIntReg(struct int_Reg *iR) {
	int i;
	for(i = 0; i < 32; i++) {
		printf("R%d = %d",i, iR->R_Num[i]);
	}
}

/*Show contents of floating point register files*/
void showFPReg(struct float_Reg *fR) {
	int i;
	for(i = 0; i < 32; i++) {
		printf("F%d = %d",i, fR->F_Num[i]);
	}	
}