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
	int ***first_pt;
};

struct pageFrame {
	int pid;
	unsigned vpn;
	unsigned first_pn;
	unsigned second_pn;
	int pfn;
	int hash_index;
	struct pageFrame *next;
	struct pageFrame *prev;
};

struct hash_node {
	int pid;
	unsigned vpn;
	int pfn;
	struct hash_node *next;
	struct hash_node *prev;
};

struct procEntry *procTable;

struct pageFrame *frame_list;
struct pageFrame *frame_list_head;
struct pageFrame *frame_list_tail;

struct hash_node **hashTable;

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
		for(j = 0; j < (1L << 20); j++)
		{
			procTable[i].pageTable[j][0] = 0;
			procTable[i].pageTable[j][1] = 0;
		}
	}
}

void move_head_to_tail()
{
	frame_list_tail->next = frame_list_head;
	frame_list_head->prev = frame_list_tail;
	frame_list_head = frame_list_head->next;
	frame_list_tail = frame_list_tail->next;
	frame_list_head->prev = NULL;
	frame_list_tail->next = NULL;	
}

void lru(int hitted_pfn)
{
	struct pageFrame * hitted = &frame_list[hitted_pfn];

	if (frame_list_tail->pfn == hitted_pfn) //tail hit
		return;
	else if (frame_list_head->pfn == hitted_pfn) //head hit
		move_head_to_tail();
	else
	{
		hitted->prev->next = hitted->next;
		hitted->next->prev = hitted->prev;

		frame_list_tail->next = hitted;
		hitted->prev = frame_list_tail;
		frame_list_tail = frame_list_tail->next;
		frame_list_tail->next = NULL;
	}
}

void update(int i, unsigned new_vpn)
{
	//invalid
	if (frame_list_head->pid != -1) 
	{
		procTable[frame_list_head->pid].pageTable[frame_list_head->vpn][0] = 0;
		procTable[frame_list_head->pid].pageTable[frame_list_head->vpn][1] = 0;
	}
	//allocated
	frame_list_head->pid = i;
	frame_list_head->vpn = new_vpn;
	procTable[i].pageTable[new_vpn][0] = frame_list_head->pfn;
	procTable[i].pageTable[new_vpn][1] = 1;
	
	move_head_to_tail();		
}

void oneLevelVMSim(int simType, int numProcess) {
	unsigned va;
	int i;
	unsigned vpn;
	int finish = 0;

	init_pageTable(numProcess);

	while(1)
	{
		for (i= 0; i < numProcess; i++)
		{
			if (procTable[i].end == 1)
				continue;
				
			if (!(va = read_tracefp(i)))	
			{
				finish ++;
				procTable[i].end = 1;
			}
			else
			{	
				procTable[i].ntraces++;
				vpn = va  >> 12;

				if (procTable[i].pageTable[vpn][1] == 1) //hit
				{	
					procTable[i].numPageHit++;
					if (simType == 1)
						lru(procTable[i].pageTable[vpn][0]);		
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

void init_first_page_table(int numProcess, unsigned first_page_size)
{
	int i, j;

	for(i = 0; i < numProcess; i++)
	{
		if (!(procTable[i].first_pt = malloc(sizeof(int **) * first_page_size)))
			exit(1);
		for (j = 0; j < first_page_size; j++)
			procTable[i].first_pt[j] = NULL;
	}
}

void init_second_page_table(int pid, unsigned first_pn, unsigned second_page_size)
{
	int i;
	
	if (!(procTable[pid].first_pt[first_pn] = malloc(sizeof(int *) * second_page_size)))
			exit(1);
	for (i = 0; i < second_page_size; i++)
	{
		if (!(procTable[pid].first_pt[first_pn][i] = malloc(sizeof(int) * 2)))
			exit(1);
		procTable[pid].first_pt[first_pn][i][0] = 0;
		procTable[pid].first_pt[first_pn][i][1] = 0;
	}	
}

void update_twolevel(int i, unsigned new_first_pn, unsigned new_second_pn)
{
	if (frame_list_head->pid != -1) //invalid
	{
		procTable[frame_list_head->pid].first_pt[frame_list_head->first_pn][frame_list_head->second_pn][0] = 0;
		procTable[frame_list_head->pid].first_pt[frame_list_head->first_pn][frame_list_head->second_pn][1] = 0;
	}
	//allocated
	frame_list_head->pid = i;
	frame_list_head->first_pn = new_first_pn;
	frame_list_head->second_pn = new_second_pn;
	procTable[i].first_pt[new_first_pn][new_second_pn][0] = frame_list_head->pfn;
	procTable[i].first_pt[new_first_pn][new_second_pn][1] = 1;
	
	move_head_to_tail();
}

void twoLevelVMSim(int numProcess, int firstlevelBits) {
	unsigned va;
	int i;
	unsigned first_pn, second_pn;
	int finish = 0;

	unsigned first_page_size =  1L << firstlevelBits;
	unsigned second_page_size = 1L << (20 - firstlevelBits);

	init_first_page_table(numProcess, first_page_size);

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
				first_pn = va >> (32 - firstlevelBits);  
				second_pn = (va << firstlevelBits) >> (firstlevelBits+12);

				if (procTable[i].first_pt[first_pn] != NULL && procTable[i].first_pt[first_pn][second_pn][1] == 1) //hit
				{	
					procTable[i].numPageHit++;
					lru(procTable[i].first_pt[first_pn][second_pn][0]);		
				}
				else // miss (fault)
				{
					procTable[i].numPageFault++;
					if (procTable[i].first_pt[first_pn] == NULL)
					{
						init_second_page_table(i, first_pn, second_page_size); //create 2nd tabel
						procTable[i].num2ndLevelPageTable++;
					}
					update_twolevel(i, first_pn, second_pn);
				}
			}		
		}
		if (finish == numProcess)
			break;
	}

	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

void init_hashTable(int nFrame)
{
	int i;

	if(!(hashTable = malloc(sizeof(struct hash_node *) * nFrame)))
		exit(1);
	for (i = 0; i < nFrame; i++)
		hashTable[i] = NULL;
}

int search_table(int i, int index, unsigned new_vpn)
{
	struct hash_node * p = hashTable[index];
	
	if (p == NULL) // empty
	{
		procTable[i].numIHTNULLAccess++;
		return (-1);
	}
	
	procTable[i].numIHTNonNULLAcess++;

	while(p != NULL)
	{
		procTable[i].numIHTConflictAccess++;
		if (p->vpn == new_vpn && p->pid == i)
			return (p->pfn);
		p = p->next;
	}
	return (-1);
}

void add_new_node(int i, int index, unsigned new_vpn)
{
	struct hash_node * new;

	if (!(new = malloc (sizeof(struct hash_node))))
		exit(1);
	new->pid = i;
	new->vpn = new_vpn;
	new->pfn = frame_list_head->pfn;
	new->prev = NULL;
	if (hashTable[index] == NULL) //empty
	{
		hashTable[index] = new;
		new->next = NULL;
	}
	else
	{
		new->next = hashTable[index];
		hashTable[index]->prev = new;
		hashTable[index] = new;

	}
}

void delete_node(struct hash_node * p)
{
	if (p->prev == NULL && p->next == NULL) //only one node
		hashTable[frame_list_head->hash_index] = NULL;
	else if (p->prev == NULL) //first node
	{
		hashTable[frame_list_head->hash_index] = p->next;
		p->next->prev = NULL;
	}
	else if (p->next == NULL) //last node
		p->prev->next = NULL;
	
	else 
	{
		p->prev->next = p->next;
		p->next->prev = p->prev;
	}
	free(p);
}

void update_inverted(int new_index, int i, unsigned new_vpn)
{
	//invalid
	if (frame_list_head->pid != -1) 
	{
		struct hash_node * p = hashTable[frame_list_head->hash_index];
		while (p->pfn != frame_list_head->pfn)
			p = p->next;
		delete_node(p);
	}
	//allocated
	frame_list_head->pid = i;
	frame_list_head->vpn = new_vpn;
	frame_list_head->hash_index = new_index;
	add_new_node(i, new_index, new_vpn);

	move_head_to_tail();	
}

void invertedPageVMSim(int numProcess, int nFrame) {
	unsigned va;
	int i;
	unsigned vpn;
	int hash_index;
	int pfn;
	int finish = 0;

	init_hashTable(nFrame);

	while(1)
	{
		for (i= 0; i < numProcess; i++)
		{
			if (procTable[i].end == 1)
				continue;
				
			if (!(va = read_tracefp(i)))	
			{
				finish ++;
				procTable[i].end = 1;
			}
			else
			{	
				procTable[i].ntraces++;
				vpn = va  >> 12;
				hash_index = ((int)vpn + i)% nFrame;

				if ((pfn = search_table(i, hash_index, vpn)) != -1) //hit
				{	
					procTable[i].numPageHit++;
					lru(pfn);		
				}
				else // miss (fault)
				{
					procTable[i].numPageFault++;
					update_inverted(hash_index, i, vpn);
				}
			}		
		}
		if (finish == numProcess)
			break;
	}

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
}


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
		frame_list[i].first_pn = 0;
		frame_list[i].second_pn = 0;
		frame_list[i].pfn = i;
		frame_list[i].hash_index = -1;
		
		if (i == 0)
		{
			frame_list_head = frame_list;
			frame_list_tail = frame_list;
			frame_list_head->prev = NULL;
		}
		else
		{
			frame_list_tail->next = &frame_list[i];
			frame_list_tail->next->prev = frame_list_tail;
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
		twoLevelVMSim(numProcess, firstlevelBits);
	}

	if (simType == 3) {
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		invertedPageVMSim(numProcess, nFrame);
	}

	for (i = 0; i < numProcess; i++)
		fclose(procTable[i].tracefp);

	free(procTable);
	free(frame_list);
	return(0);
}