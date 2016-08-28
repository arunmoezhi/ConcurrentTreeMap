#include<iostream>
#include<pthread.h>
#include<stdint.h>
#include <new>
#include<gsl/gsl_rng.h>
#include<gsl/gsl_randist.h>
#include<assert.h>
#include "TreeMap.h"


int NUM_OF_THREADS;
int findPercent;
int insertPercent;
int deletePercent;
unsigned long keyRange;
volatile bool stop=false;
struct timespec runTime;
TreeMap<int, int> map;

static inline unsigned long getRandomKey(gsl_rng* r)
{
	return(gsl_rng_uniform_int(r,keyRange) + 2);
}

void *operateOnTree(void* id)
{
	int threadId;
	unsigned long lseed;

	threadId = *((int*)id);

	const gsl_rng_type* T;
  gsl_rng* r;
  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc(T);
	lseed = threadId;

	while(!stop)
  {
		int chooseOperation = gsl_rng_uniform(r)*100;
		unsigned long key = getRandomKey(r);
		if(chooseOperation < findPercent)
		{
			map.lookup(key);
		}
		else if (chooseOperation < insertPercent)
		{
			map.insert(key, key);
		}
		else
		{
			map.remove(key);
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	unsigned long lseed;

	//get run configuration from command line
  NUM_OF_THREADS = atoi(argv[1]);
  findPercent = atoi(argv[2]);
  insertPercent= findPercent + atoi(argv[3]);
  deletePercent = insertPercent + atoi(argv[4]);
  runTime.tv_sec = atoi(argv[5]);
	runTime.tv_nsec =0;
	keyRange = (unsigned long) atol(argv[6]);
	lseed = (unsigned long) atol(argv[7]);

	const gsl_rng_type* T;
  gsl_rng* r;
  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc(T);
  gsl_rng_set(r,lseed);

	pthread_t threadArray[NUM_OF_THREADS];
	int threadId[NUM_OF_THREADS];

	for(int i=0;i<NUM_OF_THREADS;i++)
	{
		threadId[i] = i;
		pthread_create(&threadArray[i], NULL, operateOnTree, (void*) (threadId + i) );
	}

	//start operations
	nanosleep(&runTime,NULL);
	stop=true;	//stop operations

	for(int i=0;i<NUM_OF_THREADS;i++)
	{
		pthread_join(threadArray[i], NULL);
	}
	assert(map.isValidTree());
	pthread_exit(NULL);
	return 0;
}