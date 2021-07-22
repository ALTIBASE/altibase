/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <idl.h>
#include <iduStack.h>

#include <stdio.h>
/*% cc -g traceback.c -lexc
**% a.out
*/
struct exception_info;
/* To suppress compilation warnings from sym.h */
#include <demangle.h>
#include <excpt.h>
#include <setjmp.h>
#include <signal.h>
#include <sym.h>
#include <unistd.h>


static void printHeader(void)
{
    //printf("Diagnostic stack trace ...\n");
}


static void printTrailer(void)
{
    //printf("end of diagnostic stack trace.\n");
}


#define RETURNADDRREG (26)
#define FAULTING_ADDRESS sc_traparg_a0

extern unsigned long _procedure_string_table;
extern unsigned long _procedure_table_size;
extern unsigned long _procedure_table;
extern unsigned long _ftext;           /* Start address of the program text.*/
extern unsigned long _etext;           /* First address above the program text.*/



/* Turn a PC into the name of the routine containing that PC.
*/
char* programCounterToName(
    unsigned long programCounter);


/* Unwind the stack one level.
*/
void unwindOneLevel(
    struct sigcontext*  unwindSignalContext,
    void *              runtimeProcedurePtr);


void printFrame(
    void *        rpdp,
    unsigned long programCounter)
{
    char* procedureName;

    /* Get the procedure name.
    */
    procedureName = programCounterToName(programCounter);

    ideLog::logMessage(IDE_SERVER_0, "(%p) %s\n",
        (void* ) programCounter,  procedureName ? procedureName : "");
}


char* programCounterToName(
    unsigned long programCounter)
{
    long int          procedureTableSize;
    char *            procedureNameTable;
    char *            procedureName;
    long int          currentProcedure;
    long int          procedure;
    pRPDR             runTimePDPtr;
    int               found;
    unsigned long int address;

    found = 0;
    procedureName = 0;

    procedureTableSize = (long int)&_procedure_table_size;
    procedureNameTable = (char *)&_procedure_string_table;
    runTimePDPtr = (pRPDR)&_procedure_table;

    /* The table of run time procedure descriptors is ordered by
     procedure start address. Search the table (linearly) for the
     first procedure with a start address larger than the one for
     which we are looking.
    */
    for (currentProcedure = 0;
       currentProcedure < procedureTableSize;
       currentProcedure++) {

        /* Because of the way the linker builds the table we need to make
         special cases of the first and last entries. The linker uses 0
         for the address of the procedure in the first entry, here we
         substitute the start address of the text segment. The linker
         uses -1 for the address of the procedure in the last entry,
         here we substitute the end address of the text segment. For all
         other entries the procedure address is obtained from the table.*/

        if (currentProcedure == 0)                             /* first entry */
            address = (unsigned long int)&_ftext;
        else if (currentProcedure == procedureTableSize - 1)   /* last entry */
            address = (unsigned long int)&_etext;
        else                                                   /* other entries */
            address = runTimePDPtr[currentProcedure].adr;

        if (address > programCounter) {
            if (currentProcedure != 0) {
                /* the PC is in this image */
                found = 1;
                procedure = currentProcedure - 1; /* the PC is in preceeding procedure */
                break; /* stop searching */
            } else {
                /* the PC is outside this image (at a lower address) */
                break; /* stop searching */            }
        }
    }

    if (found) {
        /* the PC is inside this image */
        procedureName = &procedureNameTable[runTimePDPtr[procedure].irpss];
    } else {
        /* the PC is outside this image, probably in a different shared object */
        procedureName = 0;
    }

    return procedureName;
}


void unwindOneLevel(
    struct sigcontext *unwindSignalContext,
    void *             runtimeProcedurePtr)
{
    unwind(unwindSignalContext, (pdsc_crd *)runtimeProcedurePtr);
}


/* Print a stack trace.
*/
void printStackWkr(struct sigcontext *signalContextPtr)
{
    struct sigcontext unwindSignalContext;
    void *runTimeProcedurePtr;

    unsigned long int stackpointer    = 0;
    unsigned long int programCounter  = 0;
    unsigned long int returnAddress   = 0;
    unsigned long int faultingAddress = 0;

    printHeader();

    /* Set the initial context for the unwind.
    */
    unwindSignalContext = *signalContextPtr;

    /* Pick out the return address and program counter.
    */
    returnAddress  = unwindSignalContext.sc_regs[RETURNADDRREG];
    programCounter = unwindSignalContext.sc_pc;

    /* This is the address that caused the fault when we tried to access
     it.
    */
    faultingAddress = signalContextPtr->FAULTING_ADDRESS;

    /* Special cases for bogus program counter values. If the program
     counter is zero or the fault occurred when we were trying to
     fetch an instruction (because the program counter itself was bad)
     then we cannot unwind the stack.
    */
    if (programCounter == 0) {

        ideLog::logMessage(IDE_SERVER_0, "PC is zero - stack trace not available.\n");

    } else if (programCounter == faultingAddress) {

        ideLog::logMessage(IDE_SERVER_0, "bad PC (%p) - stack trace not available.\n",
            faultingAddress);

    } else {

        unsigned int sameSpCount = 0;

        /* Loop through all the stack frames.
        */
        while  ((returnAddress != 0) && (programCounter != 0)) {

            /* Get the run time procedure descriptor for this frame.
            */
            runTimeProcedurePtr = find_rpd(programCounter);

            /* Print a frame.
            */
            printFrame(runTimeProcedurePtr, programCounter);

            /* Unwind one level.
            */
            unwindOneLevel(&unwindSignalContext, runTimeProcedurePtr);
            returnAddress   = unwindSignalContext.sc_regs[RETURNADDRREG];
            programCounter  = unwindSignalContext.sc_pc;

            if (unwindSignalContext.sc_sp <= stackpointer) {
                sameSpCount++;
                if (sameSpCount == 10) break;
            } else {
                sameSpCount  = 0;
                stackpointer = unwindSignalContext.sc_sp;
            }
        }
    }

    printTrailer();
}

/* Discard one stack frame by silently unwinding.
*/
long int discardOneLevel(
    long int programCounter,
    struct sigcontext *signalContextPtr)
{
    void *runTimeProcedurePtr;

    runTimeProcedurePtr = find_rpd(programCounter);
    unwindOneLevel(signalContextPtr, runTimeProcedurePtr);
    return signalContextPtr->sc_pc;
}


void printStack(int levelsToDiscard)
{
    long int programCounter;
    void *runTimeProcedurePtr;
    jmp_buf context;
    struct sigcontext *signalContextPtr;

    /* Capture the current context.
    */
    setjmp(context);
    signalContextPtr = (struct sigcontext *)context;
    programCounter = signalContextPtr->sc_pc;

    /* Discard the frame for this routine.
    */
    programCounter = discardOneLevel(programCounter, signalContextPtr);

    /* Discard any other frames requested by our caller.
    */
    if (levelsToDiscard > 0) {
        int levels;
        for (levels = 0; levels < levelsToDiscard; levels++) {
            programCounter = discardOneLevel(programCounter, signalContextPtr);
        }
    }

    /* Now that we have the right context, print the stack trace.
    */
    printStackWkr(signalContextPtr);

}
