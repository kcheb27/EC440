#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <assert.h>
#include "ec440threads.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>


#define MAX_THREADS 128			/* number of threads you support */
#define THREAD_STACK_SIZE (1<<15)	/* size of stack in bytes */
#define QUANTUM (50 * 1000)		/* quantum in usec */
/* 
   Thread_status identifies the current state of a thread. What states could a thread be in?
   Values below are just examples you can use or not use. 
 */
enum thread_status
{
 TS_EXITED,
 TS_RUNNING,
 TS_READY,
 TS_UNIT
};
/* The thread control block stores information about a thread. You will
 * need one of this per thread. What information do you need in it? 
 * Hint, remember what information Linux maintains for each task?
 */
struct thread_control_block {
  enum thread_status Status;
  jmp_buf buff;
  pthread_t id;
  void *stack;
  void * exit;
};
/// create TCB GLOBAL Table
static struct thread_control_block tcb_arr[MAX_THREADS];
static int curr_thread = 0;
static int thread_cnt = 0;

static void schedule(int signal)
{
  /* 
     TODO: implement your round-robin scheduler, e.g., 
     - if whatever called us is not exiting 
       - mark preempted thread as runnable
       - save state of preempted thread
     - determin which thread should be running next
     - mark thread you are context switching to as running
     - restore registers of that thread
   */
  //printf("scheduling");
  //switch status of current thread
  tcb_arr[curr_thread].Status = TS_READY;
  //update to next available thread.

   int next = ((curr_thread + 1) % thread_cnt);
   if(setjmp(tcb_arr[curr_thread].buff) == 0){
    for(int i = curr_thread + 1; i<(2*thread_cnt);i++){
      if(tcb_arr[i%thread_cnt].Status == TS_READY){
        next = i%thread_cnt;
        break;
      }
    }
   
      curr_thread = next;
      tcb_arr[curr_thread].Status = TS_RUNNING;
      longjmp(tcb_arr[curr_thread].buff,1);
    
    
  }
  
  
}
// to supress compiler error saying these static functions may not be used...
static void schedule(int signal) __attribute__((unused));
static void scheduler_init()
{
  tcb_arr[0].id = 0;
  tcb_arr->Status = TS_READY;
  thread_cnt = 1;
  //set stacks
  for (int i = 0; i < MAX_THREADS; i++){
      tcb_arr[i].stack = NULL;
      tcb_arr[i].Status = TS_UNIT;
  }
  //if(setjmp(tcb_arr[0].buff)== 0){
    //timer stuff
    struct sigaction sa;
    sa.sa_handler = schedule;
    sa.sa_flags = SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    ///set Sigaction for SIGALARM to be schedule and error if fails
    sigaction(SIGALRM, &sa, NULL);
	useconds_t usecs = QUANTUM; // Timer interval
	useconds_t interval = QUANTUM; // ualarm interval
	ualarm(usecs, interval);
    //sleep(5);
  //}
  /* 
     TODO: do everything that is needed to initialize your scheduler.
     For example:
     - allocate/initialize global threading data structures
     - create a TCB for the main thread. so your scheduler will be able to schedule it
     - set up your timers to call scheduler...
  */
}
int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
  
{
  
  //bool finding_thread = true;
  // Create the timer and handler for the scheduler. Create thread 0.
  static bool is_first_call = true;
  int thread_num = -1;
  if (is_first_call) {
    is_first_call = false;
    scheduler_init();
  }
  for(int i = 1; i<MAX_THREADS; i++){
    if( (tcb_arr[i].stack == NULL)){
      thread_num = i;
      break;
    }
  }
  if(thread_cnt>=MAX_THREADS){
    return -1;
  }
  if(thread_num == -1){
    return -1;
  }else if (thread_num >= MAX_THREADS){
    return -1;
  }
  if(setjmp(tcb_arr[thread_num].buff) == 0){
    tcb_arr[thread_num].id = thread_cnt;
    *thread = thread_cnt;
    thread_cnt = thread_cnt + 1;
    tcb_arr[thread_num].Status = TS_READY;
    tcb_arr[thread_num].stack = (void*)malloc(THREAD_STACK_SIZE);
    unsigned long int stkptr = (unsigned long int)tcb_arr[thread_num].stack + THREAD_STACK_SIZE - sizeof(void *);
    //set return adress
    *((unsigned long int *)stkptr) = (unsigned long int)pthread_exit;
    set_reg(&tcb_arr[thread_num].buff,JBL_RSP,stkptr);
    //Set PC  + R12&R13
    set_reg(&tcb_arr[thread_num].buff,JBL_PC,(unsigned long int)start_thunk);
    set_reg(&tcb_arr[thread_num].buff,JBL_R13,(unsigned long int)arg);
    set_reg(&tcb_arr[thread_num].buff,JBL_R12,(unsigned long int)start_routine);
    
  }
    return 0;
  /* TODO: Return 0 on successful thread creation, non-zero for an error.
   *       Be sure to set *thread on success.
   *
   * You need to create and initialize a TCB (thread control block) including:
   * - Allocate a stack for the thread
   * - Set up the registers for the functions, including:
   *   - Assign the stack pointer in the thread's registers to point to its stack. 
   *   - Assign the program counter in the thread's registers.
   *   - figure out how to have pthread_exit invoked if thread returns
   * - After you are done, mark your new thread as READY
   * Hint: Easiest to use setjmp to save a set of registers that you then modify, 
   *       and look at notes on reading/writing registers saved by setjmp using 
   * Hint: Be careful where the stackpointer is set to it may help to draw
   *       an empty stack diagram to answer that question.
   * Hint: Read over the comment in header file on start_thunk before 
   *       setting the PC.
   * Hint: Don't forget that the thread is expected to "return" to pthread_exit after it is done
   * 
   * Don't forget to assign RSP too! Functions know where to
   * return after they finish based on the calling convention (AMD64 in
   * our case). The address to return to after finishing start_routine
   * should be the first thing you push on your stack.
   */
}

void pthread_exit(void *value_ptr)
{

  tcb_arr[curr_thread].Status = TS_EXITED;
  tcb_arr[curr_thread].exit = value_ptr;
  /* TODO: Exit the current thread instead of exiting the entire process.
   * Hints:
   * - Release all resources for the current thread. CAREFUL though.
   *   If you free() the currently-in-use stack then do something like
   *   call a function or add/remove variables from the stack, bad things
   *   can happen.
   * - Update the thread's status to indicate that it has exited
   * What would you do after this?
   */
  // switch to next thread
  /*
    */
   bool thread_check = false;

    for (int i = 0; i< thread_cnt; i ++){
      if(tcb_arr[i].Status == TS_READY){
        thread_check = true;
      }
    }
    if (thread_check) { 
        ualarm(0,0); // 
        schedule(0); /* Schedule the next thread */
    }

    exit(0);
}
pthread_t pthread_self(void)
{
  /* 
   * TODO: Return the current thread instead of -1, note it is up to you what ptread_t refers to
   */
  return tcb_arr[curr_thread].id;
}
int pthread_join(pthread_t thread, void **retval)
{

  while(tcb_arr[thread].Status != TS_EXITED){
    schedule(SIGALRM);
  }
  free(tcb_arr[thread].stack);
  tcb_arr[thread].stack = NULL;
  *retval = tcb_arr[thread].exit;

  return 0;
}
/* 
 * Don't implement main in this file!
 * This is a library of functions, not an executable program. If you
 * want to run the functions in this file, create separate test programs
 * that have their own main functions.
 */