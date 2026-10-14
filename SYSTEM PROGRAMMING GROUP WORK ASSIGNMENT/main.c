/*
 * main.c — Menu-driven front end for the Smart City Traffic Light Data
 * System. Hardened input handling and wiring for the sensor and GC
 * modules (ICS 2305, Question 2).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "traffic.h"

static TrafficSensor** g_sensors = NULL;
static int g_count = 0;
static int g_tick = 0;

/* Single shutdown path: free everything, then prove nothing leaked. */
static void cleanupAndExit(int code) {
    printf("\nShutting down: freeing all sensors...\n");
    for (int i = 0; i < g_count; i++) {
        destroySensor(g_sensors[i]);
    }
    free(g_sensors);
    g_sensors = NULL;
    g_count = 0;
    leakCheckOnExit();
    exit(code);
}

/*
 * Bullet 2 (input hardening): range-checked integer reader. Rejects
 * non-numeric input, drains over-long lines so no residue is left in
 * stdin, and exits cleanly at end-of-input.
 */
static int safeInputInt(const char* prompt, int min, int max) {
    char buffer[128];
    int value;
    char extra;
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(buffer, (int)sizeof buffer, stdin)) {
            printf("\nEnd of input detected.\n");
            cleanupAndExit(0);
        }
        /* Line longer than the buffer? Drain the rest so nothing lingers. */
        if (!strchr(buffer, '\n') && !feof(stdin)) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
            printf("Input too long! Enter a number between %d and %d.\n", min, max);
            continue;
        }
        /* Exactly one integer and nothing else on the line */
        if (sscanf(buffer, "%d %c", &value, &extra) == 1) {
            if (value < min || value > max) {
                printf("Out of range! Enter a number between %d and %d.\n", min, max);
                continue;
            }
            return value;
        }
        printf("Invalid input! Enter a number between %d and %d.\n", min, max);
    }
}

/* Read one line into out[size]; drains any residue of an over-long line. */
static void safeReadLine(const char* prompt, char* out, size_t size) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(out, (int)size, stdin)) {
        printf("\nEnd of input detected.\n");
        cleanupAndExit(0);
    }
    if (!strchr(out, '\n') && !feof(stdin)) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
        printf("(note: input longer than %zu chars was cut off)\n", size - 1);
    }
    out[strcspn(out, "\n")] = '\0';
}

/* New ids are always maxExistingId + 1 - removal can never cause collisions */
static int nextSensorId(TrafficSensor** sensors, int count) {
    int maxId = 0;
    for (int i = 0; i < count; i++) {
        if (sensors[i] && sensors[i]->id > maxId) maxId = sensors[i]->id;
    }
    return maxId + 1;
}

static void printMenu(void) {
    printf("\n===== Smart City Traffic Light System ===== (tick %d, %d sensor(s))\n",
           g_tick, g_count);
    printf("1. View all sensors\n");
    printf("2. View sensor by ID\n");
    printf("3. Add sensor\n");
    printf("4. Simulate new sensor readings (tick)\n");
    printf("5. Toggle sensor active/inactive\n");
    printf("6. Rename sensor (overflow-guard demo)\n");
    printf("7. Manually remove sensor (immediate free)\n");
    printf("8. Simulate corrupt data (buffer-overflow demo)\n");
    printf("9. Run garbage collection\n");
    printf("10. Memory statistics\n");
    printf("11. Exit (free all + leak check)\n");
    printf("Enter choice: ");
}


int main(void) {
    srand((unsigned)time(NULL));
    printf("=== Smart City Traffic Light Data System ===\n");
    printf("(ICS 2305 Systems Programming - Question 2)\n\n");

    int numSensors = safeInputInt("Enter number of sensors (1-100): ", 1, MAX_SENSORS);
    g_sensors = (TrafficSensor**)malloc((size_t)numSensors * sizeof *g_sensors);
    if (!g_sensors) {
        printf("Memory allocation for sensor array failed!\n");
        return 1;
    }
    for (int i = 0; i < numSensors; i++) {
        g_sensors[i] = createSensor(i + 1, g_tick);
        if (!g_sensors[i]) cleanupAndExit(1);   /* partial state still freed */
        g_count++;
    }
    printf("Created %d sensor(s).\n", g_count);

    int choice, id;
    for (;;) {
        printMenu();
        choice = safeInputInt("", 1, 11);
        switch (choice) {
            case 1:
                if (g_count == 0) printf("No sensors registered.\n");
                for (int i = 0; i < g_count; i++) displaySensor(g_sensors[i]);
                break;

            case 2:
                id = safeInputInt("Enter Sensor ID: ", 1, 1000000);
                {
                    TrafficSensor* s = findSensorById(g_sensors, g_count, id);
                    if (s) displaySensor(s);
                    else   printf("Sensor ID %03d not found.\n", id);
                }
                break;

            case 3:
                if (g_count >= MAX_SENSORS) {
                    printf("Sensor limit (%d) reached.\n", MAX_SENSORS);
                    break;
                }
                {
                    /* Grow through a temp pointer: original stays valid on failure */
                    TrafficSensor** temp = (TrafficSensor**)
                        realloc(g_sensors, (size_t)(g_count + 1) * sizeof *g_sensors);
                    if (!temp) {
                        printf("Reallocation failed - array unchanged (no leak).\n");
                        break;
                    }
                    g_sensors = temp;
                    int newId = nextSensorId(g_sensors, g_count);
                    g_sensors[g_count] = createSensor(newId, g_tick);
                    if (!g_sensors[g_count]) {
                        printf("Sensor creation failed.\n");
                        break;
                    }
                    g_count++;
                    printf("New sensor added successfully with ID %03d.\n", newId);
                }
                break;

            case 4:
                g_tick++;
                {
                    int updated = 0;
                    for (int i = 0; i < g_count; i++) {
                        if (g_sensors[i] && g_sensors[i]->status == 1 &&
                            !g_sensors[i]->isOrphaned) {
                            updateReadings(g_sensors[i], g_tick);
                            updated++;
                        }
                    }
                    printf("Tick %d: %d active sensor(s) reported new readings.\n",
                           g_tick, updated);
                }
                break;

            case 5:
                id = safeInputInt("Enter Sensor ID to toggle: ", 1, 1000000);
                {
                    TrafficSensor* s = findSensorById(g_sensors, g_count, id);
                    if (!s) { printf("Sensor ID %03d not found.\n", id); break; }
                    s->status = !s->status;
                    if (s->status) {
                        s->isOrphaned = 0;          /* revived: un-mark it */
                        s->lastUpdateTick = g_tick;
                        printf("Sensor %03d is now ACTIVE (orphan mark cleared).\n", id);
                    } else {
                        printf("Sensor %03d is now INACTIVE - after %d idle ticks "
                               "the GC will reclaim it.\n", id, STALE_TICKS);
                    }
                }
                break;

            case 6:
                id = safeInputInt("Enter Sensor ID to rename: ", 1, 1000000);
                {
                    TrafficSensor* s = findSensorById(g_sensors, g_count, id);
                    if (!s) { printf("Sensor ID %03d not found.\n", id); break; }
                    char name[100];
                    safeReadLine("New name: ", name, sizeof name);
                    safeRenameSensor(s, name);
                }
                break;

            case 7:
                id = safeInputInt("Enter Sensor ID to remove: ", 1, 1000000);
                removeSensorById(&g_sensors, &g_count, id);
                break;

            case 8:
                id = safeInputInt("Enter Sensor ID to corrupt: ", 1, 1000000);
                {
                    TrafficSensor* s = findSensorById(g_sensors, g_count, id);
                    if (!s) { printf("Sensor ID %03d not found.\n", id); break; }
                    simulateCorruptData(s);
                    unsigned int flags = validateSensor(s);
                    if (flags != VALID_OK) {
                        printValidationIssues(flags);
                        s->isOrphaned = 1;         /* quarantine for the GC */
                        s->status = 0;
                        printf("Sensor %03d QUARANTINED - run garbage collection "
                               "to reclaim it.\n", id);
                    } else {
                        printf("Validation passed (corruption not detected?!).\n");
                    }
                }
                break;

            case 9:
                garbageCollect(&g_sensors, &g_count, g_tick);
                break;

            case 10:
                memStatsReport();
                break;

            case 11:
                cleanupAndExit(0);
                break;
        }
    }
}
