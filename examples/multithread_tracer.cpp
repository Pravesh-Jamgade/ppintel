/*BEGIN_LEGAL 
BSD License 

Copyright (c)2013 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */

//
// @ORIGINAL_AUTHORS: Harish Patil 
//

#include <iostream>

#include "pin.H"
#include "instlib.H"

using namespace std; 

#include "pinplay.H"

// LOCALVAR BIMODAL bimodal;

using namespace INSTLIB; 

// LOCALVAR ofstream *outfile;
FILE* out;
string fileName = "xyzzz_champsim.trace";

#define KNOB_LOG_NAME  "log"
#define KNOB_REPLAY_NAME "replay"
#define KNOB_FAMILY "pintool:pinplay-driver"


PINPLAY_ENGINE pinplay_engine;

KNOB_COMMENT pinplay_driver_knob_family(KNOB_FAMILY, "PinPlay Driver Knobs");

KNOB<BOOL>KnobReplayer(KNOB_MODE_WRITEONCE, KNOB_FAMILY,
                       KNOB_REPLAY_NAME, "0", "Replay a pinball");
KNOB<BOOL>KnobLogger(KNOB_MODE_WRITEONCE,  KNOB_FAMILY,
                     KNOB_LOG_NAME, "0", "Create a pinball");

// KNOB<UINT64> KnobPhases(KNOB_MODE_WRITEONCE, 
//                         "pintool",
//                         "phaselen",
//                         "100000000", 
//                         "Print branch mispredict stats every these many instructions (and also at the end).\n");
// KNOB<string>KnobStatFileName(KNOB_MODE_WRITEONCE,  "pintool",
//                      "statfile", "bimodal.out", "Name of the branch predictor stats file.");


INT32 Usage()
{
    cerr <<
        "This pin tool is a simple PinPlay-enabled branch predictor \n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

// static void threadCreated(THREADID threadIndex, CONTEXT *, INT32 , VOID *)
// {
    // if (threadIndex > 0)
    // {
    //     cerr << "More than one thread detected. This tool currently works only for single-threaded programs/pinballs." << endl;
    //     exit(0);
    // }
// }

const int NUM_INSTR_DESTINATIONS_SPARC = 4;
const int NUM_INSTR_DESTINATIONS = 2;
const int NUM_INSTR_SOURCES = 4;

class Trace
{
    public:
    Trace(){}
    unsigned long long ip = 0;

    // branch info
    unsigned char is_branch = 0;
    unsigned char branch_taken = 0;

    unsigned char destination_registers[NUM_INSTR_DESTINATIONS] = {}; // output registers
    unsigned char source_registers[NUM_INSTR_SOURCES] = {};           // input registers

    unsigned long long destination_memory[NUM_INSTR_DESTINATIONS] = {}; // output memory
    unsigned long long source_memory[NUM_INSTR_SOURCES] = {};           // input 
    
    char context = '0';        // input 
};

class Xchg
{
    public:
    Xchg(){}

    char context = '0';
    UINT64 start = 0;
    UINT64 end = 0;
    BOOL found = 0;
    UINT64 count[2] = {0};
    UINT32 print = 4;

    BOOL within_scope(UINT64 addr)
    {
        return start <= addr && addr <= end;
    }
};

char A='1'; 
char B='2'; 
char C='3';

Xchg* index_arr = new Xchg();
Xchg* edge = new Xchg();
Xchg* property = new Xchg();

UINT64 total_loads = 0;
UINT64 total_stores = 0;
// total instruction traced for trace
UINT64 instrCount = 0;
// total instruction (without any skip)
UINT64 total_instr = 0;

#define vui vector<uint64_t>
vui th_total_instr;

// debung total threads
std::set<uint32_t> th_total_count;

bool start_flag = false;
bool temp = start_flag;

UINT32 huchback = 0;

// SIMPOINT for each of the thread
#define pii pair<uint64_t, uint64_t>
#define vpii vector<pii>
#define vvpii vector<vpii>

vvpii th_specific_simpoint;
FILE* simpoint_out;
string simpoint_file = "";
KNOB<string>KnobStatFileName(KNOB_MODE_WRITEONCE,  "pintool",
                     "input", "bimodal.out", "Name of the branch predictor stats file.");

bool trace_on = false;

VOID set_flip(UINT64 RegA, UINT64 RegB, UINT64 RegC)
{
    if( RegA==RegB && RegB==RegC)
    {
        // std::cout << std::hex << "flip, " << RegA << "," << RegB << "," << RegC << '\n';
        temp = start_flag;
        // cout << start_flag << "-->";
        start_flag = !start_flag;
        // cout << start_flag << '\n';
    }
    else
    {
        if(huchback >= 3)
        {
            return;
        }

        if(RegC == 1)
        {
            cout << std::hex << "[Handle Magic 1 ] regA=" << RegA << ", regB=" << RegB <<", regC=" << RegC << '\n'; 
            index_arr->context = '1';
            index_arr->start = RegA;
            index_arr->end = RegB;
            index_arr->found = 1;
            huchback+=1;
        }
        else if(RegC == 2)
        {
            cout << std::hex << "[Handle Magic 2 ] regA=" << RegA << ", regB=" << RegB <<", regC=" << RegC << '\n'; 

            edge->context = '2';
            edge->start = RegA;
            edge->end = RegB;
            edge->found = 1;
            huchback+=1;
        }
        else if(RegC == 3)
        {
            cout << std::hex << "[Handle Magic 3 ] regA=" << RegA << ", regB=" << RegB <<", regC=" << RegC << '\n'; 

            property->context = '3';
            property->start = RegA;
            property->end = RegB;
            property->found = 1;
            huchback+=1;
        }   
    }
}

VOID Found(UINT64 RegA, UINT64 RegB, int RegC)
{
    if(RegC == 1)
    {
        index_arr->context = '1';
        index_arr->start = RegA;
        index_arr->end = RegB;
        index_arr->found = 1;
    }
    else if(RegC == 2)
    {
        edge->context = '2';
        edge->start = RegA;
        edge->end = RegB;
        edge->found = 1;
    }
    else if(RegC == 3)
    {
        property->context = '3';
        property->start = RegA;
        property->end = RegB;
        property->found = 1;
    }

    cout << std::hex << "* " << RegA << ", " << RegB << "," << RegC << '\n';
}

VOID Branch(BOOL branch_taken, VOID* v)
{
    Trace* trace = reinterpret_cast<Trace*>(v);
    trace->branch_taken = branch_taken;
}

void RegRead(UINT32 i, UINT32 index, VOID* v)
{
    Trace* trace = reinterpret_cast<Trace*>(v);

    REG r = (REG)i;

    // check to see if this register is already in the list
    int already_found = 0;
    for(int i=0; i<NUM_INSTR_SOURCES; i++)
    {
        if(trace->source_registers[i] == ((unsigned char)r))
        {
            already_found = 1;
            break;
        }
    }
    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_SOURCES; i++)
        {
            if(trace->source_registers[i] == 0)
            {
                trace->source_registers[i] = (unsigned char)r;
                break;
            }
        }
    }
}

void RegWrite(REG i, UINT32 index, VOID* v)
{
    Trace* trace = reinterpret_cast<Trace*>(v);

    REG r = (REG)i;

    int already_found = 0;
    for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
    {
        if(trace->destination_registers[i] == ((unsigned char)r))
        {
            already_found = 1;
            break;
        }
    }

    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
        {
            if(trace->destination_registers[i] == 0)
            {
                trace->destination_registers[i] = (unsigned char)r;
                break;
            }
        }
    }
}

void MemoryRead(VOID* addr, UINT32 index, UINT32 read_size, VOID* v)
{
    Trace* trace = reinterpret_cast<Trace*>(v);

    total_loads++;

    // check to see if this memory read location is already in the list
    int already_found = 0;
    for(int i=0; i<NUM_INSTR_SOURCES; i++)
    {
        if(trace->source_memory[i] == ((unsigned long long int)addr))
        {
            already_found = 1;
            break;
        }
    }

    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_SOURCES; i++)
        {
            if(trace->source_memory[i] == 0)
            {
                trace->source_memory[i] = (unsigned long long int)addr;
                break;
            }
        }
    }
}

void MemoryWrite(VOID* addr, UINT32 index, VOID* v)
{
    Trace* trace = reinterpret_cast<Trace*>(v);

    total_stores++;

    // check to see if this memory write location is already in the list
    int already_found = 0;
    for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
    {
        if(trace->destination_memory[i] == ((unsigned long long int)addr))
        {
            already_found = 1;
            break;
        }
    }

    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
        {
            if(trace->destination_memory[i] == 0)
            {
                trace->destination_memory[i] = (unsigned long long int)addr;
                break;
            }
        }
    }
}

/// @brief 
/// @param r        effective memory address 
/// @param index    read(0)/write(1) 
/// @param v        trace
void addMagic(UINT64 r, UINT32 index, VOID* v)
{
    Trace* trace = reinterpret_cast<Trace*>(v);
    if(index_arr->within_scope(r))
    {
        // PIN_SafeCopy(&curr_instr.context, &A, sizeof(char));
        trace->context = A;
        if(A!=trace->context)
        {
            exit(-1);
        }

        if(index_arr->print > 0){
            std::cout << std::hex << "[MAGIC "<< A <<"]" << index_arr->start << "," << r << "," << index_arr->end << "," << trace->context << "," << '\n';
            index_arr->print = index_arr->print -1;
        }
        index_arr->count[index]+=1;
    }

    else if(edge->within_scope(r))
    {
        // PIN_SafeCopy(&curr_instr.context, &B, sizeof(char));
        trace->context = B;
        if(B!=trace->context)
        {
            exit(-1);
        }

        if(edge->print>0)
        {
            std::cout << std::hex << "[MAGIC "<< B <<"]" << edge->start << "," << r << "," << edge->end << "," << trace->context << "," << '\n';
            edge->print = edge->print - 1;
        }
        edge->count[index]+=1;
    }

    else if(property->within_scope(r))
    {
        // PIN_SafeCopy(&curr_instr.context, &C, sizeof(char));
        trace->context = C;
        if(C!=trace->context)
        {
            exit(-1);
        }

        if(property->print>0)
        {
            std::cout << std::hex << "[MAGIC "<< C <<"]" << property->start << "," << r << "," << property->end << "," << trace->context << "," << '\n';
            property->print = property->print - 1;
        }
        property->count[index]+=1;
    }
  
}

VOID InsCount(UINT64* ip, VOID *v)
{
    instrCount++;

    Trace* trace = reinterpret_cast<Trace*>(v);

    trace->ip = (unsigned long long)*ip;
    trace->is_branch = 0;
    trace->branch_taken = 0;

    for(int i=0; i<NUM_INSTR_DESTINATIONS; i++) 
    {
        trace->destination_registers[i] = 0;
        trace->destination_memory[i] = 0;
    }

    for(int i=0; i<NUM_INSTR_SOURCES; i++) 
    {
        trace->source_registers[i] = 0;
        trace->source_memory[i] = 0;
    }
}

VOID TotalInsCount(THREADID tid)
{
    
    total_instr++;
    th_total_instr[tid]++;
    th_total_count.insert(tid);
}

VOID WriteToFile(VOID* v)
{
    Trace* trace = reinterpret_cast<Trace*>(v);
    // if(trace->context != '0')
    //     cout << "WRITE: " << trace->context << '\n';
    fwrite(trace, sizeof(Trace), 1, out);
}

VOID checkSimpoint(THREADID tid, VOID* v_total_instr, VOID* v_trace_on)
{
    uint64_t* th_total_instr = reinterpret_cast<uint64_t*>(v_total_instr);
    bool* trace_on = reinterpret_cast<bool*>(v_trace_on);
    // cout << tid << "," << th_total_instr[tid] << '\n';
    // for(size_t i=0; i< (size_t)th_specific_simpoint[tid].size(); i++)
    // {
    //     if(th_total_instr[tid] >= th_specific_simpoint[tid][i].first && 
    //         th_total_instr[tid] <= th_specific_simpoint[tid][i].second)
    //     {
    //     //     if(!trace_on)
    //     //         std::cout << "tid," << tid << ", simpoint, " << th_specific_simpoint[tid][i].first << ", *, " << th_total_instr[tid]  << ", " << th_specific_simpoint[tid][i].second << '\n';
            
    //     //     //allow
    //     //     trace_on = true;
    //     }
    //     else trace_on = false;
    // }
   
    for(auto pr : th_specific_simpoint[tid])
    {
        if(th_total_instr[tid] >= pr.first && 
        th_total_instr[tid] <= pr.second)
        {
            // if(!trace_on)
            //     std::cout << std::dec << "tid," << tid << ", simpoint, " << pr.first << ", *, " << th_total_instr[tid]  << ", " << pr.second << '\n';
            
            //allow
            *trace_on = true;
        }
        else *trace_on = false;
    }
}

VOID Instruction(INS ins, VOID *v)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)TotalInsCount, IARG_THREAD_ID, IARG_END);

    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)checkSimpoint, IARG_THREAD_ID, IARG_PTR, &th_total_instr, IARG_PTR, &trace_on, IARG_END);
 
    if ( INS_IsXchg(ins) && INS_OperandReg(ins, 0) == REG_BX && INS_OperandReg(ins, 1) == REG_BX)
    {
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)set_flip,  IARG_REG_VALUE, REG_GAX, IARG_REG_VALUE, REG_GBX, IARG_REG_VALUE, REG_GCX, IARG_END);

        // // if it is a magic instruction (ROI marker) then donot trace
        // if(temp != start_flag)
        // {
        //     return;
        // }
    }

    // //KNOB:  It allows the use of SimRoiStart and SimRoiEnd
    // // if it is ROI Start then keep tracing untill ROI End 
    if(!start_flag)
    {
        return;
    }

    Trace* trace = new Trace();

    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)InsCount, IARG_INST_PTR, IARG_PTR, (VOID*)trace, IARG_END);

    // if(INS_IsXchg(ins) && INS_OperandReg(ins, 0) == REG_BX && INS_OperandReg(ins, 1))
    // {
    //     INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)Found, IARG_REG_VALUE, REG_GAX, IARG_REG_VALUE, REG_GBX, IARG_REG_VALUE, REG_GCX, IARG_END);
    //     // Trace->context = REG_GCX;
    //     // Trace->ip = IARG_INST_PTR;
    //     // fwrite(Trace, sizeof(Trace), 1, out);
    // }

    // instrument branch instructions
    if(INS_IsBranch(ins))
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)Branch, IARG_BRANCH_TAKEN, IARG_PTR, (VOID*)trace, IARG_END);
    

    // instrument register reads
    UINT32 readRegCount = INS_MaxNumRRegs(ins);
    for(UINT32 i=0; i<readRegCount; i++) 
    {
        UINT32 regNum = INS_RegR(ins, i);
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RegRead, IARG_UINT32, regNum, IARG_UINT32, i, IARG_PTR, (VOID*)trace, IARG_END);
    }

    // instrument register writes
    UINT32 writeRegCount = INS_MaxNumWRegs(ins);
    for(UINT32 i=0; i<writeRegCount; i++) 
    {
        UINT32 regNum = INS_RegW(ins, i);
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RegWrite, IARG_UINT32, regNum, IARG_UINT32, i, IARG_PTR, (VOID*)trace, IARG_END);
    }

    // Iterate over each memory operand of the instruction.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    if(!trace_on && memOperands<=0)
    {
        return;
    }

    for (UINT32 memOp = 0; memOp < memOperands; memOp++) 
    {
        if (INS_MemoryOperandIsRead(ins, memOp)) 
        {
            UINT32 read_size = 0;//INS_MemoryReadSize(ins);

            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemoryRead,
                    IARG_MEMORYOP_EA, memOp, IARG_UINT32, memOp, IARG_UINT32, read_size, IARG_PTR, (VOID*)trace, IARG_END);
        }
        
        if (INS_MemoryOperandIsWritten(ins, memOp)) 
        {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemoryWrite,
                    IARG_MEMORYOP_EA, memOp, IARG_UINT32, memOp, IARG_PTR, (VOID*)trace, IARG_END);
        }

        if( index_arr->found || edge->found ||  property->found )
        {
          if(INS_MemoryOperandIsWritten(ins, memOp)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)addMagic, IARG_MEMORYOP_EA, memOp, IARG_UINT32, 1, IARG_PTR, (VOID*)trace, IARG_END);
          }
          else if(INS_MemoryOperandIsRead(ins, memOp)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)addMagic, IARG_MEMORYOP_EA, memOp, IARG_UINT32, 0, IARG_PTR, (VOID*)trace, IARG_END);
          }
        }
    }

    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToFile, IARG_PTR, (VOID*)trace, IARG_END);
}

VOID Fini(INT32 code, VOID *v)
{
    if(out!=NULL)
    {
        fclose(out);
        out = NULL;

        std::cout << std::dec << "XXX, load(A),"<< index_arr->count[0] << ",store(A)," << index_arr->count[1] 
                << ",load(B)," << edge->count[0] << ",store(B)," << edge->count[1] 
                << ",load(C)," << property->count[0] << ",store(C)," << property->count[1] 
                << ",total_loads," << total_loads << ",total_stores," << total_stores << ",total_instr," << instrCount <<  '\n'; 

        for(auto e: th_total_count)
            std::cout << std::dec << e << '\n';
    }
}

void read_simpoint()
{
    // total threads
    int cth;
    // thread number
    int th;
    // total simpoints per thread
    int nsims;
    // simpoint range
    UINT64 sim_start, sim_end;

    std::cin >> cth;
    th_specific_simpoint.resize(cth+1);
    th_total_instr.resize(cth+1);

    // no. of threads
    while(cth--)
    {
        // thread number and number of simpoints
        std::cin>> th >> nsims;

        th_specific_simpoint[th].resize(nsims);

        // sim regions 
        std::cout << "TH: " << th << ", NSIMS: " << nsims << '\n';
        for(int i=0; i< nsims; i++)
        {
            std::cin >> sim_start >> sim_end;
            th_specific_simpoint[th][i] = {sim_start, sim_end};

            std::cout << th_specific_simpoint[th][i].first << "," << th_specific_simpoint[th][i].second << '\n';            
        }
    }
    
}

int main(int argc, char *argv[])
{
    for(int i=0; i< argc; i++)
    {
        std::cout << i << ", " << argv[i] << '\n';
    }



    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    out = fopen(fileName.c_str(), "w");
    freopen(KnobStatFileName.Value().c_str(), "r", stdin);
    read_simpoint();

    // outfile = new ofstream(KnobStatFileName.Value().c_str());
    // bimodal.Activate(KnobPhases, outfile);
    
    pinplay_engine.Activate(argc, argv, KnobLogger, KnobReplayer);
    if(KnobLogger)
    {
        cout << "Logger basename " << pinplay_engine.LoggerGetBaseName() 
            << endl;
    }
    if(KnobReplayer)
    {
        cout << "Replayer basename " << pinplay_engine.ReplayerGetBaseName() 
            << endl;
    }

    INS_AddInstrumentFunction(Instruction, 0);

    PIN_AddFiniFunction(Fini, 0);

    // PIN_AddThreadStartFunction(threadCreated, reinterpret_cast<void *>(0));


    PIN_StartProgram();
}
