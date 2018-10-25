Noah Perryman
Patrick Lyons
COE 1541 - Project 2

This file explains the contents of the submission

—————————————————————
source_code Contents:
—————————————————————

(1)CPU+cache.c is an extension of CPU.c that includes an instruction cache and a data cache. 
Compile using gcc -lm -o  CPU+cache CPU+cache.c
***********
NOTE - if you do not compile using ‘gcc -lm -o CPU+cache CPU+cache.c’ the program will not
compile. <math.h> is included, but it needs to be linked to cache.h during compilation. -lm
handles this.
***********
run using ./CPU+cache ‘trace_file_name’ ‘branch_prediction_method’ ‘trace_view_on’
The program assumes a file named cache_config.txt is available in the current directory. If
cache_config.txt is not included the program will not compile.
cache_config.txt contains 7 integer values: the size of the instruction cache in Kilo Bytes,
the associativity of the instruction cache, the block size of the instruction cache in bytes,
the size of the data cache Kilo Bytes, the associativity of the data cache, the data cache 
block size in bytes, and the memory access time in cycles. The values are taken in that order.
A five stage pipeline is simulated where the PC instruction fetch retrieves the PC value 
from the instruction cache in the IF stage and load/stores are read/stored from/in the data
cache in the MEM stage. If a miss is detected that pipeline stalls accordingly. Note that 
this source file includes CPU.h, so CPU.h needs to be in the directory that this file is compiled
in in order to compile correctly. Here is an example of our output:

    ** opening file sample_large1.tr

    Cache Size: 1 KB
    Associativity: 4
    Block Size: 4 Bytes

    + Simulation terminates at cycle : 388618939
    I-cache accesses 93672795 and misses 434006
    D-cache Read accesses 20813032 and misses 9098182
    D-cache Write accesses 9625833 and misses 1861519

The first line tells us which trace file is being used. The next 3 tell us the
various sizes of cache properties, and finally, we see the execution time in
cycles, along with the access and miss stats for the caches. To get the miss
rates, we simply take I_Misses/I_Accesses * 100% to get the miss rate for
the I Cache, and (D_Read_Miss + D_Write_Miss)/(D_Read_Access + D_Write_Access)
* 100% for the D Cache. All of this information is shown in the excel sheets.
The first sheet in the workbook is for sample_large1.tr output, and the second
is for sample_large2.tr. In this workbook, the data near the top is the miss rates as percentages, and
the data below that is without the * 100% calculation.

(2)cache.h is a skeleton file which includes the structure for the cache implementations needed
for CPU+cache.c. This source file creates a cache which contains the number of sets, the block size, 
the associativity, the memory latency (miss penalty), a pointer array to the array of cache blocks,
and an array of Node stacks to keep track of the least recently used cache block. A cache access
starts by calculating the index and tag fields for the address and then determines if there is a 
hit or a miss. If it is a hit there is no pipeline stall. If there is a miss there is a pipeline stall.
If a miss and a write back are needed, the pipeline stalls for two times the memory latency. If 
there is no write-back, the pipeline stalls for the memory latency.

—————————————————————————————————————————
simulations_tables_explanations Contents:
—————————————————————————————————————————

(1)Experiment1.txt, Experiment2.txt, Experiment3.txt: The results of running the simulation with trace_view_on = 0 
for sample_large1.tr. and sample_large2.tr. This includes the combined .txt files for Experiments 1, 2, and 3.

(2)miss_rates_and_execution_time: The tables and bar graph specified in the experiments.The first sheet contains 
the table for sample_large1.tr and the second sheet contains the table for sample_large2.tr. The third sheet 
contains the optimal parameters for Experiment 2. The fourth sheet contains the tables and bar graph for 
Experiment 3. Note that the I_cache miss rate, in percent, is so low (approximately 0 to three decimal places)
that it does not appear to be on the bar graph, but it is there. 

(3)Our remarks and explanation of the results and the conclusions that we draw from the experiments.