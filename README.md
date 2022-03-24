---
finished_date: 2020-10-11
tags:
    - OS
    - process_scheduling
    - multi-level_feedback_queue
    - feedback_queue
    - C
    - linux
---
# Process scheduling: Multi-level Feedback Queue
- Implement MFQ in C
- total four ready queues
    - priority: Q<sub>0</sub> > Q<sub>1</sub> > Q<sub>2</sub> > Q<sub>3</sub>
    - time slice
        - Q<sub>0</sub>: 2, Q<sub>1</sub>: 4, Q<sub>2</sub> 8, Q<sub>3</sub>: FCFS
- sheduling rules
    - while a time slice is started, it cannot be interrupt by other process  
    ex) if process A from Q<sub>1</sub> is running, process B from Q<sub>0</sub> cannot stop running. process B should wait until process A ends.
    - when a process is preempted, it is moved to Q<sub>i+1</sub> queue
    - if i/o burst time passed away, the process is moved to Q<sub>i-1</sub> queue
    - if cpu burst time passed away, the process becomes sleep state to fill i/o burst time
    - if cpu burst time is left and time slice is passed away, the process becomes preempt state.
- input file
    - the file includes **pid, arrival time, the number of cycle, cpu burst, i/o burst, initial Q of each process**
    - **the number of process** is also included
- output file
    - gantt chart.txt
    - **waiting time, turnaround time of each process** are included
    - **average waiting time, turnaround time of whoel process** are also included
## Solving strategy
- implement *QUEUE* data structure and make four ready Q
- save input file's information in *PROCESS* struct
- order processes in array of *PROCESS*
- check time in simulation program with variable *time*
- when *time==arrival time** push the process to proper ready Q
- when there are processes that have same arrival time and wake up time, newly arrived process is pushed first
- find ready Q that has process to be run in that time
- when finished, store *time* in *PROCESS* struct
- when simulation is ended, make gantt chart as txt file

detailed struct, function information is included in the [document file](https://github.com/dongwon18/MFQ_Scheduling_Simulation/blob/main/MFQ_scheduling_simulation_document.pdf).

## Result
- console output  
![image](https://user-images.githubusercontent.com/74483608/159843884-5c29d346-0b08-4afd-b563-94fe55d82ea3.png)
- file output  
![image](https://user-images.githubusercontent.com/74483608/159844030-a9f823ea-3f09-40e3-88d3-c0e8af7271aa.png)

```
|   1   |0 // process 0 is run between time 0 and 1
```
- when process or queue is changed, mark with ---
- wait: wait until process wakes up who is sleeling or wait for a process whose arrival time is not passed

## File structure
```
|-- data
    |-- input.txt
    |-- input1.txt
    |-- input2.txt
    |-- input3.txt
    |-- input4.txt
    |-- input5.txt
|-- src
    |-- main.c
|-- MFQ_sheduling_simulation_document.pdf
|-- gantt_chart.txt
```
## 배운 점
- multi level feedback queue를 직접 구현하면서 동일한 process를 다룰 때 예제로 접했던 다른 방식에 비해 CPU 효율이 평균적으로 높은 것을 확인할 수 있었다.
- OS에서 어떤 방식으로 자료구조를 사용하는지 간접적으로 체험할 수 있었다.
- queue, sort 등 프로젝트에 필요한 함수를 모두 직접 구현했기 때문에 자료구조를 구현하는 능력을 키울 수 있었다.
## 한계점
- random input을 생성하는 기능을 추가하여 더욱 다양한 input에 대해 관찰할 수 있었다면 더 좋았을 것이다.
- header 파일로 구현한 함수 혹은 자료구조를 처리하여 file을 나눠 관리하였다면 재사용성, 가독성과 관리의 측면에서 도움이 되었을 것이다.
