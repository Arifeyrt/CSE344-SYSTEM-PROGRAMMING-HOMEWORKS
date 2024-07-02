#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define TEMP_PICKUP_CAPACITY 4
#define TEMP_AUTOMOBILE_CAPACITY 8
#define PARKING_PICKUP_CAPACITY 4
#define PARKING_AUTOMOBILE_CAPACITY 8
#define TOTAL_VEHICLES 30

// Semaphores
sem_t newAutomobile;
sem_t newPickup;
sem_t inChargeforAutomobile;
sem_t inChargeforPickup;

// Counters for free spaces
int mFree_automobile = TEMP_AUTOMOBILE_CAPACITY; // Free spaces in temporary parking for automobiles
int mFree_pickup = TEMP_PICKUP_CAPACITY; // Free spaces in temporary parking for pickups
int parkingFreeAutomobile = PARKING_AUTOMOBILE_CAPACITY;
int parkingFreePickup = PARKING_PICKUP_CAPACITY;

// Function prototypes
void* carOwner(void* arg);
void* carAttendant(void* arg);

int main() {
    srand(time(NULL));

    sem_init(&newAutomobile, 0, 1);
    sem_init(&newPickup, 0, 1);
    sem_init(&inChargeforAutomobile, 0, 0);
    sem_init(&inChargeforPickup, 0, 0);

    pthread_t ownerThreads[TOTAL_VEHICLES];
    pthread_t attendantThreads[2];

    // Create car attendant threads
    for (int i = 0; i < 2; i++) {
        int* carType = malloc(sizeof(int));
        *carType = i;
        pthread_create(&attendantThreads[i], NULL, carAttendant, carType);
        sleep(1); // Random delay to simulate valet arrival
    }

    // Create car owner threads
    for (int i = 0; i < TOTAL_VEHICLES; i++) {
        int* carType = malloc(sizeof(int));
        *carType = rand() % 2; // 0 for automobile, 1 for pickup
        pthread_create(&ownerThreads[i], NULL, carOwner, carType);
        sleep(rand() % 3 + 1); // Random delay to simulate car arrivals
    }

    // Join car owner threads
    for (int i = 0; i < TOTAL_VEHICLES; i++) {
        pthread_join(ownerThreads[i], NULL);
    }

    // Terminate car attendant threads
    for (int i = 0; i < 2; i++) {
        pthread_cancel(attendantThreads[i]);
        pthread_join(attendantThreads[i], NULL);
    }

    // Destroy semaphores
    sem_destroy(&newAutomobile);
    sem_destroy(&newPickup);
    sem_destroy(&inChargeforAutomobile);
    sem_destroy(&inChargeforPickup);

    return 0;
}

void* carOwner(void* arg) {
    int carType = *(int*)arg;
    free(arg);
    static int carId = 0;

    int localCarId = __sync_add_and_fetch(&carId, 1); // Atomic increment

    sleep(rand() % 3 + 1); // Random delay to simulate car arrivals

    char* type = (carType == 0) ? "Automobile" : "Pickup";

    if (carType == 0) {
        sem_wait(&newAutomobile);
        if (mFree_automobile > 0) {
            mFree_automobile--;// Decrement free spaces in temporary parking for automobiles
            sem_post(&inChargeforAutomobile);// Signal valet for automobile
            printf("Car %d owner parked in temporary parking (%s). Valet is waiting.\n", localCarId, type);
            printf("Remaining spaces in temporary parking for automobiles= %d\n", mFree_automobile);
            printf("Remaining spaces in temporary parking for pickups= %d\n", mFree_pickup);
            printf("******************************************\n");
            sem_post(&newAutomobile);// Release semaphore
        } else {
            sem_post(&newAutomobile);// Release semaphore
            printf("Car %d owner (%s) couldn't park in temporary parking. No space left.\n", localCarId, type);
            printf("********************************************\n");

        }
    } else {
        sem_wait(&newPickup);
        if (mFree_pickup > 0) {
            mFree_pickup--;// Decrement free spaces in temporary parking for pickups
            sem_post(&inChargeforPickup);
            printf("Car %d owner parked in temporary parking (%s). Valet is waiting.\n", localCarId, type);
            printf("Remaining spaces in temporary parking for automobiles= %d\n", mFree_automobile);
            printf("Remaining spaces in temporary parking for pickups= %d\n", mFree_pickup);
            printf("******************************************\n");
            sem_post(&newPickup);
        } else {
            sem_post(&newPickup);
            printf("Car %d owner (%s) couldn't park in temporary parking. No space left.\n", localCarId, type);
            printf("********************************************\n");

        }
    }
    return NULL;
}

void* carAttendant(void* arg) {
    int carType = *(int*)arg;
    free(arg);
    char* type = (carType == 0) ? "Automobile" : "Pickup";
    int attendantId = carType == 0 ? 1 : 2; // Valet 1 for Automobile, Valet 2 for Pickup

    printf("Valet %d started for %s.\n", attendantId, type);

    while (1) {
        int random_transport_time = rand() % 4 + 3; // Random time to simulate transport process

        if (carType == 0) {
            sem_wait(&inChargeforAutomobile);
            sem_wait(&newAutomobile);// Wait for new automobile
            if (parkingFreeAutomobile > 0) {
                parkingFreeAutomobile--;//  Decrement free spaces in main parking for automobiles
                mFree_automobile++;// Increment free spaces in temporary parking for automobiles
                printf("Valet %d parked %s.\n", attendantId, type);
                printf("Remaining spaces in main parking for automobiles= %d\n", parkingFreeAutomobile);
                printf("Remaining spaces in main parking for pickups= %d\n", parkingFreePickup);
                printf("Remaining spaces in temporary parking for automobiles= %d\n", mFree_automobile);
                printf("Remaining spaces in temporary parking for pickups= %d\n", mFree_pickup);
                printf("********************************************\n");
                sem_post(&newAutomobile);
            } else {
                sem_post(&newAutomobile);
                printf("No space left for %s in main parking. Valet %d can't take more cars.\n", type, attendantId);
                printf("********************************************\n");

                break;
            }
        } else {
            sem_wait(&inChargeforPickup);
            sem_wait(&newPickup);
            if (parkingFreePickup > 0) {
                parkingFreePickup--;// Decrement free spaces in main parking for pickups
                mFree_pickup++;// Increment free spaces in temporary parking for pickups
              printf("Valet %d parked %s.\n", attendantId, type);
                printf("Remaining spaces in main parking for automobiles= %d\n", parkingFreeAutomobile);
                printf("Remaining spaces in main parking for pickups= %d\n", parkingFreePickup);
                printf("Remaining spaces in temporary parking for automobiles= %d\n", mFree_automobile);
                printf("Remaining spaces in temporary parking for pickups= %d\n", mFree_pickup);
                printf("********************************************\n");
                sem_post(&newPickup);
            } else {
                sem_post(&newPickup);
                printf("No space left for %s in main parking. Valet %d can't take more cars.\n", type, attendantId);
                printf("********************************************\n");

                break;
            }
        }

        sleep(random_transport_time); // Simulate transport process
    }
    return NULL;
}
