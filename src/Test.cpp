#include<iostream>
#include<pthread.h>
#include<stdint.h>
#include <new>
#include<gsl/gsl_rng.h>
#include<gsl/gsl_randist.h>
#include<assert.h>
#include "ConcurrentTreeMap.h"


int NUM_OF_THREADS;
int findPercent;
int insertPercent;
int deletePercent;
unsigned long keyRange;
volatile bool stop=false;
struct timespec runTime;
ConcurrentTreeMap<unsigned long, unsigned long> map;

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

void testRange()
{
  unsigned long min = keyRange/20;
  unsigned long max = keyRange/10;
  std::list<Node<unsigned long, unsigned long>*> out = map.range(min, max);
  unsigned long prevKey = std::numeric_limits<unsigned long>::min();
  for(const auto& node : out)
  {
    unsigned long currKey = node->getKey();
    assert(currKey >= min && currKey <= max && currKey >= prevKey);
    prevKey = currKey;
  }
  return;
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
  map.size();
  assert(map.isValidTree());
  testRange();
  pthread_exit(NULL);
  return 0;
}
