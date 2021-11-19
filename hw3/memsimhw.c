//
// Virual Memory Simulator Homework
// One-level page table system with FIFO and LRU
// Two-level page table system with LRU
// Inverted page table with a hashing system
// Submission Year: 2021
// Student Name: 김원진
// Student Number: B911040
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>


#define PAGESIZEBITS 12			// page size = 4Kbytes
#define VIRTUALADDRBITS 32		// virtual address space size = 4Gbytes

struct procEntry {
	char *traceName;			// the memory trace name
	int pid;					// process (trace) id
	int ntraces;				// the number of memory traces
	int num2ndLevelPageTable;	// The 2nd level page created(allocated);
	int numIHTConflictAccess; 	// The number of Inverted Hash Table Conflict Accesses
	int numIHTNULLAccess;		// The number of Empty Inverted Hash Table Accesses
	int numIHTNonNULLAcess;		// The number of Non Empty Inverted Hash Table Accesses
	int numPageFault;			// The number of page faults
	int numPageHit;				// The number of page hits
	int end;
	struct pageTableEntry *firstLevelPageTable;
	FILE *tracefp;
	int pageTable[0xfffff + 1][2];
};

struct pageFrame {
	int pid;
	unsigned vpn;
	int pfn;
	struct pageFrame *next;
};

struct procEntry *procTable;

struct pageFrame *frame_list;
struct pageFrame *frame_list_head;
struct pageFrame *frame_list_tail;

unsigned read_tracefp(int pid)
{
	unsigned addr;
	char rw;

	fscanf(procTable[pid].tracefp, "%x %c", &addr, &rw);	
	if (!feof(procTable[pid].tracefp))	
		return (addr);
	return (0);
}

void init_pageTable(int numProcess)
{
	int i, j;
	for(i = 0; i < numProcess; i++)
	{
		for(j = 0; j < 0xfffff + 1; j++)
		{
			procTable[i].pageTable[j][0] = 0;
			procTable[i].pageTable[j][1] = 0;
		}
	}
}

void lru(int hitted_pfn)
{
	struct pageFrame *p = frame_list_head;
	struct pageFrame *hitted = &frame_list[hitted_pfn];
	
	if (p->pfn == hitted_pfn) // head hit
		frame_list_head = frame_list_head->next;
	else
	{
		while (p->next != NULL && p->next->pfn != hitted_pfn)
			p = p->next;
	}
	
	p->next = hitted->next;
	frame_list_tail->next = hitted;
	frame_list_tail = frame_list_tail->next;
	frame_list_tail->next = NULL;
}

void update(int i, unsigned new_vpn)
{
	int backup_pid;
	unsigned backup_vpn;

	backup_vpn = frame_list_head->vpn;
	backup_pid = frame_list_head->pid;

	frame_list_head->pid = i;
	frame_list_head->vpn = new_vpn;
	procTable[i].pageTable[new_vpn][0] = frame_list_head->pfn;
	procTable[i].pageTable[new_vpn][1] = 1;

	//update head & tail	
	frame_list_tail->next = frame_list_head;
	frame_list_head = frame_list_head->next;
	frame_list_tail = frame_list_tail->next;
	frame_list_tail->next = NULL;	

	if (backup_pid != -1) //not first
	{
		procTable[backup_pid].pageTable[backup_vpn][0] = 0;
		procTable[backup_pid].pageTable[backup_vpn][1] = 0;
	}
}

void oneLevelVMSim(int simType, int numProcess) {
	unsigned va;
	int i;
	unsigned vpn;
	unsigned backup_vpn;
	int backup_pid;
	int finish = 0;

	init_pageTable(numProcess);

	while(1)
	{
		for (i= 0; i < numProcess; i++)
		{
			if (procTable[i].end == 1)
				continue;
			
			if (!(va = read_tracefp(i))) //EOF
			{
				finish ++;
				procTable[i].end = 1;
			}
			else
			{	
				procTable[i].ntraces++;
				vpn = (va & 0xfffff000) >> 12;

				if (procTable[i].pageTable[vpn][1] == 1) //hit
				{	
					procTable[i].numPageHit++;
					if (simType == 1)
					{	lru(procTable[i].pageTable[vpn][0]);	
						printf("%d\n",procTable[i].pageTable[vpn][0])	;	
					}
				}
				else // miss (fault)
				{
					procTable[i].numPageFault++;
					update(i,vpn);
				}
			}		
		}
		if (finish == numProcess)
			break;
	}

	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}

}
/*void twoLevelVMSim(...) {

	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

void invertedPageVMSim(...) {

	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Inverted Hash Table Access Conflicts %d\n",i,procTable[i].numIHTConflictAccess);
		printf("Proc %d Num of Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNULLAccess);
		printf("Proc %d Num of Non-Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNonNULLAcess);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		assert(procTable[i].numIHTNULLAccess + procTable[i].numIHTNonNULLAcess == procTable[i].ntraces);
	}
}*/


int main(int argc, char *argv[]) {
	int i,c, simType;
	int numProcess;
	int firstlevelBits;
	int phyMemSizeBits;
	int nFrame;

	simType = atoi(argv[1]);
	numProcess = argc - 4;
	firstlevelBits = atoi(argv[2]);
	phyMemSizeBits = atoi(argv[3]);
	nFrame = (1L<<phyMemSizeBits)/(1L<<12);

	if (!(procTable = malloc(sizeof(struct procEntry) * numProcess)))
		return (0);

	if (!(frame_list = malloc(sizeof(struct pageFrame) * nFrame)))
		return (0);

	for (i = 0; i < numProcess; i++) // initialize procTable for Memory Simulations
	{
		procTable[i].traceName = argv[i + optind + 3];
		procTable[i].pid = i;
		procTable[i].ntraces = 0;
		procTable[i].num2ndLevelPageTable = 0;
		procTable[i].numIHTConflictAccess = 0;
		procTable[i].numIHTNULLAccess = 0;
		procTable[i].numIHTNonNULLAcess = 0;
		procTable[i].numPageFault = 0;
		procTable[i].numPageHit = 0;
		procTable[i].end = 0;
		procTable[i].firstLevelPageTable = NULL;
	
	// opening a tracefile for the process
		printf("process %d opening %s\n",i,argv[i + optind + 3]);
		procTable[i].tracefp = fopen(argv[i + optind + 3],"r");
		if (procTable[i].tracefp == NULL) {
			printf("ERROR: can't open %s file; exiting...",argv[i+optind+3]);
			exit(1);
		}
	}
	for (i = 0; i < nFrame; i++) //initialize procPage (linked list)
	{
		frame_list[i].pid = -1;
		frame_list[i].vpn = 0;
		frame_list[i].pfn = i;
		
		if (i == 0)
		{
			frame_list_head = frame_list;
			frame_list_tail = frame_list;
		}
		else
		{
			frame_list_tail->next = &frame_list[i];
			frame_list_tail = frame_list_tail->next; 
		}
	}
	frame_list_tail->next = NULL;
	
	
	
	printf("Num of Frames %d Physical Memory Size %ld bytes\n",nFrame, (1L<<phyMemSizeBits));

	if (simType == 0) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(simType, numProcess);
	}

	if (simType == 1) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(simType, numProcess);
	}

	if (simType == 2) {
		printf("=============================================================\n");
		printf("The Two-Level Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
	//	twoLevelVMSim(...);
	}

	if (simType == 3) {
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
	//	invertedPageVMSim(...);
	}

	for (i = 0; i < numProcess; i++)
		fclose(procTable[i].tracefp);

	free(procTable);
	free(frame_list);
	return(0);
}