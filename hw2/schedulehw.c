// 
// 2021
// Operating System
// CPU Schedule Simulator Homework
// Student Number : B911040
// Name : 김원진
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>

#define SEED 10

// process states
#define S_IDLE 0
#define S_READY 1
#define S_BLOCKED 2
#define S_RUNNING 3
#define S_TERMINATE 4

int NPROC, NIOREQ, QUANTUM;

struct ioDoneEvent {
	int procid;
	int doneTime;
	int len;
	struct ioDoneEvent *prev;
	struct ioDoneEvent *next;
} *ioDoneEvent;

struct process {
	int id;
	int len;		// for queue
	int targetServiceTime;
	int serviceTime;
	int startTime;
	int endTime;
	char state;
	int priority;
	int saveReg0, saveReg1;
	struct process *prev;
	struct process *next;
} *procTable;

struct process idleProc;
struct process readyQueue;
struct process blockedQueue;
struct process *runningProc;
struct ioDoneEvent ioDoneEventQueue;

struct process *last = NULL;
struct ioDoneEvent *last_ = NULL;   //pointing last element
struct ioDoneEvent *ptr = NULL;

int cpuReg0, cpuReg1;
int currentTime = 0;
int *procIntArrTime, *procServTime, *ioReqIntArrTime, *ioServTime;

void compute() {
	// DO NOT CHANGE THIS FUNCTION
	cpuReg0 = cpuReg0 + runningProc->id;
	cpuReg1 = cpuReg1 + runningProc->id;
	//printf("In computer proc %d cpuReg0 %d\n",runningProc->id,cpuReg0);
}

void initProcTable() {
	int i;
	for(i=0; i < NPROC; i++) {
		procTable[i].id = i;
		procTable[i].len = 0;
		procTable[i].targetServiceTime = procServTime[i];
		procTable[i].serviceTime = 0;
		procTable[i].startTime = 0;
		procTable[i].endTime = 0;
		procTable[i].state = S_IDLE;
		procTable[i].priority = 0;
		procTable[i].saveReg0 = 0;
		procTable[i].saveReg1 = 0;
		procTable[i].prev = NULL;
		procTable[i].next = NULL;
	}
}

void initidleProc(){
	idleProc.id = -1;
	idleProc.targetServiceTime = -1;
	idleProc.startTime = 0;
	idleProc.serviceTime = 0;
	idleProc.state = S_IDLE;
}

void initioDoneEvent() {
	int i;
	for(i=0; i < NIOREQ; i++)
	{
		ioDoneEvent[i].procid = -1;
		ioDoneEvent[i].doneTime = INT_MAX;
		ioDoneEvent[i].len = 0;
		ioDoneEvent[i].prev = NULL;
		ioDoneEvent[i].next = NULL;
	}
}

void Enqueue(struct process *quename, struct process *proc_to_add)
{
	quename->len++;
	if (quename->next == quename)// first item
	{
		quename->next = proc_to_add;
		proc_to_add->prev = quename;
		last = quename->next;
	}
	else
	{
		proc_to_add->prev = last;
		last->next = proc_to_add;
		last = last->next;
	}
}

void Dequeue(struct process *quename, struct process *proc_to_rm)
{
	quename->len--;
	if (proc_to_rm->next == NULL && proc_to_rm->prev == quename)// 1 item left
	{
		quename->next = quename->prev = quename;
		last = NULL;
	}
	else if (proc_to_rm->prev == quename)//first
	{
		proc_to_rm->next->prev = quename;
		quename->next = proc_to_rm->next;
	}
	else if (proc_to_rm->next == NULL)//last
	{
		proc_to_rm->prev->next = NULL;
		last = proc_to_rm->prev;
	}
	else if (proc_to_rm->next == NULL && proc_to_rm->prev == NULL)
		printf("error\n");
	else 
	{
		proc_to_rm->prev->next = proc_to_rm->next;
		proc_to_rm->next->prev = proc_to_rm->prev;
	}
	proc_to_rm->next = NULL;
	proc_to_rm->prev = NULL;
}

void procExecSim(struct process *(*scheduler)()) {
	int pid, qTime=0, cpuUseTime = 0, nproc=0, termProc = 0, nioreq=0; 
	char schedule = 0, nextState = S_IDLE;
	int nextForkTime, nextIOReqTime;
	int runningProc_to_ready = 0;

	nextForkTime = procIntArrTime[nproc];
	nextIOReqTime = ioReqIntArrTime[nioreq];
	initioDoneEvent();
	initidleProc();
	runningProc = &idleProc;

	while(1) {
		currentTime++;
		qTime++;
		runningProc->serviceTime++;
		runningProc_to_ready = 0;
		schedule = 0;
		
		if (runningProc != &idleProc )
		{
			cpuUseTime++;
			compute();
			runningProc->saveReg0 = cpuReg0;
			runningProc->saveReg1 = cpuReg0;
		}
		// MUST CALL compute() Inside While loop
		if (currentTime == nextForkTime) { /* CASE 2 : a new process created */
	//		printf("new! : %d\n", nproc);
			procTable[nproc].state = S_READY;
			Enqueue(&readyQueue, &procTable[nproc]);
			procTable[nproc].startTime = currentTime;

			if (runningProc->state == S_RUNNING)
				runningProc_to_ready = 1;
			if (nproc < NPROC - 1)
				nextForkTime += procIntArrTime[++nproc];
		}

		if (qTime == QUANTUM ) { /* CASE 1 : The quantum expires */
		//	printf("quantum!\n");
			if (runningProc->state == S_RUNNING)
			{
				runningProc->priority--; //cpubound
				runningProc_to_ready = 1;
			}
		}

		ptr = ioDoneEventQueue.next;

		while(ptr != NULL && ptr != &ioDoneEventQueue)  /* CASE 3 : IO Done Event */
		{
			if(ptr->doneTime == currentTime)
			{
				pid = ptr->procid;
			//	printf("iodone! : %d\n",pid);
				if (procTable[pid].state == S_BLOCKED)
				{
					procTable[pid].state = S_READY;
					Enqueue(&readyQueue, &procTable[pid]);
				}
				//dequeue iodone
				struct ioDoneEvent * deq = ptr;
				ptr = ptr->prev;
				deq->procid = -1;
				deq->doneTime = INT_MAX;
				ioDoneEventQueue.len--;
				
				if (deq->next == NULL && deq->prev == &ioDoneEventQueue)//1 item left
				{
					ioDoneEventQueue.next = ioDoneEventQueue.prev = &ioDoneEventQueue;
					last_ = NULL;
				}
				else if (deq->prev == &ioDoneEventQueue)//first
				{
					deq->next->prev = &ioDoneEventQueue;
					ioDoneEventQueue.next = deq->next;
				}
				else if (deq->next == NULL)//last
				{
						deq->prev->next = NULL;
						last_ = deq->prev;
				}
				else
				{
					deq->prev->next = deq->next;
					deq->next->prev = deq->prev;
				}
				deq->next = NULL;
				deq->prev = NULL;

				//runningProc ready
				if (runningProc->state == S_RUNNING)
				runningProc_to_ready = 1;
			}
			ptr = ptr->next;
		}

		if (cpuUseTime == nextIOReqTime) { /* CASE 5: reqest IO operations (only when the process does not terminate) */
		//	printf("ioreq! : %d\n",runningProc->id);
			if (qTime < QUANTUM)
				runningProc->priority++;
			runningProc->state = S_BLOCKED;
			ioDoneEvent[nioreq].procid = runningProc->id;
			runningProc_to_ready = 0;
			ioDoneEvent[nioreq].doneTime = currentTime + ioServTime[nioreq];
			ioDoneEvent->len++;
			//enqueue iodone
			if (ioDoneEventQueue.next == &ioDoneEventQueue) //first new item
			{
				ioDoneEventQueue.next = &ioDoneEvent[nioreq];
				ioDoneEvent[nioreq].prev = &ioDoneEventQueue;
				last_ = ioDoneEventQueue.next;
			}
			else
			{
				ioDoneEvent[nioreq].prev = last_;
				last_->next = &ioDoneEvent[nioreq];
				last_ = last_->next;
			}
			if (nioreq < NIOREQ - 1)
				nextIOReqTime += ioReqIntArrTime[++nioreq];
			else
				nextIOReqTime = 0;
		}

		if (runningProc->serviceTime == runningProc->targetServiceTime) { /* CASE 4 : the process job done and terminates */
	//		printf("terminate! : %d\n", runningProc->id);
			runningProc->state = S_TERMINATE;
			runningProc->endTime = currentTime;
			termProc++;
			runningProc_to_ready = 0;
		}

		if (runningProc_to_ready == 1)
		{
			runningProc->state = S_READY;
			Enqueue(&readyQueue,runningProc);			
		}

		// call scheduler() if needed
		if (runningProc->state != S_RUNNING)
		{
			if (readyQueue.next == &readyQueue)//empty
				runningProc = &idleProc;
			else
			{
				runningProc = scheduler();
				runningProc->state = S_RUNNING;
				qTime = 0;
				cpuReg0 = runningProc->saveReg0;
				cpuReg1 = runningProc->saveReg1;
				Dequeue(&readyQueue, runningProc);
			}
		}
		if (termProc == NPROC) //all process terminated
			break;
	} // while loop
}

//RR,SJF(Modified),SRTN,Guaranteed Scheduling(modified),Simple Feed Back Scheduling
//
struct process* RRschedule() {
	return (readyQueue.next);
}

struct process* SJFschedule() {
	struct process* p = readyQueue.next;
	struct process* ret =  readyQueue.next;
	while (p != NULL)
	{
		if (ret->targetServiceTime > p->targetServiceTime)
			ret = p;
		p = p->next;
	}
	return (ret);
}

struct process* SRTNschedule() {
	struct process* p = readyQueue.next;
	struct process* ret = readyQueue.next;
	while (p != NULL)
		{
			if (ret->targetServiceTime - ret->serviceTime > p->targetServiceTime - p->serviceTime)
			ret = p;
			p = p->next;
		}
		return (ret);
}

struct process* GSschedule() {
	struct process* p = readyQueue.next;
	struct process* ret = readyQueue.next;
	while (p != NULL)
	{
		if ((float)ret->serviceTime / (float)ret->targetServiceTime > (float)p->serviceTime / (float)p->targetServiceTime)
			ret = p;
		p = p->next;
	}
	return (ret);
}

struct process* SFSschedule() {
	struct process* p = readyQueue.next;
	struct process* ret = readyQueue.next;
	while (p != NULL)
	{
		if (ret->priority < p->priority)
			ret = p;
		p = p->next;
	}
	return (ret);
}

void printResult() {
	// DO NOT CHANGE THIS FUNCTION
	int i;
	long totalProcIntArrTime=0,totalProcServTime=0,totalIOReqIntArrTime=0,totalIOServTime=0;
	long totalWallTime=0, totalRegValue=0;
	for(i=0; i < NPROC; i++) {
		totalWallTime += procTable[i].endTime - procTable[i].startTime;
		/*
		printf("proc %d serviceTime %d targetServiceTime %d saveReg0 %d\n",
			i,procTable[i].serviceTime,procTable[i].targetServiceTime, procTable[i].saveReg0);
		*/
		totalRegValue += procTable[i].saveReg0+procTable[i].saveReg1;
		/* printf("reg0 %d reg1 %d totalRegValue %d\n",procTable[i].saveReg0,procTable[i].saveReg1,totalRegValue);*/
	}
	for(i = 0; i < NPROC; i++ ) {
		totalProcIntArrTime += procIntArrTime[i];
		totalProcServTime += procServTime[i];
	}
	for(i = 0; i < NIOREQ; i++ ) {
		totalIOReqIntArrTime += ioReqIntArrTime[i];
		totalIOServTime += ioServTime[i];
	}

	printf("Avg Proc Inter Arrival Time : %g \tAverage Proc Service Time : %g\n", (float) totalProcIntArrTime/NPROC, (float) totalProcServTime/NPROC);
	printf("Avg IOReq Inter Arrival Time : %g \tAverage IOReq Service Time : %g\n", (float) totalIOReqIntArrTime/NIOREQ, (float) totalIOServTime/NIOREQ);
	printf("%d Process processed with %d IO requests\n", NPROC,NIOREQ);
	printf("Average Wall Clock Service Time : %g \tAverage Two Register Sum Value %g\n", (float) totalWallTime/NPROC, (float) totalRegValue/NPROC);

}

int main(int argc, char *argv[]) {
	// DO NOT CHANGE THIS FUNCTION
	int i;
	int totalProcServTime = 0, ioReqAvgIntArrTime;
	int SCHEDULING_METHOD, MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME, MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME;

	if (argc < 12) {
		printf("%s: SCHEDULING_METHOD NPROC NIOREQ QUANTUM MIN_INT_ARRTIME MAX_INT_ARRTIME MIN_SERVTIME MAX_SERVTIME MIN_IO_SERVTIME MAX_IO_SERVTIME MIN_IOREQ_INT_ARRTIME\n",argv[0]);
		exit(1);
	}

	SCHEDULING_METHOD = atoi(argv[1]);
	NPROC = atoi(argv[2]);
	NIOREQ = atoi(argv[3]);
	QUANTUM = atoi(argv[4]);
	MIN_INT_ARRTIME = atoi(argv[5]);
	MAX_INT_ARRTIME = atoi(argv[6]);
	MIN_SERVTIME = atoi(argv[7]);
	MAX_SERVTIME = atoi(argv[8]);
	MIN_IO_SERVTIME = atoi(argv[9]);
	MAX_IO_SERVTIME = atoi(argv[10]);
	MIN_IOREQ_INT_ARRTIME = atoi(argv[11]);

	printf("SIMULATION PARAMETERS : SCHEDULING_METHOD %d NPROC %d NIOREQ %d QUANTUM %d \n", SCHEDULING_METHOD, NPROC, NIOREQ, QUANTUM);
	printf("MIN_INT_ARRTIME %d MAX_INT_ARRTIME %d MIN_SERVTIME %d MAX_SERVTIME %d\n", MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME);
	printf("MIN_IO_SERVTIME %d MAX_IO_SERVTIME %d MIN_IOREQ_INT_ARRTIME %d\n", MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME);

	srandom(SEED);

	// allocate array structures
	procTable = (struct process *) malloc(sizeof(struct process) * NPROC);
	ioDoneEvent = (struct ioDoneEvent *) malloc(sizeof(struct ioDoneEvent) * NIOREQ);
	procIntArrTime = (int *) malloc(sizeof(int) * NPROC);
	procServTime = (int *) malloc(sizeof(int) * NPROC);
	ioReqIntArrTime = (int *) malloc(sizeof(int) * NIOREQ);
	ioServTime = (int *) malloc(sizeof(int) * NIOREQ);

	// initialize queues
	readyQueue.next = readyQueue.prev = &readyQueue;
	blockedQueue.next = blockedQueue.prev = &blockedQueue;
	ioDoneEventQueue.next = ioDoneEventQueue.prev = &ioDoneEventQueue;
	ioDoneEventQueue.doneTime = INT_MAX;
	ioDoneEventQueue.procid = -1;
	ioDoneEventQueue.len  = readyQueue.len = blockedQueue.len = 0;

	// generate process interarrival times
	for(i = 0; i < NPROC; i++ ) {
		procIntArrTime[i] = random()%(MAX_INT_ARRTIME - MIN_INT_ARRTIME+1) + MIN_INT_ARRTIME;
	}

	// assign service time for each process
	for(i=0; i < NPROC; i++) {
		procServTime[i] = random()% (MAX_SERVTIME - MIN_SERVTIME + 1) + MIN_SERVTIME;
		totalProcServTime += procServTime[i];
	}

	ioReqAvgIntArrTime = totalProcServTime/(NIOREQ+1);
	assert(ioReqAvgIntArrTime > MIN_IOREQ_INT_ARRTIME);

	// generate io request interarrival time
	for(i = 0; i < NIOREQ; i++ ) {
		ioReqIntArrTime[i] = random()%((ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+1) + MIN_IOREQ_INT_ARRTIME;
	}

	// generate io request service time
	for(i = 0; i < NIOREQ; i++ ) {
		ioServTime[i] = random()%(MAX_IO_SERVTIME - MIN_IO_SERVTIME+1) + MIN_IO_SERVTIME;
	}

#ifdef DEBUG
	// printing process interarrival time and service time
	printf("Process Interarrival Time :\n");
	for(i = 0; i < NPROC; i++ ) {
		printf("%d ",procIntArrTime[i]);
	}
	printf("\n");
	printf("Process Target Service Time :\n");
	for(i = 0; i < NPROC; i++ ) {
		printf("%d ",procTable[i].targetServiceTime);
	}
	printf("\n");
#endif

	// printing io request interarrival time and io request service time
	printf("IO Req Average InterArrival Time %d\n", ioReqAvgIntArrTime);
	printf("IO Req InterArrival Time range : %d ~ %d\n",MIN_IOREQ_INT_ARRTIME,
			(ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+ MIN_IOREQ_INT_ARRTIME);

#ifdef DEBUG
	printf("IO Req Interarrival Time :\n");
	for(i = 0; i < NIOREQ; i++ ) {
		printf("%d ",ioReqIntArrTime[i]);
	}
	printf("\n");
	printf("IO Req Service Time :\n");
	for(i = 0; i < NIOREQ; i++ ) {
		printf("%d ",ioServTime[i]);
	}
	printf("\n");
#endif

	struct process* (*schFunc)();
	switch(SCHEDULING_METHOD) {
		case 1 : schFunc = RRschedule; break;
		case 2 : schFunc = SJFschedule; break;
		case 3 : schFunc = SRTNschedule; break;
		case 4 : schFunc = GSschedule; break;
		case 5 : schFunc = SFSschedule; break;
		default : printf("ERROR : Unknown Scheduling Method\n"); exit(1);
	}
	initProcTable();
	procExecSim(schFunc);
	printResult();

}
