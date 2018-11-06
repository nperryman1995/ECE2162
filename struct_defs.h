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

/*Memory unit to have a distinction between integer and floating point values*/
struct memUnit {
	uint32_t intMem;
	float floatMem;
	char type;
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
struct instruction *BTB_array[8]; // Stores the instruction that is predicted to be fetched after a branch


struct branch_pred_entry { // One entry of the branch predictor. Array of them makes the entire predictor
    boolean prediction;
} branch_pred_entry;
