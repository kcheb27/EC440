#include "tls.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>


/*
 * This is a good place to define any data structures you will use in this file.
 * For example:
 *  - struct TLS: may indicate information about a thread's local storage
 *    (which thread, how much storage, where is the storage in memory)
 *  - struct page: May indicate a shareable unit of memory (we specified in
 *    homework prompt that you don't need to offer fine-grain cloning and CoW,
 *    and that page granularity is sufficient). Relevant information for sharing
 *    could be: where is the shared page's data, and how many threads are sharing it
 *  - Some kind of data structure to help find a TLS, searching by thread ID.
 *    E.g., a list of thread IDs and their related TLS structs, or a hash table.
 */

/*
 * Now that data structures are defined, here's a good place to declare any
 * global variables.
 */

/*
 * With global data declared, this is a good point to start defining your
 * static helper functions.
 */

/*
 * Lastly, here is a good place to add your externally-callable functions.
 */ 

#define MAX_THREAD_COUNT 128

struct TLS
{
	pthread_t tid;			//id of thread that owns this TLS
	unsigned int size;		//size in bytes
	unsigned int page_num;	//number of pages
	struct page **pages;	//array of pointers to pages
};

struct page
{
	unsigned long int address;	//start address of page
	int ref_count;				//counter for shared pages
};

struct TLS_HASH
{
	pthread_t tid;
	struct TLS *tls;
};

//---------Global Variables---------//
int page_size;
bool initialize = false;

struct TLS_HASH TLS_Hash[MAX_THREAD_COUNT]; //global variable to keep track of all thread IDs and the corresponding TLS


//---------Helper Functions---------//
void tls_protect(struct page* p)
{
	if (mprotect((void *) p->address, page_size, PROT_NONE))
	{
		fprintf(stderr, "tls_protect: could not protect page\n");
		exit(1);
	}
}

void tls_unprotect(struct page* p)
{
	if (mprotect((void *) p->address, page_size, PROT_READ | PROT_WRITE))
	{
		fprintf(stderr, "tls_unprotect: could not unprotect page\n");
		exit(1);
	}
}

void tls_handle_page_fault(int sig, siginfo_t *si, void *context)
{
	unsigned long int p_fault = ((unsigned long int) si->si_addr) & ~(page_size - 1); //start address of the page that caused the seg fault

	for (int i = 0; i < MAX_THREAD_COUNT; i++) //loop through array of TIDs and TLSs
	{
		for (int j = 0; j < TLS_Hash[j].tls->page_num; j++) //loop through all pages of each TLS
		{
			if (TLS_Hash[i].tls->pages[j]->address == p_fault) //does the page's address match the page fault address?
			{
				printf("Seg fault caused by thread... exiting thread...\n");
				pthread_exit(NULL);
			}
		}
	}

	printf("Regular seg fault... re-raising signal...\n");
	signal(SIGSEGV, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	raise(sig);
}

void tls_init()
{
	page_size = getpagesize();

	struct sigaction sigact;

	/* Handle page faults (SIGSEGV, SIGBUS) */
	sigemptyset(&sigact.sa_mask);

	/* Give context to handler */
	sigact.sa_flags = SA_SIGINFO;
	sigact.sa_sigaction = tls_handle_page_fault;
	sigaction(SIGBUS, &sigact, NULL);
	sigaction(SIGSEGV, &sigact, NULL);

	for (int i = 0; i < MAX_THREAD_COUNT; i++)
	{
		TLS_Hash[i].tid = -1; //For now, set all thread IDs to -1
	}

	initialize = true;

}




int tls_create(unsigned int size)
{
	if (initialize == false) //for first time tls_create is called
	{
		tls_init();
	}

	//Error checking----

	if (size < 0)
	{
		return -1;
	}

	bool current_has_tls = false;
	for(int i = 0; i < MAX_THREAD_COUNT; i++) //check if the thread already has a LSA
	{
		if (TLS_Hash[i].tid == pthread_self())
		{
			current_has_tls = true;
		}
	}

	if (current_has_tls == true)
	{
		return -1;
	}
	
	//-------------------

	int new_tls_index = -1;
	struct TLS* new_tls = (struct TLS *)calloc(1, sizeof(struct TLS)); //calloc size for new TLS

	for (int i = 0; i < MAX_THREAD_COUNT; i++) //need to find next open spot to store TID and TLS pair
	{
		if (TLS_Hash[i].tid == -1) //then the spot is available
		{
			new_tls_index = i;
			break;
		}
	}

	//assign the correct properties for the new TLS
	new_tls->size = size; 
	new_tls->page_num = ((size - 1) / (page_size)) + 1; // - 1 ??
	new_tls->pages = (struct page**)calloc(new_tls->page_num, sizeof(struct page *)); //calloc the array of pages
	new_tls->tid = pthread_self();

	for (int i = 0; i < new_tls->page_num; i++)
	{
		struct page *pg = (struct page *)calloc(1, sizeof(struct page *)); //calloc the page
		pg->address = (unsigned long int)mmap(0, page_size, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0); //obtain memory for the page
		pg->ref_count = 1; //set ref_count to 1 (only this thread is currently using the page)
		new_tls->pages[i] = pg; //place the page in the array of pages
        //protect pages
        tls_protect(new_tls->pages[i]);
	}	

	TLS_Hash[new_tls_index].tls = new_tls;
	TLS_Hash[new_tls_index].tid = pthread_self();

	return 0;
}

int tls_destroy()
{
    bool has_tls = false;
    int index = -1; 
    for (int i = 0; i <MAX_THREAD_COUNT; i ++){
        if(TLS_Hash[i].tid == pthread_self()){
            has_tls = true;
            index = i;
            TLS_Hash[index].tid = -1;
            TLS_Hash[index].tls->tid = -1;
            TLS_Hash[index].tls->size = -1;
            
            break;

        }
    }
    if(!has_tls){
        return -1;
    }
    for(int i = 0; i<TLS_Hash[index].tls->page_num; i++){
        //check if shared
        if(TLS_Hash[index].tls->pages[i]->ref_count > 1){
            TLS_Hash[index].tls->pages[i]->ref_count = TLS_Hash[index].tls->pages[i]->ref_count - 1;
        }else{
            //delete if not
            munmap((void *)TLS_Hash[index].tls->pages[i]->address,page_size);
            //free page 
            free(TLS_Hash[index].tls->pages[i]);
        }

    }
    TLS_Hash[index].tls->page_num = -1;
    free(TLS_Hash[index].tls->pages);
    free(TLS_Hash[index].tls);
    


	return 0;
}

int tls_read(unsigned int offset, unsigned int length, char *buffer)
{
    //check tls
    bool tls_exists = false;
    int index= -1;
    for(int i = 0; i <MAX_THREAD_COUNT; i++)
    {
        if(TLS_Hash[i].tid == pthread_self()){
            tls_exists = true;
            index = i;
            break;
        }
    }
    if(!tls_exists){
        return -1;
    }
    if((offset + length)>(TLS_Hash[index].tls->size)){
        return -1;
    }
    //unprotect
    for(int i = 0; i < TLS_Hash[index].tls->page_num; i++){
        tls_unprotect(TLS_Hash[index].tls->pages[i]);
    }
    unsigned int  cnt , idx;
    for (cnt = 0, idx = offset; idx < (offset + 
        length); ++cnt, ++idx) {
        struct page *p;
        unsigned int pn, poff;
        pn = idx / page_size;
        poff = idx % page_size;
        p = TLS_Hash[index].tls->pages[pn];
        char * src = ((char *) p->address) + poff;
        
        buffer[cnt] = *src;
    }
    //protect
    for(int i = 0; i < TLS_Hash[index].tls->page_num; i++){
        tls_protect(TLS_Hash[index].tls->pages[i]);
    }

	return 0;
}

int tls_write(unsigned int offset, unsigned int length, const char *buffer)
{
     //check tls
    bool tls_exists = false;
    int index= -1;
    for(int i = 0; i <MAX_THREAD_COUNT; i++)
    {
        if(TLS_Hash[i].tid == pthread_self()){
            tls_exists = true;
            index = i;
            break;
        }
    }
    if(!tls_exists){
        return -1;
    }
    if((offset + length)>(TLS_Hash[index].tls->size)){
        return -1;
    }
    for(int i = 0; i < TLS_Hash[index].tls->page_num; i++){
        tls_unprotect(TLS_Hash[index].tls->pages[i]);
    }
    unsigned int cnt, idx;

    for (cnt = 0, idx = offset; idx < (offset + length); ++cnt, ++idx) {
        struct page *p, *copy;
        unsigned int pn, poff;
        pn = idx / page_size;
        poff = idx % page_size;
        p = TLS_Hash[index].tls->pages[pn];
        if (p->ref_count > 1) {
            copy = (struct page *) calloc(1, sizeof(struct page));
            copy->address = (long int) mmap(0, page_size, PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
            copy->ref_count = 1;
            memcpy((void*) copy->address,(void*)p->address,page_size);
            TLS_Hash[index].tls->pages[pn] = copy; 
            /* update original page */
            p->ref_count--;
            tls_protect(p);
            p = copy;     
        }
        char * dst = ((char *) p->address) + poff;
        *dst = buffer[cnt];
     }
    for(int i = 0; i < TLS_Hash[index].tls->page_num; i++){
        tls_protect(TLS_Hash[index].tls->pages[i]);
    }
	//Error checking----------------
    return 0;

}

int tls_clone(pthread_t tid)
{
    bool tls_exists = false;
    bool target_exists = false;
    int index = -1;

    for(int i = 0; i <MAX_THREAD_COUNT; i++)
    {
        if(TLS_Hash[i].tid == pthread_self()){
            tls_exists = true;
            break;
        }
    }
    if(tls_exists){
        return -1;
    }
    //find target
    for(int i = 0; i <MAX_THREAD_COUNT; i++){
        if(TLS_Hash[i].tid == tid){
            index = i;
            target_exists = true;
        }
    }
    if(!target_exists){
        return -1;
    }
    int new_tls_index = -1;
    for (int i= 0; i<MAX_THREAD_COUNT; i++){
        if(TLS_Hash[i].tid == -1)
        {
            new_tls_index = i;
            break;
        }
    }
    //allocate tls
    struct TLS* new_tls = (struct TLS *)calloc(1, sizeof(struct TLS)); //calloc size for new TLS
    new_tls->page_num = TLS_Hash[index].tls->page_num;
    new_tls->size = TLS_Hash[index].tls->size;
    new_tls->pages = (struct page **)calloc(new_tls->page_num,sizeof(struct page *));
    new_tls->tid= pthread_self();

    for (int i = 0; i<TLS_Hash[index].tls->page_num; i++){
        struct page * copy;
        copy = (struct page *)calloc(1,sizeof(struct page));
        //unprotect
        tls_unprotect(TLS_Hash[index].tls->pages[i]);
        copy = TLS_Hash[index].tls->pages[i];
        TLS_Hash[index].tls->pages[i]->ref_count = TLS_Hash[index].tls->pages[i]->ref_count + 1;
        new_tls->pages[i] = copy;
        tls_protect(TLS_Hash[index].tls->pages[i]);
        
    }

    //save to HASH table
    TLS_Hash[new_tls_index].tid = new_tls->tid;
    TLS_Hash[new_tls_index].tls = new_tls;



	return 0;
}


