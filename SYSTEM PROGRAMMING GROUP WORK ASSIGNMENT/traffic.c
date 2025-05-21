#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SENSOR_NAME_LEN 20

// Structure to represent a traffic sensor
typedef struct TrafficSensor {
    int id;
    char name[SENSOR_NAME_LEN];
    int status; // 0 = inactive, 1 = active
    float trafficDensity;
    float speed; // Average speed of vehicles detected
    float occupancy; // Percentage of road occupied by vehicles
    int vehicleCount; // Number of vehicles detected
    int queueLength; // Number of vehicles in a queue at a stop
    float waitTime; // Average wait time at a specific stop
} TrafficSensor;

// Function to generate random sensor data
float generateRandomFloat(float min, float max) {
    return min + (rand() / (float)RAND_MAX) * (max - min);
}

// Function to create a new sensor
TrafficSensor* createSensor(int id) {
    TrafficSensor* newSensor = (TrafficSensor*)malloc(sizeof(TrafficSensor));
    if (!newSensor) {
        printf("Memory allocation failed!\n");
        return NULL;
    }
    newSensor->id = id;
    snprintf(newSensor->name, SENSOR_NAME_LEN, "Sensor_%03d", id); // Auto-generate name
    newSensor->status = 1; // Default to active
    newSensor->trafficDensity = generateRandomFloat(10, 100);
    newSensor->speed = generateRandomFloat(20, 120);
    newSensor->occupancy = generateRandomFloat(10, 90);
    newSensor->vehicleCount = rand() % 100;
    newSensor->queueLength = rand() % 50;
    newSensor->waitTime = generateRandomFloat(5, 60);
    return newSensor;
}

// Function to display sensor details
void displaySensor(TrafficSensor* sensor) {
    if (sensor) {
        printf("ID: %03d, Name: %s, Status: %d\n", sensor->id, sensor->name, sensor->status);
        printf("Traffic Density: %.2f, Speed: %.2f km/h, Occupancy: %.2f%%\n", 
               sensor->trafficDensity, sensor->speed, sensor->occupancy);
        printf("Vehicle Count: %d, Queue Length: %d, Wait Time: %.2f seconds\n\n", 
               sensor->vehicleCount, sensor->queueLength, sensor->waitTime);
    } else {
        printf("Invalid sensor!\n");
    }
}

// Function to remove a faulty sensor and simulate garbage collection
void removeSensor(TrafficSensor*** sensors, int* count, int id) {
    int index = -1;
    for (int i = 0; i < *count; i++) {
        if ((*sensors)[i] && (*sensors)[i]->id == id) {
            free((*sensors)[i]); // Free the memory
            (*sensors)[i] = NULL; // Mark as removed
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        printf("Sensor ID %03d not found.\n", id);
        return;
    }
    
    // Shift elements and reallocate memory
    for (int i = index; i < *count - 1; i++) {
        (*sensors)[i] = (*sensors)[i + 1];
    }
    (*count)--;
    *sensors = (TrafficSensor**)realloc(*sensors, (*count) * sizeof(TrafficSensor*));
    if (!(*sensors) && *count > 0) {
        printf("Memory reallocation failed!\n");
    }
    printf("Sensor ID %03d removed successfully.\n", id);
}

// Function to safely read integer input
int safeInputInt(const char* prompt) {
    int value;
    char buffer[100];
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin)) {
            buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
            if (sscanf(buffer, "%d", &value) == 1) {
                return value;
            }
        }
        printf("Invalid input! Please enter a valid number.\n");
    }
}

// Main function
int main() {
    srand(time(NULL)); // Seed random number generator
    int numSensors = safeInputInt("Enter number of sensors: ");
    
    TrafficSensor** sensors = (TrafficSensor**)malloc(numSensors * sizeof(TrafficSensor*));
    if (!sensors) {
        printf("Memory allocation for sensor array failed!\n");
        return 1;
    }

    for (int i = 0; i < numSensors; i++) {
        sensors[i] = createSensor(i + 1);
    }

    int choice, id;
    while (1) {
        printf("\n1. View All Sensors\n2. View Specific Sensor\n3. Add New Sensor\n4. Remove Faulty Sensor\n5. Exit\nEnter choice: ");
        choice = safeInputInt("");
        
        switch (choice) {
            case 1:
                for (int i = 0; i < numSensors; i++) {
                    displaySensor(sensors[i]);
                }
                break;
            case 2:
                id = safeInputInt("Enter Sensor ID to view details: ");
                for (int i = 0; i < numSensors; i++) {
                    if (sensors[i] && sensors[i]->id == id) {
                        displaySensor(sensors[i]);
                        break;
                    }
                }
                break;
            case 3:
                numSensors++;
                sensors = (TrafficSensor**)realloc(sensors, numSensors * sizeof(TrafficSensor*));
                if (!sensors) {
                    printf("Memory allocation failed!\n");
                    return 1;
                }
                sensors[numSensors - 1] = createSensor(numSensors);
                printf("New sensor added successfully with ID %03d\n", numSensors);
                break;
            case 4:
                id = safeInputInt("Enter Sensor ID to remove: ");
                removeSensor(&sensors, &numSensors, id);
                break;
            case 5:
                for (int i = 0; i < numSensors; i++) {
                    free(sensors[i]);
                }
                free(sensors);
                return 0;
            default:
                printf("Invalid choice!\n");
        }
    }
}
