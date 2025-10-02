#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <limits.h>
#define ELEVATOR_MAX_CAP 20
#define MAX_NEW_REQUESTS 30
#define PERMS 0666

typedef struct PassengerRequest
{
    int requestId;
    int startFloor;
    int requestedFloor;
} PassengerRequest;

typedef struct MainSharedMemory
{
    char authStrings[100][ELEVATOR_MAX_CAP + 1];
    char elevatorMovementInstructions[100];
    PassengerRequest newPassengerRequests[MAX_NEW_REQUESTS];
    int elevatorFloors[100];
    int droppedPassengers[1000];
    int pickedUpPassengers[1000][2];
} MainSharedMemory;

typedef struct TurnChangeResponse
{
    long mtype;
    int turnNumber;
    int newPassengerRequestCount;
    int errorOccured;
    int finished;
} TurnChangeResponse;

typedef struct TurnChangeRequest
{
    long mtype;
    int droppedPassengersCount;
    int pickedUpPassengersCount;
} TurnChangeRequest;

typedef struct SolverRequest
{
    long mtype;
    int elevatorNumber;
    char authStringGuess[ELEVATOR_MAX_CAP + 1];
} SolverRequest;

typedef struct SolverResponse
{
    long mtype;
    int guessIsCorrect;
} SolverResponse;

typedef struct
{
    int size;
    char keys[10000][100];
    int values[10000];
} Map;

// Function to get the index of a key in the keys array
int getIndex(Map *map, char key[])
{
    for (int i = 0; i < map->size; i++)
    {
        if (strcmp(map->keys[i], key) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Function to insert a key-value pair into the map
void insert(Map *map, char key[], int value)
{
    int index = getIndex(map, key);
    if (index == -1)
    { // Key not found
        strcpy(map->keys[map->size], key);
        map->values[map->size] = value;
        map->size++;
    }
    else
    { // Key found
        map->values[index] = value;
    }
}

// Function to get the value of a key in the map
int get(Map *map, char key[])
{
    int index = getIndex(map, key);
    if (index == -1)
    {
        return -1;
    }
    else
    {
        return map->values[index];
    }
}

// Function to delete a key-value pair from the map
void delete(Map *map, char key[])
{
    int index = getIndex(map, key);
    if (index == -1)
    {
        printf("Key not found.\n");
        return;
    }

    // Shift elements to the left to fill the gap
    for (int i = index; i < map->size - 1; i++)
    {
        strcpy(map->keys[i], map->keys[i + 1]);
        map->values[i] = map->values[i + 1];
    }

    map->size--; 
}


void removeElementFromArray(int *arr, int index)
{
    int tempSize = arr[0];
    for (int i = index; i <= tempSize - 1; i++)
    {
        arr[i] = arr[i + 1];
    }
    arr[0]--;
}

char *generateAuthStrings(int noOfPeople, SolverRequest reqAuth, int solverId)
{
    reqAuth.mtype = 3;

    SolverResponse resAuth;

    char *authString = malloc(noOfPeople + 1);
    authString[noOfPeople] = '\0';

    int indices[noOfPeople];
    for (int i = 0; i < noOfPeople; i++)
    {
        indices[i] = 0;
    }
    int done = 0;

    while (!done)
    {
        for (int i = 0; i < noOfPeople; i++)
        {
            authString[i] = 'a' + indices[i];
        }

        // we get the generated auth string here
        for (int i = 0; i < noOfPeople + 1; i++)
        {
            reqAuth.authStringGuess[i] = authString[i];
        }

        msgsnd(solverId, &reqAuth, sizeof(reqAuth) - sizeof(reqAuth.mtype), 0);
        msgrcv(solverId, &resAuth, sizeof(resAuth) - sizeof(resAuth.mtype), 4, 0);
        if (resAuth.guessIsCorrect != 0)
        {
            return authString;
        }

        for (int i = noOfPeople - 1; i >= 0; i--)
        {
            if (indices[i] < 5)
            {
                indices[i]++;
                break;
            }
            else
            {
                indices[i] = 0;
                if (i == 0)
                {
                    done = 1;
                }
            }
        }
    }
    return authString;
}

int main()
{
    // N <= 100    Number of Elevators
    // K <= 500    Number of Floors
    // M <= N	   Number of Solvers
    // T <= 100    Turn of Last Request

    int N, K, M, T, shmKey, mainMsgQueueKey;

    // taking input from input.txt
    FILE *inputFilePtr = fopen("input.txt", "r");
    fscanf(inputFilePtr, "%d", &N);
    fscanf(inputFilePtr, "%d", &K);
    fscanf(inputFilePtr, "%d", &M);
    fscanf(inputFilePtr, "%d", &T);
    fscanf(inputFilePtr, "%d", &shmKey);
    fscanf(inputFilePtr, "%d", &mainMsgQueueKey);
    int solverMsgQueue[M];

    for (int i = 0; i < M; i++)
    {
        fscanf(inputFilePtr, "%d", &solverMsgQueue[i]);
    }

    // printing the input.txt for degubbing
    printf("%d\n", N);
    printf("%d\n", K);
    printf("%d\n", M);
    printf("%d\n", T);
    printf("%d\n", shmKey);
    printf("%d\n", mainMsgQueueKey);
    for (int i = 0; i < M; i++)
    {
        printf("%d\n", solverMsgQueue[i]);
    }

    // Connecting to shared Memory
    MainSharedMemory *mainShmPtr;
    int shmId = shmget(shmKey, sizeof(MainSharedMemory), PERMS);
    mainShmPtr = shmat(shmId, NULL, 0);

    // getting the first response from main message queue
    TurnChangeResponse mainMsgResponse;
    int mainMsgResponseId = msgget((key_t)mainMsgQueueKey, PERMS);
    msgrcv(mainMsgResponseId, &mainMsgResponse, sizeof(mainMsgResponse) - sizeof(mainMsgResponse.mtype), 2, 0);
    int isFinished = mainMsgResponse.finished;


    // printing the passengers with request and floor going details for understanding
    for (int i = 0; i < mainMsgResponse.newPassengerRequestCount; i++)
    {
        printf("Passenger %d: requestId: %d\n", i, mainShmPtr->newPassengerRequests[i].requestId);
        printf("Passenger %d: startFloor: %d\n", i, mainShmPtr->newPassengerRequests[i].startFloor);
        printf("Passenger %d: requestedFloor: %d\n", i, mainShmPtr->newPassengerRequests[i].requestedFloor);
    }

    int moveDirection[N];
    for (int i = 0; i < N; i++)
    {
        moveDirection[i] = 0;
    }

    int **elevatorManager = (int **)malloc(N * sizeof(int *));
    int **crowdManager = (int **)malloc(N * sizeof(int *));
    for (int i = 0; i < N; i++)
    {
        elevatorManager[i] = (int *)malloc((30 * T + 1) * sizeof(int));
        crowdManager[i] = (int *)malloc((30 * T + 1) * sizeof(int));
    }
    for (int i = 0; i < N; i++)
    {
        elevatorManager[i][0] = 0;
        crowdManager[i][0] = 0;
    }

    Map passengerStart = {0};
    Map passengerDest = {0};

    int noOfPeopleInElevator[N];
    for (int i = 0; i < N; i++)
    {
        noOfPeopleInElevator[i] = 0;
    }

    int totalPassengers = 0;
    int pickedPassengers = 0;
    int droppedPassengers = 0;

    while (isFinished == 0)
    {
        printf("\n\nturnNumber: %d ----------------------------------------------------------\n", mainMsgResponse.turnNumber);
        printf("newPassengerRequestCount: %d\n", mainMsgResponse.newPassengerRequestCount);
        for (int i = 0; i < mainMsgResponse.newPassengerRequestCount; i++)
        {
            printf("Passenger %d: requestId: %d\n", i, mainShmPtr->newPassengerRequests[i].requestId);
            printf("Passenger %d: startFloor: %d\n", i, mainShmPtr->newPassengerRequests[i].startFloor);
            printf("Passenger %d: requestedFloor: %d\n", i, mainShmPtr->newPassengerRequests[i].requestedFloor);
            printf("\n");
        }

        TurnChangeRequest mainMsgRequest;
        mainMsgRequest.mtype = 1;
        mainMsgRequest.droppedPassengersCount = 0;
        mainMsgRequest.pickedUpPassengersCount = 0;

        // inserting new passenges on the map
        for (int i = 0; i < mainMsgResponse.newPassengerRequestCount; i++)
        {
            char requestIdString[1000];
            sprintf(requestIdString, "%d", mainShmPtr->newPassengerRequests[i].requestId);
            insert(&passengerStart, requestIdString, mainShmPtr->newPassengerRequests[i].startFloor);
            insert(&passengerDest, requestIdString, mainShmPtr->newPassengerRequests[i].requestedFloor);
        }

        // assigning of passengers to their respective elevators
        int newPassengersCount = mainMsgResponse.newPassengerRequestCount;

        for (int i = 0; i < newPassengersCount; i++)
        {
            int passengerRequestId = mainShmPtr->newPassengerRequests[i].requestId;
            int passengerStartfloor = mainShmPtr->newPassengerRequests[i].startFloor;
            int passengerRequestedFloor = mainShmPtr->newPassengerRequests[i].requestedFloor;

            int minDist = INT_MAX;
            int assignedLift = -1;
            int movePassengerVector = passengerRequestedFloor - passengerStartfloor;
            int minLoadLift = -1;
            int minLoad = INT_MAX;

            // assigining passengers to lift
            for (int j = 0; j < N; j++)
            {
                int liftLoad = elevatorManager[j][0] + crowdManager[j][0];
                int crowdInWaiting = elevatorManager[j][0];
                if (liftLoad < minLoad)
                {
                    minLoad = liftLoad;
                    minLoadLift = j;
                }

                int liftCurrentFloor = mainShmPtr->elevatorFloors[j];

                if (moveDirection[j] == 0)
                {
                    int tempDist = liftCurrentFloor - passengerStartfloor;
                    if (tempDist < 0)
                    {
                        tempDist *= (-1);
                    }
                    if (tempDist < minDist)
                    {
                        minDist = tempDist;
                        assignedLift = j;
                    }
                }
                else if (moveDirection[j] == 1 && movePassengerVector >= 0 && liftLoad <= 5)
                {
                    if (passengerStartfloor - liftCurrentFloor >= 0)
                    {
                        int tempDist = passengerStartfloor - liftCurrentFloor;
                        if (tempDist < minDist)
                        {
                            minDist = tempDist;
                            assignedLift = j;
                        }
                    }
                }
                else if (moveDirection[j] == -1 && movePassengerVector < 0 && liftLoad <= 5)
                {
                    if (liftCurrentFloor - passengerStartfloor >= 0)
                    {
                        int tempDist = liftCurrentFloor - passengerStartfloor;
                        if (tempDist < minDist)
                        {
                            minDist = tempDist;
                            assignedLift = j;
                        }
                    }
                }
            }
            if (assignedLift == -1)
            {
                assignedLift = minLoadLift;
            }

            int tempSize = elevatorManager[assignedLift][0];
            elevatorManager[assignedLift][tempSize + 1] = passengerRequestId;
            elevatorManager[assignedLift][0]++;
        }


        // dropping passengers on this floor
        for (int i = 0; i < N; i++)
        {
            int temp = crowdManager[i][0];
            for (int j = 1; j <= temp; j++)
            {
                int insidePassengerId = crowdManager[i][j];
                char insidePassengerIdString[1000];
                sprintf(insidePassengerIdString, "%d", insidePassengerId);
                int destFloorOfInsidePassenger = get(&passengerDest, insidePassengerIdString);
                if (destFloorOfInsidePassenger == mainShmPtr->elevatorFloors[i])
                {
                    removeElementFromArray(crowdManager[i], j);
                    temp--;
                    j--;
                    int noOfDropped = mainMsgRequest.droppedPassengersCount;
                    mainShmPtr->droppedPassengers[noOfDropped] = insidePassengerId;
                    mainMsgRequest.droppedPassengersCount++;
                }
            }
        }

        // picking of passengers on this floor
        for (int i = 0; i < N; i++)
        {
            if (crowdManager[i][0] >= 4)
            {
                continue;
            }
            int tempAssignedSize = elevatorManager[i][0];
            for (int j = 1; j <= tempAssignedSize; j++)
            {
                int passengerID = elevatorManager[i][j];
                char passengerIDString[100];
                sprintf(passengerIDString, "%d", passengerID);
                int passengerfloor = get(&passengerStart, passengerIDString);
                if (passengerfloor == mainShmPtr->elevatorFloors[i])
                {
                    removeElementFromArray(elevatorManager[i], j);
                    tempAssignedSize--;
                    int tempSize2 = crowdManager[i][0];
                    crowdManager[i][tempSize2 + 1] = passengerID;
                    crowdManager[i][0]++;
                    int noOfPicked = mainMsgRequest.pickedUpPassengersCount;
                    mainShmPtr->pickedUpPassengers[noOfPicked][0] = passengerID;
                    mainShmPtr->pickedUpPassengers[noOfPicked][1] = i;
                    mainMsgRequest.pickedUpPassengersCount++;
                }
            }
        }
        printf("elevator manager : \n");
        for (int j = 0; j < N; j++)
        {
            printf("%d ", elevatorManager[j][0]);
        }
        printf("\n");
        printf("crowd manager : \n");
        for (int j = 0; j < N; j++)
        {
            printf("%d ", crowdManager[j][0]);
        }
        totalPassengers += mainMsgResponse.newPassengerRequestCount;
        pickedPassengers = pickedPassengers + mainMsgRequest.pickedUpPassengersCount - mainMsgRequest.droppedPassengersCount;
        droppedPassengers += mainMsgRequest.droppedPassengersCount;
        printf("\n\n");
        printf("Turn Number: %d\n", mainMsgResponse.turnNumber);
        printf("Total Pasengers : %d\n", totalPassengers);
        printf("Picked Passengers : %d\n", pickedPassengers);
        printf("Dropped Passengers : %d\n\n", droppedPassengers);

        // assigning elevator to move
        for (int i = 0; i < N; i++)
        {
            if (crowdManager[i][0] == 0)
            {
                if (elevatorManager[i][0] == 0)
                {
                    moveDirection[i] = 0;
                }
                else
                {
                    int passengerIdFollow = elevatorManager[i][1];
                    char passengerIdFollowString[1000];
                    sprintf(passengerIdFollowString, "%d", passengerIdFollow);
                    int nextFloor = get(&passengerStart, passengerIdFollowString);
                    int currentFloorElevator = mainShmPtr->elevatorFloors[i];
                    if (nextFloor > currentFloorElevator)
                    {
                        moveDirection[i] = 1;
                    }
                    else
                    {
                        moveDirection[i] = -1;
                    }
                }
            }
            else
            {
                int passengerIdFollow = crowdManager[i][1];
                char passengerIdFollowString[1000];
                sprintf(passengerIdFollowString, "%d", passengerIdFollow);
                int nextFloor = get(&passengerDest, passengerIdFollowString);
                int currentFloorElevator = mainShmPtr->elevatorFloors[i];
                if (nextFloor > currentFloorElevator)
                {
                    moveDirection[i] = 1;
                }
                else
                {
                    moveDirection[i] = -1;
                }
            }
        }


        // setting elevator movement instructions
        for (int i = 0; i < N; i++)
        {
            if (moveDirection[i] == 1)
            {
                mainShmPtr->elevatorMovementInstructions[i] = 'u';
            }
            else if (moveDirection[i] == -1)
            {
                mainShmPtr->elevatorMovementInstructions[i] = 'd';
            }
            else
            {
                mainShmPtr->elevatorMovementInstructions[i] = 's';
            }
        }

        // finding authString for the elevators
        for (int i = 0; i < N; i++)
        {
            int noOfPeople = noOfPeopleInElevator[i];
            if (moveDirection[i] == 0 || noOfPeople == 0)
            {
                continue;
            }

            int solverKey = solverMsgQueue[0];
            SolverRequest reqAuth;
            reqAuth.elevatorNumber = i;
            reqAuth.mtype = 2;
            int solverId = msgget((key_t)solverKey, PERMS);
            msgsnd(solverId, &reqAuth, sizeof(reqAuth) - sizeof(reqAuth.mtype), 0);

            printf("Auth Cal Size %d with Elevator Number : %d\n", crowdManager[i][0], i);
            char *correctAuthString = generateAuthStrings(noOfPeople, reqAuth, solverId);

            for (int j = 0; j < noOfPeople + 1; j++)
            {
                mainShmPtr->authStrings[i][j] = correctAuthString[j];
            }
        }

        for (int i = 0; i < N; i++)
        {
            noOfPeopleInElevator[i] = crowdManager[i][0];
        }

        msgsnd(mainMsgResponseId, &mainMsgRequest, sizeof(mainMsgRequest) - sizeof(mainMsgRequest.mtype), 0);
        msgrcv(mainMsgResponseId, &mainMsgResponse, sizeof(mainMsgResponse) - sizeof(mainMsgResponse.mtype), 2, 0);
        isFinished = mainMsgResponse.finished;
        int isError = mainMsgResponse.errorOccured;
        if (isError == 1)
        {
            return 0;
        }
    }

    printf("\nProgram Executed\n");

    return 0;
}
