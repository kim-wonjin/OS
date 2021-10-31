# 1. oshw2

|                 |                                                              |
| --------------- | ------------------------------------------------------------ |
| Course | Operating System |
| Stack           | C                                                            |
| Topics          | `CPU Scheduling` |
| Specification | [Homework2](https://github.com/kim-wonjin/OS/blob/main/hw2/Homework2.pdf) |

<br/>

# 2. Summary

본 프로젝트는 프로세스의 상태전환(Process state transitions)과 여러가지 CPU scheduling 기법을 구현하여 시뮬레이션 하는것이다.

<br/>

# 3. Features

* 프로세스의 상태는 S_IDLE, S_RUNNING, S_READY, S_BLOCKED, S_TERMINATE 로 나누어 진다. 

* 시뮬레이션이 진행하는 동안에 다음과 같은 이벤트가 발생한다.

  * 프로세스의 생성

  * Quantum 만료

  * IO 요청

  * IO 처리가 끝남 (IO Done Event)

  * 종료(Terminate)

  * 상기 이벤트는 동시에 발생할 수 있다. 이 경우 다음과 같은 순서로 상태전환이 우선 된다. 종료 > Blocked > Ready

* 다음의 CPU 스케쥴링 알고리즘을 구현한다.

  * Round Robin Scheduling
  * Shortest Job First (Modified)
  * Shortes Remaining Time Next (SRTN)
  * Guaranteed Scheduling (Modified)
  * Simple Feedback Scheduling

<br/>


# 4. How to execute the program

프로그램은 다음과 같은 인자를 받아 실행된다.

> ./a.out SCHEDULING_METHOD NPROC NIOREQ QUANTUM MIN_INT_ARRTIME MAX_INT_ARRTIME MIN_SERVTIME MAX_SERVTIME MIN_IO_SERVTIME MAX_IO_SERVTIME MIN_IOREQ_INT_ARRTIME

> Round Robin Scheduling = 1, Shortest Job First = 2, Shortes Remaining Time Next = 3, Guaranteed Scheduling = 4, Simple Feedback Scheduling = 5


<br/>

# 5. Test result

```c
PARAM3 = All Schedule Methods + 10000 10000 50 10 1000 100 1000 1000 100000 10
***************************************************************************
../oshw2 - basic test 3 - ./a.out with [1 2 3 4 5] and 10000 10000 50   10 1000   100 1000   1000 100000  10
***************************************************************************
Next?

**** Running Simulation SCHTYPE = 1 ****
SIMULATION PARAMETERS : SCHEDULING_METHOD 1 NPROC 10000 NIOREQ 10000 QUANTUM 50
MIN_INT_ARRTIME 10 MAX_INT_ARRTIME 1000 MIN_SERVTIME 100 MAX_SERVTIME 1000
MIN_IO_SERVTIME 1000 MAX_IO_SERVTIME 100000 MIN_IOREQ_INT_ARRTIME 10
IO Req Average InterArrival Time 547
IO Req InterArrival Time range : 10 ~ 1084
Avg Proc Inter Arrival Time : 507.559 	Average Proc Service Time : 547.798
Avg IOReq Inter Arrival Time : 547.72 	Average IOReq Service Time : 50696
10000 Process processed with 10000 IO requests
Average Wall Clock Service Time : 332642 	Average Two Register Sum Value 5.48581e+06

**** Running Simulation SCHTYPE = 2 ****
SIMULATION PARAMETERS : SCHEDULING_METHOD 2 NPROC 10000 NIOREQ 10000 QUANTUM 50
MIN_INT_ARRTIME 10 MAX_INT_ARRTIME 1000 MIN_SERVTIME 100 MAX_SERVTIME 1000
MIN_IO_SERVTIME 1000 MAX_IO_SERVTIME 100000 MIN_IOREQ_INT_ARRTIME 10
IO Req Average InterArrival Time 547
IO Req InterArrival Time range : 10 ~ 1084
Avg Proc Inter Arrival Time : 507.559 	Average Proc Service Time : 547.798
Avg IOReq Inter Arrival Time : 547.72 	Average IOReq Service Time : 50696
10000 Process processed with 10000 IO requests
Average Wall Clock Service Time : 158567 	Average Two Register Sum Value 5.48581e+06

**** Running Simulation SCHTYPE = 3 ****
SIMULATION PARAMETERS : SCHEDULING_METHOD 3 NPROC 10000 NIOREQ 10000 QUANTUM 50
MIN_INT_ARRTIME 10 MAX_INT_ARRTIME 1000 MIN_SERVTIME 100 MAX_SERVTIME 1000
MIN_IO_SERVTIME 1000 MAX_IO_SERVTIME 100000 MIN_IOREQ_INT_ARRTIME 10
IO Req Average InterArrival Time 547
IO Req InterArrival Time range : 10 ~ 1084
Avg Proc Inter Arrival Time : 507.559 	Average Proc Service Time : 547.798
Avg IOReq Inter Arrival Time : 547.72 	Average IOReq Service Time : 50696
10000 Process processed with 10000 IO requests
Average Wall Clock Service Time : 153293 	Average Two Register Sum Value 5.48581e+06

**** Running Simulation SCHTYPE = 4 ****
SIMULATION PARAMETERS : SCHEDULING_METHOD 4 NPROC 10000 NIOREQ 10000 QUANTUM 50
MIN_INT_ARRTIME 10 MAX_INT_ARRTIME 1000 MIN_SERVTIME 100 MAX_SERVTIME 1000
MIN_IO_SERVTIME 1000 MAX_IO_SERVTIME 100000 MIN_IOREQ_INT_ARRTIME 10
IO Req Average InterArrival Time 547
IO Req InterArrival Time range : 10 ~ 1084
Avg Proc Inter Arrival Time : 507.559 	Average Proc Service Time : 547.798
Avg IOReq Inter Arrival Time : 547.72 	Average IOReq Service Time : 50696
10000 Process processed with 10000 IO requests
Average Wall Clock Service Time : 2.3556e+06 	Average Two Register Sum Value 5.48581e+06

**** Running Simulation SCHTYPE = 5 ****
SIMULATION PARAMETERS : SCHEDULING_METHOD 5 NPROC 10000 NIOREQ 10000 QUANTUM 50
MIN_INT_ARRTIME 10 MAX_INT_ARRTIME 1000 MIN_SERVTIME 100 MAX_SERVTIME 1000
MIN_IO_SERVTIME 1000 MAX_IO_SERVTIME 100000 MIN_IOREQ_INT_ARRTIME 10
IO Req Average InterArrival Time 547
IO Req InterArrival Time range : 10 ~ 1084
Avg Proc Inter Arrival Time : 507.559 	Average Proc Service Time : 547.798
Avg IOReq Inter Arrival Time : 547.72 	Average IOReq Service Time : 50696
10000 Process processed with 10000 IO requests
Average Wall Clock Service Time : 740092 	Average Two Register Sum Value 5.48581e+06
```
