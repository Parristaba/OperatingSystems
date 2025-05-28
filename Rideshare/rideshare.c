#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

const int SINGLE_TEAM = 4;
const int TWO_TEAMS = 2;

typedef struct {
    sem_t semaphore;  
    int fanCount;     
} teamInfo;

pthread_mutex_t globalLock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    pthread_t driverId;
    int carId;
    pthread_barrier_t carBarrier;    
} carInfo;


teamInfo teamA;
teamInfo teamB;
carInfo myCar;

void* threadForFans(void *args){
    char team = *((char*)args);

    pthread_mutex_lock(&globalLock);
    printf("Thread ID: %lu, Team: %c, I am looking for a car\n",
        (unsigned long) pthread_self(), team);

    if(team == 'A'){
        teamA.fanCount++;
    }
    else{
        teamB.fanCount++;
    }

    if(team == 'A'){
        if(teamA.fanCount  == SINGLE_TEAM){
            myCar.driverId = pthread_self();
            for(int i = 0; i < 3; i++) {
                sem_post(&teamA.semaphore);
            }
            teamA.fanCount -= 4;
        }
        else if((teamA.fanCount == TWO_TEAMS) && (teamB.fanCount >= TWO_TEAMS)){
            myCar.driverId = pthread_self();
            sem_post(&teamA.semaphore);
            teamA.fanCount -= 2;
            for(int i = 0; i < 2; i++){
                sem_post(&teamB.semaphore);
            } 
            teamB.fanCount -= 2;
        }
        else{
            pthread_mutex_unlock(&globalLock);
            sem_wait(&teamA.semaphore);
        }
    }
    else{
        if(teamB.fanCount == SINGLE_TEAM){
            myCar.driverId = pthread_self();
            for(int i = 0; i < 3; i++) {
                sem_post(&teamB.semaphore);
            }
            teamB.fanCount -= 4;
        }
        else if((teamB.fanCount == TWO_TEAMS) && (teamA.fanCount >= TWO_TEAMS)){
            myCar.driverId = pthread_self();
            sem_post(&teamB.semaphore);
            teamB.fanCount -= 2;
            for(int i = 0; i < 2; i++) {
                sem_post(&teamA.semaphore);
            }
            teamA.fanCount -= 2;
        }
        else{
            pthread_mutex_unlock(&globalLock);
            sem_wait(&teamB.semaphore);
        }
    }
    
    printf("Thread ID: %lu, Team: %c, I have found a spot in a car\n",
        (unsigned long) pthread_self(), team);
    pthread_barrier_wait(&myCar.carBarrier); 
    
    if (pthread_self() == myCar.driverId){
        printf("Thread ID: %lu, Team: %c, I am the captain and driving the car with ID %d\n",
            (unsigned long) pthread_self() , team, myCar.carId);
        pthread_mutex_unlock(&globalLock);
        myCar.carId ++;
    }  

    pthread_exit(NULL);
}

int processArguments(int argc, char *argv[], int *A, int *B) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <A> <B>\n", argv[0]);
        return 1;
    }

    *A = atoi(argv[1]);
    *B = atoi(argv[2]);

    return 0;
}


int main(int argc, char *argv[]) {
    int A, B;
    if (processArguments(argc, argv, &A, &B) != 0) {
        return 1;
    }
    int total = A + B;
    
    if(total % 4 != 0 || A % 2 != 0 || B % 2 != 0){
        return EXIT_FAILURE;
    }
    else{

        myCar.carId = 0;
        pthread_mutex_init(&globalLock, NULL);
        pthread_barrier_init(&myCar.carBarrier, NULL, 4);
        sem_init(&teamA.semaphore, 0, 0);
        sem_init(&teamB.semaphore, 0, 0);

        pthread_t threadsA[A];
        pthread_t threadsB[B];

    pthread_t *allThreads = malloc((A + B) * sizeof(pthread_t));

    for (int i = 0; i < A + B; i++) {
        char *team = (i < A) ? "A" : "B";
        pthread_create(&allThreads[i], NULL, threadForFans, team);
    }
    
    for (int i = 0; i < A + B; i++) {
        pthread_join(allThreads[i], NULL);
    }
        
    }

    printf("The main terminates\n");
    return 0;
}