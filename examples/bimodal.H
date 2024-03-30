/*BEGIN_LEGAL 
BSD License 

Copyright (c)2012 Intel Corporation. All rights reserved.
 
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
// @ORIGINAL_AUTHORS: Cristiano Pereira and Harish Patil 
//

#define NUM_INSTR_DESTINATIONS 2
#define NUM_INSTR_SOURCES 4
typedef struct trace_instr_format {
    unsigned long long int ip;  // instruction pointer (program counter) value

    unsigned char is_branch;    // is this branch
    unsigned char branch_taken; // if so, is this taken

    unsigned char destination_registers[NUM_INSTR_DESTINATIONS]; // output registers
    unsigned char source_registers[NUM_INSTR_SOURCES];           // input registers

    unsigned long long int destination_memory[NUM_INSTR_DESTINATIONS]; // output memory
    unsigned long long int source_memory[NUM_INSTR_SOURCES];           // input memory

    char context = '0';
} trace_instr_format_t;

class DS
{
    public:
    DS(){}
    
   
};

class BIMODAL
{
  public:
    BIMODAL();
    UINT64 References() { return _references;};
    UINT64 Predicts() { return _predicts;};
    UINT64 Icount() { return _icount;};
    UINT64 Mispredicts() { return _references - _predicts;};
    VOID Activate(UINT64 phaseLen, ofstream *outfile)
    {
        _phaseLen = phaseLen;
        _nextPrintIcount = phaseLen;
        _outfile = outfile;
        INS_AddInstrumentFunction(Instruction, this);
        PIN_AddFiniFunction(PrintStats, this);
    }

  private:
    enum 
    {
        TABLESIZE = 4096
    };

    static VOID Instruction(INS ins, VOID *v);
    static VOID CondBranch(VOID *v, VOID * v2,  BOOL taken);
    static VOID Count(VOID *v);
    static VOID PrintStats(int n, VOID *v);
    INT8 * HistAddress(INS ins);

    VOID addMagic(unsigned long long r, UINT32 index);
    VOID handleMagic(unsigned long long a, unsigned long long b, int hint);

    static BOOL check(INS ins, VOID* bm);
    static VOID caller1(VOID* add);
    static VOID caller2(VOID* handle);

    ofstream *_outfile;

    INT8 _branchHistory[TABLESIZE];
    UINT64 _references, _predicts, _icount;
    UINT64 _phaseLen, _nextPrintIcount;

    unsigned long long start[4]={0}, end[4]={0};
    bool found[4]={0};
    char  A='1', B='2', C='3';
    int printA, printB, printC;
    UINT64 countA[2]={0}, countB[2]={0}, countC[2]={0};
    trace_instr_format_t curr_instr = trace_instr_format();
};

BIMODAL::BIMODAL()
{
    _references = 0;
    _predicts = 0;
    _icount = 0;
    _phaseLen = 0;
    _nextPrintIcount = 0;
}

void BIMODAL::addMagic(unsigned long long r, UINT32 index)
{
  if(start[1]<=r && r<= end[1])
  {
    // PIN_SafeCopy(&curr_instr.context, &A, sizeof(char));
    curr_instr.context = A;
    if(A!=curr_instr.context)
    {
      exit(-1);
    }

    if(printA>0){
        std::cout << std::hex << "[MAGIC "<< A <<"]" << start[1] << "," << r << "," << end[1] << "," << curr_instr.context << "," << '\n';
        printA = printA -1;
    }
    // sleep(1);
    countA[index]+=1;
  }

  else if(start[2]<=r && r<= end[2])
  {
    // PIN_SafeCopy(&curr_instr.context, &B, sizeof(char));
    curr_instr.context = B;
    if(B!=curr_instr.context)
    {
      exit(-1);
    }

    if(printB>0)
    {
        std::cout << std::hex << "[MAGIC "<< B <<"]" << start[2] << "," << r << "," << end[2] << "," << curr_instr.context << "," << '\n';
        printB = printB - 1;
    }
    // sleep(1);
    countB[index]+=1;
  }

  else if(start[3]<=r && r<= end[3])
  {
    // PIN_SafeCopy(&curr_instr.context, &C, sizeof(char));
    curr_instr.context = C;
    if(C!=curr_instr.context)
    {
      exit(-1);
    }
    
    if(printC>0)
    {
        std::cout << std::hex << "[MAGIC "<< C <<"]" << start[3] << "," << r << "," << end[3] << "," << curr_instr.context << "," << '\n';
        printC = printC - 1;
    }
    // sleep(1);
    countC[index]+=1;
  }
  
}

VOID BIMODAL::handleMagic(unsigned long long a, unsigned long long b, int hint)
{
    std::cout << a <<","<< b <<","<<hint << ", hhhhhdsdsdsd\n";
    start[hint] = a;
    end[hint] = b;
    found[hint] = true;
    if(a!=b)
        std::cout << std::hex << "[FOUND] " << start[hint] << "," << end[hint]  << "," << hint << "," << found[hint] << '\n';
}

BOOL BIMODAL::check(INS ins, VOID* v)
{
    BIMODAL *bm = reinterpret_cast<BIMODAL*>(v);
    return bm->found[0] || bm->found[1] || bm->found[2];
}

VOID BIMODAL::Instruction(INS ins, VOID *v)
{
    BIMODAL *bm = reinterpret_cast<BIMODAL*>(v);

    // UINT32 memOperands = INS_MemoryOperandCount(ins);
    // for (UINT32 memOp = 0; memOp < memOperands; memOp++) 
    // {
    //     // if (INS_MemoryOperandIsRead(ins, memOp)) 
    //     // {
    //     //     UINT32 read_size = 0;//INS_MemoryReadSize(ins);

    //     //     INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemoryRead,
    //     //             IARG_MEMORYOP_EA, memOp, IARG_UINT32, memOp, IARG_UINT32, read_size, IARG_PTR, (VOID*)&curr_instr,
    //     //             IARG_END);
    //     // }
        
    //     // if (INS_MemoryOperandIsWritten(ins, memOp)) 
    //     // {
    //     //     INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemoryWrite,
    //     //             IARG_MEMORYOP_EA, memOp, IARG_UINT32, memOp, IARG_PTR, (VOID*)&curr_instr,
    //     //             IARG_END);
    //     // }

    //     INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)check, IARG_PTR, (VOID*)bm, IARG_END);
        
    //     if(INS_MemoryOperandIsWritten(ins, memOp)) {
    //     INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)caller2, (VOID *)bm->addMagic(memOp, 1), IARG_END);
    //     }
    //     else if(INS_MemoryOperandIsRead(ins, memOp)) {
    //     INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)caller2, (VOID *)bm->addMagic(memOp, 0), IARG_END);
    //     }
        

    // }

    // if (INS_IsXchg(ins) && INS_OperandReg(ins, 0) == REG_BX && INS_OperandReg(ins, 1) == REG_BX)
    // {
    //   INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)caller1, (VOID *)bm->handleMagic(REG_GAX, REG_GBX, REG_GCX), IARG_END);
    // }

    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, 
           (AFUNPTR)Count, IARG_PTR, (VOID *)bm, IARG_END);
    if ((INS_IsBranch(ins) || INS_IsCall(ins)) && INS_HasFallThrough(ins))
        {
            INS_InsertCall(ins, IPOINT_BEFORE, 
                           (AFUNPTR)CondBranch, IARG_PTR, (VOID *)bm, IARG_PTR, (VOID *)bm->HistAddress(ins), IARG_BRANCH_TAKEN, IARG_END);
        }
}

INT8 * BIMODAL::HistAddress(INS ins)
{
    ADDRINT ip = INS_Address(ins);
    return &(_branchHistory[ip % TABLESIZE]);
}

VOID BIMODAL::CondBranch(VOID *v, VOID * v2, BOOL taken)
{
    BIMODAL *bm = reinterpret_cast<BIMODAL*>(v);
    INT8  *branchHistory = reinterpret_cast<INT8*>(v2);
    INT8 history;

    bm->_references += 1;

    history = (*branchHistory) & 0x3;

    if((taken && history >= 2) || (!taken && history <= 1))
        bm->_predicts += 1;

    INT8 delta = 0;
    if (taken && (history < 3))
    {
        delta = 1;
    }
    if (!taken && (history > 0))
    {
        delta = -1;
    }
  
    *branchHistory += delta;
}

VOID BIMODAL::PrintStats(int n, VOID *v)
{
    BIMODAL *bm = reinterpret_cast<BIMODAL*>(v);
    *(bm->_outfile) << endl;
    *(bm->_outfile) << "Icount: " << dec << bm->Icount() << " Mispredicts: " << dec << bm->Mispredicts() << endl;
}

VOID BIMODAL::Count(VOID *v)
{
    BIMODAL *bm = reinterpret_cast<BIMODAL*>(v);
    bm->_icount++;
    if(bm->_icount > bm->_nextPrintIcount)
    {
        bm->PrintStats(0,bm);
        bm->_nextPrintIcount += bm->_phaseLen; 
    }
}
