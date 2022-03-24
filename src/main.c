#include <stdio.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0
#define SLICE0 2  // time slice of Q0
#define SLICE1 4  // time slice of Q1
#define SLICE2 8  // time slice of Q2

typedef enum __state{
    PREEMPT = 0,
    WAKEUP,
    SLEEP,
    FINISH,
    RUN,
}STATE;

typedef struct __process{
    int pid;
    int at;         //  arrival time
    int ft;         //  finish time
    int Q;          //  current Q
    int numCycle;   //  total no. of the burst cycle
    int *bc;        //  pointer array to store burst time
    int cpu_burst;  //  total cpu_burst time
    int io_burst;   //  total i/o burst time
    int wt;         //  waiting time
    int tt;         //  turnaround time
    int curCycle;   //  current burst cycle
}PROCESS;

// linked list of the PROCESS
typedef struct __node{
    PROCESS *data;
    struct __node *next;
} NODE;

typedef struct __queue{
    NODE *front;
    NODE *rear;
}QUEUE;

void Qinit(QUEUE *Q);
int isQempty(QUEUE *Q);
void EnQ(QUEUE *Q, PROCESS *x);
void Qprint(QUEUE *Q, int Qno);
PROCESS *DelQ(QUEUE *Q);
PROCESS **getP(int totalP, FILE *infile);
void sortQ(int totalP, PROCESS **plist);
void closeP(int totalP, PROCESS **plist);
void printQ(int totalP, PROCESS **plist);
void enterQ(int totalP, PROCESS **plist, int time, QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3);
void nxtQ(PROCESS *process, STATE state, QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3, QUEUE *readyQ , int *busy);
void calculatewaiting(int totalP, PROCESS **plist, double *avg_wt, double *avg_tt);
int wakeup(QUEUE *readyQ, int time, QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3, int *busy);
PROCESS * delCurNode(QUEUE *Q, int pid, NODE **position);
STATE findstate(int runtime, int time, QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3, QUEUE *readyQ, int Qno, int *busy);
int findnxtQ(QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3, int busy);
int runCPU(QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3, int Q, int changed, int *busy, int time, FILE *outfile);

/**
 * make Q0, Q1, Q2, Q3, readyQ and initialize
 * count time
 * count no. of finished process
 * get information from input file
 * check arrival and wake up
 * find next Q to run
 * print the element of the Qs
 * if all process is scheduled, calculate waiting time, turnaround time
 */
int main(){
    QUEUE Q0, Q1, Q2, Q3, readyQ;
    Qinit(&Q0);
    Qinit(&Q1);
    Qinit(&Q2);
    Qinit(&Q3);
    Qinit(&readyQ);
    int time = 0;

    FILE *infile = fopen("input.txt", "r");
    FILE *outfile = fopen("gantt_chart.txt", "w");  // output txt file that contains gantt chart
    int totalP;
    int fileread = fscanf(infile, "%d", &totalP);
    if(fileread == -1){
        printf("file opening failed\n");
        exit(-1);
    }
    PROCESS **plist = getP(totalP, infile);
    printQ(totalP, plist);
    sortQ(totalP, plist);
    printQ(totalP, plist);

    int finish_P = 0;
    int retwakeup;
    int busy = -1;
    int curQ = 0;
    int changed = 0;
    int runtime;
    STATE state;

    while(1){
        /*
         * check arrival process and push to proper Q
         * check wake up process and push to proper Q
         * if process wakes up, retwakeup == 1, else 0
         */
        enterQ(totalP, plist, time, &Q0, &Q1, &Q2, &Q3);
        retwakeup = wakeup(&readyQ, time, &Q0, &Q1, &Q2, &Q3, &busy);

        // if cpu is not running other process, find next Q
        // else, maintain current Q
        if( busy == -1){
            curQ = findnxtQ(&Q0, &Q1, &Q2, &Q3, busy);
        }
        // check if running process is changed
        if(state == PREEMPT || state == SLEEP ||state == FINISH){
            changed = 1;
        }else{
            changed = 0;
        }
        // if there is process to run or running process exists
        if(curQ != -1){
            runtime = runCPU(&Q0, &Q1, &Q2, &Q3, curQ, changed, &busy, time, outfile);
            state = findstate(runtime, time, &Q0, &Q1, &Q2, &Q3, &readyQ, curQ, &busy);
            printf("state %d\n", state);
            if(state == FINISH){
                finish_P++;
                busy = -1;
            }
        }else{
            busy = -1;
            // wait for wake up
            if(!isQempty(&readyQ)){
                printf("\n");
                printf("wait for wakeup\n");
                fprintf(outfile, "|  wait  |%d\n", time);
            }else{
                printf("finish Process %d\n", finish_P);
                // finish scheduling
                if(finish_P == totalP){
                    printf("all process done\n");
                    break;
                // wait for new process to arrive
                }else{
                    fprintf(outfile, "|  wait  |%d\n", time);
                    printf("wait for new process to come\n");
                }
            }
        }

        Qprint(&Q0, 0);
        Qprint(&Q1, 1);
        Qprint(&Q2, 2);
        Qprint(&Q3, 3);
        Qprint(&readyQ, 4);
        printf("<time> %d %d %d\n", time, retwakeup, finish_P);
        time++;
    }
    // calculate waiting time, turnaround time
    double avg_wt, avg_tt;
    calculatewaiting(totalP, plist, &avg_wt, &avg_tt);
    printQ(totalP, plist);
    fprintf(outfile, "average waiting time: %.2lf\naverage turnaround time: %.2lf\n", avg_wt, avg_tt);

    // free memory, close file
    closeP(totalP, plist);
    fclose(outfile);
    fclose(infile);
    return 0;
}

/**
 * find the state of the process in cpu
 * preempt
 * sleep
 * finish
 */
STATE findstate(int runtime, int time, QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3, QUEUE *readyQ, int Qno, int *busy){
    QUEUE *Q;
    STATE state;
    PROCESS *runP;
    // allocate runP
    if(Qno == 0){
        runP = Q0 -> front -> data;
        Q = Q0;
    }else if(Qno == 1){
        runP = Q1 -> front -> data;
        Q = Q1;
    }else if(Qno == 2){
        runP = Q2-> front -> data;
        Q = Q2;
    }else if(Qno == 3){
        runP = Q3 -> front -> data;
        Q = Q3;
    }

    // calculate current bc index
    int curCycle = runP -> curCycle;
    int bc_index = curCycle * 2 - 2;
    if(Qno == 0){
        if(runtime == SLICE0){
            if(runP -> bc[bc_index] > 0){
                state = PREEMPT;
            // cpu burst is not left
            }else{
                if(curCycle == runP -> numCycle){
                    runP -> ft = time + 1;
                    DelQ(Q);
                    state =  FINISH;
                }else{
                    state = SLEEP;
                }
            }
        // runtime < time slice
        }else{
           if(runP -> bc[bc_index] > 0){
                state = RUN;
            // cpu burst is not left
           }else{
               if(curCycle == runP -> numCycle){
                    runP -> ft = time + 1;
                    DelQ(Q);
                    state =  FINISH;
               }else{
                   state =  SLEEP;
               }
           }
        }
    }else if(Qno == 1){
        if(runtime == SLICE1){
            if(runP -> bc[bc_index] > 0){
                state = PREEMPT;
            }else{
                if(curCycle == runP -> numCycle){
                    runP -> ft = time + 1;
                    DelQ(Q);
                    state = FINISH;
                }else{
                    state = SLEEP;
                }
            }
        }else{
           if(runP -> bc[bc_index] > 0){
                state = RUN;
           }else{
               if(curCycle == runP -> numCycle){
                    runP -> ft = time + 1;
                    DelQ(Q);
                    state = FINISH;
               }else{
                   state = SLEEP;
               }
           }
        }
    }else if(Qno == 2){
        if(runtime == SLICE2){
            if(runP -> bc[bc_index] > 0){
                state = PREEMPT;
            }else{
                if(curCycle == runP -> numCycle){
                    runP -> ft = time + 1;
                    DelQ(Q);
                    state = FINISH;
                }else{
                    state = SLEEP;
                }
            }
        }else{
           if(runP -> bc[bc_index] > 0){
                state = RUN;
           }else{
               if(curCycle == runP -> numCycle){
                    runP -> ft = time + 1;
                    DelQ(Q);
                    state = FINISH;
               }else{
                   state = SLEEP;
               }
           }
        }
    }else if(Qno == 3){
        if(curCycle != runP -> numCycle){
            if(runP -> bc[bc_index] == 0){
                state = SLEEP;
            }else{
                state = RUN;
            }
        }else{
            if(runP -> bc[bc_index] == 0){
                runP -> ft = time;
                DelQ(Q);
                state = FINISH;
            }else{
                state = RUN;
            }
        }
    }

    // push to the next Q
    if(!isQempty(Q)){
        nxtQ(runP, state, Q0, Q1, Q2, Q3, readyQ, busy);
    }
    return state;
}

/**
 * find next Q to run
 * busy 0: process in Q0 is running
 *      1: process in Q1 is running
 *      2: process in Q2 is running
 *      3: process in Q3 is running
 *     -1: time slice is over or previously running process sleeps or finishes
 * when busy is i, only process in Qi can use cpu
 *              -1, all process can use cpu
 *
 * return Q number (int)
 * if no Q to be run, return -1
 */
int findnxtQ(QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3, int busy){
    if(!isQempty(Q0)&& (busy == -1 || busy == 0)){
        return 0;
    }else if(!isQempty(Q1)&& (busy == -1 || busy == 1)){
        return 1;
    }else if(!isQempty(Q2)&& (busy == -1 || busy == 2)){
        return 2;
    }else if(!isQempty(Q3)&& (busy == -1 || busy == 3)){
        return 3;
    }else{
        return -1;
    }
}

/**
 * find process to be run
 * set busy
 * increase runtime, decrease cpu burst time
 * print gantt chart to outfile
 * return runtime(int)
 */
int runCPU(QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3, int Q, int changed, int *busy, int time, FILE *outfile){
    PROCESS *runP;
    //  allocate running process and set busy
    if(Q == 0){
        runP = Q0 -> front -> data;
        *busy = 0;
    }else if(Q == 1){
        runP = Q1 -> front -> data;
        *busy = 1;
    }else if(Q == 2){
        runP = Q2-> front -> data;
        *busy = 2;
    }else if(Q == 3){
        runP = Q3 -> front -> data;
        *busy = 3;
    }else{
        *busy = -1;
    }
    int curCycle = runP -> curCycle;
    int bc_index = curCycle * 2 - 2;
    //int rest_time = runP -> bc[bc_index];
    static int runtime = 0;
    // running Q or process is changed
    if(changed == 1){
        runtime = 0;
        fprintf(outfile, "----------\n");
    }
    // increase run time, decrease cpu burst
    runtime++;
    runP -> bc[bc_index] -= 1;
    printf("Q%d %d runtime %d rest time %d\n", Q, runP -> pid, runtime, runP -> bc[bc_index]);
    fprintf(outfile, "|   %d   |%d\n", runP->pid, time);
    return runtime;
}

void calculatewaiting(int totalP, PROCESS **plist, double *avg_wt, double *avg_tt){
    double total_tt = 0.0;
    double total_wt = 0.0;
    for(int i = 0; i < totalP; i++){
        plist[i] -> tt = (plist[i] -> ft) - (plist[i] -> at);
        plist[i] -> wt = (plist[i] -> ft) - (plist[i] -> at) - (plist[i] -> cpu_burst) - (plist[i] ->io_burst);
        total_tt += plist[i] -> tt;
        total_wt += plist[i] -> wt;
    }
    *avg_wt = total_wt / totalP;
    *avg_tt = total_tt / totalP;
}

/**
 * if there is process that wakes up, return int 1
 * if readyQ is not empty,
 * decrease I/O burst rest time 1
 * if rest time == 0, put the process to proper next Q.
 *                    increase current cycle no.
 */
int wakeup(QUEUE *readyQ, int time, QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3, int *busy){
    int curCycle;
    int numburst;
    int bc_index;
    PROCESS *wakeup;
    NODE *cur = readyQ -> front;
    int state = 0;

    if(isQempty(readyQ)){
        return 0;
    }else{
        while(cur != NULL){
            curCycle = cur -> data -> curCycle;
            numburst = cur -> data -> numCycle * 2 - 3;
            bc_index = curCycle * 2 - 1;

            // not the last element of bc
            if(0< bc_index && bc_index <= numburst){
            printf("wakeup %d bc_index [%d] %d\n", cur-> data -> pid, bc_index, cur -> data->bc[bc_index]);
                // decrease the rest time
                if(cur -> data -> bc[bc_index] != 0){
                    cur -> data -> bc[bc_index] -= 1;
                // wake up
                }else{
                    // pop process that wakes up
                    wakeup = delCurNode(readyQ, cur -> data -> pid, &cur);
                    nxtQ(wakeup, WAKEUP, Q0, Q1, Q2, Q3, readyQ, busy);
                    wakeup -> curCycle += 1;
                    state = 1;
                }
            }
            if(cur == NULL){
                break;
            }
            cur = cur -> next;
            if(cur == NULL){
                break;
            }
        }
        return state;
    }
}

/**
 * find next Q
 * delete the process from current Q
 * push to the next Q
 * set busy
 *    if state is PREEMPT, SLEEP, set busy -1 (let all Q can use cpu)
 *                else, hold
 */
void nxtQ(PROCESS *process, STATE state, QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3, QUEUE *readyQ, int *busy){
    int Q = process -> Q;
    if(state == PREEMPT){
        switch(Q){
            case 3: DelQ(Q3); EnQ(Q3, process); process -> Q = 3; *busy = -1; break;
            case 2: DelQ(Q2); EnQ(Q3, process); process -> Q = 3; *busy = -1; break;
            case 1: DelQ(Q1); EnQ(Q2, process); process -> Q = 2; *busy = -1; break;
            default: DelQ(Q0); EnQ(Q1, process); process -> Q = 1; *busy = -1;
        }
    // deleting is already done in wakeup() by delCurNode()
    }else if(state == WAKEUP){
        switch(Q){
            case 3: EnQ(Q3, process); process -> Q = 3; break;
            case 2: EnQ(Q1, process); process -> Q = 1; break;
            case 1: EnQ(Q0, process); process -> Q = 0; break;
            default: EnQ(Q0, process); process -> Q = 0;
        }
    }else if(state == SLEEP){
        switch(Q){
            case 3: DelQ(Q3); EnQ(readyQ, process); *busy = -1; break;
            case 2: DelQ(Q2); EnQ(readyQ, process); *busy = -1; break;
            case 1: DelQ(Q1); EnQ(readyQ, process); *busy = -1; break;
            default: DelQ(Q0); EnQ(readyQ, process); *busy = -1;
        }
    // state == RUN
    }else{
        printf("\nRUN Q%d pid %d\n", Q, process -> pid);
    }
}

/**
 * delete the process whose i/0 burst = 0
 */
PROCESS *delCurNode(QUEUE *Q, int pid, NODE** position){
    NODE *cur = Q -> front;
    NODE *remove;
    PROCESS *data;
    if(cur == NULL){
        printf("no node to delete\n");
        exit(-1);
    // first is the node
    }else if(cur -> next == NULL){
        if(cur -> data ->pid == pid){
            data = DelQ(Q);
            cur = Q -> front;
            *position = cur;
            if(*position == NULL){
                    *position = NULL;
            }
            return data;
        }
    }else{
        // current process is the process
        if(cur -> data -> pid == pid){
            data = DelQ(Q);
            cur = Q -> front;
            *position  = cur;
            return data;
        }
        // search to 2nd last
        while(cur -> next -> next != NULL){
            if(cur -> next -> data -> pid == pid){
                remove = cur -> next;
                data = cur -> next -> data;
                cur -> next = cur -> next -> next;
                free(remove);
            }
            cur = cur -> next;
        }
        if(cur -> next -> next == NULL){
            // last is the node
            if(cur -> next -> data -> pid == pid){
                remove = cur -> next;
                data = cur -> next -> data;
                cur -> next = NULL;
                free(remove);
            }
        }
        return data;
    }
}

/**
 * push process at arrival time
 * sorted plist with arrival time (ascending order)
 */
void enterQ(int totalP, PROCESS **plist, int time, QUEUE *Q0, QUEUE *Q1, QUEUE *Q2, QUEUE *Q3){
    for(int i = 0; i < totalP; i++){
        if(plist[i] -> at == time){
            printf("time %d enter %d\n", time, plist[i] -> pid);
            switch(plist[i] -> Q){
                case 3: EnQ(Q3, plist[i]); plist[i] -> Q = 3; break;
                case 2: EnQ(Q2, plist[i]); plist[i] -> Q = 2; break;
                case 1: EnQ(Q1, plist[i]); plist[i] -> Q = 1; break;
                default: EnQ(Q0, plist[i]); plist[i] -> Q = 0;
            }
        }
    }
}

/**
 * sort process in the plist with arrival time(ascending order)
 */
void sortQ(int totalP, PROCESS **plist){
    for(int i = 0; i < totalP; i++){
        for(int j = 0; j < totalP - i - 1; j++){
            if(plist[j]->at > plist[j + 1] -> at ){
                PROCESS *temp = plist[j];
                plist[j] = plist[j + 1];
                plist[j + 1] = temp;
            }
        }
    }
}

/**
 * print all elements of the plist
 */
void printQ(int totalP, PROCESS **plist){
    printf("number of process: %d\n", totalP);
    for(int i = 0; i < totalP; i++){
        printf("\n pid %d\n", plist[i]->pid);
        printf("at: %d initQ: %d numcycle: %d cpu_burst: %d io_burst: %d waiting: %d\n finish: %d turnaround: %d\n",
               plist[i]->at, plist[i]->Q, plist[i]->numCycle, plist[i]->cpu_burst, plist[i] -> io_burst, plist[i]->wt, plist[i]->ft, plist[i] -> tt);
        printf("CPU - I/O burst\n");
        for(int j = 0; j < plist[i]->numCycle*2 - 1; j++){
            printf("%d ", plist[i]->bc[j]);
        }
        printf("\n\n");
    }
}

/**
 * get information from the input file
 * return pointer array
 */
PROCESS **getP(int totalP, FILE *infile){
    int pid;
    int at;
    int initQ;
    int numCycle;
    int *bc;            //  pointer array to store burst information
    int cpu_burst = 0;  //  total cpu burst of the process
    int io_burst = 0;   //  total i/o burst of the process
    int bc_temp;
    //  pointer array to store PROCESS type
    PROCESS **plist = (PROCESS **)malloc(sizeof(PROCESS *)*totalP);

    for(int i = 0; i < totalP; i++){
        //  memory allocation to a process
        plist[i] = (PROCESS *)malloc(sizeof(PROCESS));
        //  pid,arrival time, initial Q, no. of cycle, burst cycle
        fscanf(infile, "%d, %d, %d, %d, ", &pid, &at, &initQ, &numCycle);
        //  memory allocation to burst cycle array
        bc = (int *)malloc(sizeof(int)*(numCycle*2 - 1));
        for(int j = 0; j < numCycle*2 - 1; j++){
            // last cpu burst
            if(j == numCycle*2 - 2){
                fscanf(infile, "%d\n", &bc_temp);
                bc[j] = bc_temp;
            }else{
                fscanf(infile, "%d ", &bc_temp);
                bc[j] = bc_temp;
            }
        }

        for(int j = 0; j < numCycle*2 - 1; j = j + 2){
            cpu_burst += bc[j];
        }
        for(int j = 1; j < numCycle*2 - 2; j = j + 2){
            io_burst += bc[j];
        }
        plist[i] -> pid = pid;
        plist[i] -> at = at;
        plist[i] -> Q = initQ;
        plist[i] -> numCycle = numCycle;
        plist[i] -> bc = bc;
        plist[i] -> cpu_burst = cpu_burst;
        plist[i] -> io_burst = io_burst;
        plist[i] -> wt = 0;
        plist[i] -> curCycle = 1;
        plist[i] -> ft = -1;
        plist[i] -> tt = 0;
        cpu_burst = 0;
        io_burst = 0;
    }
    return plist;
}

void closeP(int totalP, PROCESS **plist){
    for(int i = 0; i < totalP; i++){
        free(plist[i] -> bc);  //  free bc array of each process
        free(plist[i]);        //  free each process
    }
    free(plist);  // free process array
}

void Qinit(QUEUE *Q){
    Q -> front = NULL;
    Q -> rear = NULL;
}

int isQempty(QUEUE *Q){
    if(Q -> front == NULL){
        return TRUE;
    }else{
        return FALSE;
    }
}

/**
 * perform push operation to Q
 */
void EnQ(QUEUE *Q, PROCESS *x){
    NODE *newnode = (NODE *)malloc(sizeof(NODE));  // free is node by DelQ
    newnode -> data = x;
    newnode -> next = NULL;
    // first node
    if(isQempty(Q)){
        Q -> front = newnode;
        Q -> rear = newnode;
    }else{
        Q -> rear -> next = newnode;
        Q -> rear = newnode;
    }
}

/**
 * perform delete operation to Q
 * return deleted process
 */
PROCESS *DelQ(QUEUE *Q){
    if(isQempty(Q)){
        printf("no element to delete\n");
        exit(-1);
    }else{
        PROCESS *data;
        NODE *remove = Q -> front;
        data = Q -> front -> data;
        Q -> front = Q -> front -> next;
        free(remove);
        return data;
    }
}

/**
 * print the pid in the Q
 */
void Qprint(QUEUE *Q, int Qno){
    NODE *cur = Q -> front;
    while(cur != NULL){
        printf("pid %d ", cur -> data-> pid);
        cur = cur -> next;
    }
    printf("\nfinish printing Q%d\n", Qno);
}
