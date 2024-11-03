#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NUM_CHEFS 3
#define DISHES_PER_CHEF 10

// Semaphores and mutex declarations
sem_t semaphoreChefs[NUM_CHEFS];
sem_t semaphoreFinish;
sem_t providerReady;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Shared variables
int chefCookCount[NUM_CHEFS];
int allCooked = 0;
double totalCookingTime = 0.0;

const char* providerOffers[NUM_CHEFS] = {"Vegetables & Meat (A+B)", "Meat & Spices (B+C)", "Vegetables & Spices (A+C)"};
const int cookingTimes[NUM_CHEFS] = {5, 3, 1};
const int providerPrepTime = 2;

struct timespec startTime, endTime;

void *chef(void *pVoid)
{
    // Get chef number, mapped to 0-based index
    int chefNumber = *(int*)(pVoid) - 1;

    for (int i = 0; i < DISHES_PER_CHEF; ++i)
    {
        /////////////////////////////////////////////////
        // Implement your code for chef actions here.
        // Remember to include:
        // - Waiting for ingredients
        sem_wait(&semaphoreChefs[chefNumber]);
        // - Printing received ingredients
        printf("Chef %d received ingredients: %s\n", chefNumber + 1, providerOffers[chefNumber]);
        // - Simulating preparation and cooking time
        sleep(cookingTimes[chefNumber]);
        // - Updating cook count and total cooking time
        pthread_mutex_lock(&mutex);
        chefCookCount[chefNumber]++;
        totalCookingTime += cookingTimes[chefNumber];
        pthread_mutex_unlock(&mutex);
        // - Printing finished cooking
        printf("Chef %d finished cooking dish %d\n", chefNumber + 1, i + 1);
        // - Signaling finish
        sem_post(&semaphoreFinish);
        /////////////////////////////////////////////////
    }
    
    pthread_exit(NULL);
}

void *provider(void *pVoid)
{
    /////////////////////////////////////////////////
    // Implement your code for provider initialization here.
    int nextChef = 0;
    /////////////////////////////////////////////////

    while (1)
    {
        /////////////////////////////////////////////////
        // Implement your code for provider actions here.
        // Remember to include:
        // - Checking if all chefs are done
        pthread_mutex_lock(&mutex);
        if (chefCookCount[0] == DISHES_PER_CHEF && chefCookCount[1] == DISHES_PER_CHEF && chefCookCount[2] == DISHES_PER_CHEF)
        {
            allCooked = 1;
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
        // - Offering ingredients
        printf("Provider preparing ingredients for Chef %d: %s\n", nextChef + 1, providerOffers[nextChef]);
        sleep(providerPrepTime);
        // - Simulating preparation time
        sleep(providerPrepTime);
        // - Signaling chef to start cooking
        sem_post(&semaphoreChefs[nextChef]);
        // - Waiting for chef to finish
        sem_wait(&semaphoreFinish);
        // - Selecting the next chef
        nextChef = (nextChef + 1) % NUM_CHEFS;
        /////////////////////////////////////////////////
    }

    return NULL;
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);

    srand(time(NULL));

    // Initialize shared variables
    /////////////////////////////////////////////////
    allCooked = 0;
    totalCookingTime = 0.0;
    for (int i = 0; i < NUM_CHEFS; ++i) {
        chefCookCount[i] = 0; // Initialize each chef's dish count to 0
    }
    // Implement your code to initialize shared variables here.
    /////////////////////////////////////////////////
    

    // Initialize semaphores
    /////////////////////////////////////////////////
    for (int i = 0; i < NUM_CHEFS; i++)
    {
        sem_init(&semaphoreChefs[i], 0, 0);
    }
    sem_init(&semaphoreFinish, 0, 0);
    sem_init(&providerReady, 0, 1);

    pthread_t chefThreads[NUM_CHEFS], providerThread;
    int chefNumbers[NUM_CHEFS] = {1, 2, 3};

    // Implement your code to initialize semaphores here.
    /////////////////////////////////////////////////
    

    // Start timing
    clock_gettime(CLOCK_MONOTONIC, &startTime);


    // Create chef and provider threads
    /////////////////////////////////////////////////
    for (int i = 0; i < NUM_CHEFS; ++i) {
        pthread_create(&chefThreads[i], NULL, chef, &chefNumbers[i]);
    }
    pthread_create(&providerThread, NULL, provider, NULL);
    // Implement your code to create threads here.
    /////////////////////////////////////////////////


    // Join threads
    /////////////////////////////////////////////////
    for (int i = 0; i < NUM_CHEFS; ++i) {
        pthread_join(chefThreads[i], NULL);
    }
    pthread_join(providerThread, NULL);
    // Implement your code to join threads here.
    /////////////////////////////////////////////////

    // End timing
    clock_gettime(CLOCK_MONOTONIC, &endTime);

    double totalTime = (endTime.tv_sec - startTime.tv_sec) + 
                       (endTime.tv_nsec - startTime.tv_nsec) / 1e9;

    printf("All chefs have finished cooking.\n");
    printf("Total running time: %.2f seconds\n", totalTime);
    printf("Total cumulative cooking time (including ingredient wait time): %.2f seconds\n", totalCookingTime);

    // Clean up
    /////////////////////////////////////////////////
    for (int i = 0; i < NUM_CHEFS; ++i) {
        sem_destroy(&semaphoreChefs[i]);
    }
    sem_destroy(&semaphoreFinish);
    sem_destroy(&providerReady);
    pthread_mutex_destroy(&mutex);
    // Implement your code to destroy semaphores and mutex here.
    /////////////////////////////////////////////////

    return 0;
}