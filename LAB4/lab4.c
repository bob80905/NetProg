#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

struct Args {
    int a;
    int b;
};

void * add(void * ptr) {
    struct Args * args = ptr;
    long ret;
    if (args->b) {  
        // printf("Thread %lu running add() with [%d + %d]\n", (long)pthread_self(), args->a, args->b);
        args->b--;  
        ret = 1 + (int)add(ptr);
    } else {
        ret = args->a;
    }
	return ((void*)ret);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        perror("Invalid arguments");
        return EXIT_FAILURE;
    }

    setvbuf(stdout, NULL, _IONBF, 0); // Turn off stdout buffering
    int MAX_ADDAND = atoi(argv[1]);
    int NUM_CHILD = MAX_ADDAND * (MAX_ADDAND-1);
	pthread_t children[NUM_CHILD];
    int tid_index = 0;
    for (int a = 1; a < MAX_ADDAND; a++) {
        for (int b = 1; b <= MAX_ADDAND; b++) {
            pthread_t tid;
            struct Args *args = (struct Args*) calloc(1, sizeof(struct Args));
            args->a = a;
            args->b = b;
            int val = pthread_create(&tid, NULL, add, (void*)args);
            printf("Main starting thread add() for [%d + %d]\n", a, b);
            printf("Thread %lu running add() with [%d + %d]\n", (long)tid, args->a, args->b);
            if (val < 0) {
                return -1;
            } else {
                children[tid_index++] = tid;
            }
        }
    }

    int i = 0;
    for (int a = 1; a < MAX_ADDAND; a++) {
        for (int b = 1; b <= MAX_ADDAND; b++) {
            int *ret_val;
            pthread_join(children[i], (void**)&ret_val);
            printf("In main, collecting thread %lu computed [%d + %d] = %d\n", (long)children[i], a, b, (int)ret_val);
            i++;
        }
    }

	return 0;
}
