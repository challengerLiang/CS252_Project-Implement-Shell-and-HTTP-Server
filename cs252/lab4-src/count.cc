
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int count;

pthread_mutex_t m1;

void increment(int ntimes )
{
	for ( int i = 0; i < ntimes; i++ ) {
		pthread_mutex_lock(&m1);

		int c;

		c = count;
		c = c + 1;

		count = c;
		
		pthread_mutex_unlock(&m1);
	}
}

int main( int argc, char ** argv )
{
	int n = 10000000;
	pthread_t t1, t2;
        //pthread_attr_t attr;

        //pthread_attr_init( &attr );
        //pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	printf("Start Test. Final count should be %d\n", 2 * n );

	// Create threads
	pthread_create( &t1, NULL, (void * (*)(void *)) increment, 
			(void *) n);

	pthread_create( &t2, NULL, (void * (*)(void *)) increment, 
			(void *) n);

	// Wait until threads are done
	pthread_join( t1, NULL );
	pthread_join( t2, NULL );

	if ( count != 2 * n ) {
		printf("\n****** Error. Final count is %d\n", count );
		printf("****** It should be %d\n", 2 * n );
	}
	else {
		printf("\n>>>>>> O.K. Final count is %d\n", count );
	}
}


