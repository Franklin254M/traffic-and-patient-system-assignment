/*
 * sensor.c — Traffic sensor lifecycle, data generation and validation.
 * Bullet 1 (dynamic allocation) and Bullet 2 (buffer overflow handling).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "traffic.h"

/* Function to generate random sensor data within a range */
float generateRandomFloat(float min, float max) {
    return min + (rand() / (float)RAND_MAX) * (max - min);
}

/* Function to create a new sensor (bullet 1: dynamic allocation) */
TrafficSensor* createSensor(int id, int tick) {
    TrafficSensor* s = (TrafficSensor*)malloc(sizeof *s);
    if (!s) {
        printf("Memory allocation failed for sensor %03d!\n", id);
        return NULL;
    }
    memset(s, 0, sizeof *s);              /* deterministic starting state */
    s->id = id;
    snprintf(s->name, SENSOR_NAME_LEN, "Sensor_%03d", id);  /* never overflows */
    s->status = 1;                        /* default to active */
    s->isOrphaned = 0;
    s->lightState = (LightState)(rand() % 3);
    s->greenRemaining = rand() % 61;
    s->trafficDensity = generateRandomFloat(10, 100);
    s->speed = generateRandomFloat(20, 120);
    s->occupancy = generateRandomFloat(10, 90);
    s->vehicleCount = rand() % 100;
    s->queueLength = rand() % 50;
    s->waitTime = generateRandomFloat(5, 60);
    s->lastUpdateTick = tick;
    s->canary = SENSOR_CANARY;            /* arm the overrun guard */
    registryOnAlloc(sizeof *s);           /* register the allocation */
    return s;
}

/* Function to deallocate a sensor (bullet 3: no leaks, ever) */
void destroySensor(TrafficSensor* s) {
    if (!s) return;
    s->canary = 0;                        /* poison: use-after-free tripwire */
    registryOnFree(sizeof *s);
    free(s);
}

/* Simulate a fresh sensor reading; only active sensors report */
int updateReadings(TrafficSensor* s, int tick) {
    if (!s) return -1;
    s->trafficDensity = generateRandomFloat(10, 100);
    s->speed = generateRandomFloat(20, 120);
    s->occupancy = generateRandomFloat(10, 90);
    s->vehicleCount = rand() % 100;
    s->queueLength = rand() % 50;
    s->waitTime = generateRandomFloat(5, 60);
    s->lightState = (LightState)(rand() % 3);
    s->greenRemaining = rand() % 61;
    s->lastUpdateTick = tick;
    s->canary = SENSOR_CANARY;
    return 0;
}

const char* lightStateName(LightState st) {
    switch (st) {
        case LIGHT_RED:   return "RED";
        case LIGHT_AMBER: return "AMBER";
        case LIGHT_GREEN: return "GREEN";
        default:          return "UNKNOWN";
    }
}

/*
 * Print a sensor name without ever running off the end of the buffer,
 * even if corruption removed the NUL terminator. The precision limits
 * how many characters printf may read.
 */
static void printSensorName(const TrafficSensor* s) {
    printf("%.*s", (int)SENSOR_NAME_LEN, s->name);
}

/* Function to display sensor details */
void displaySensor(const TrafficSensor* s) {
    if (!s) {
        printf("Invalid sensor!\n");
        return;
    }
    printf("ID: %03d, Name: ", s->id);
    printSensorName(s);
    printf(", Status: %s, Light: %s (%ds)\n",
           s->status ? "ACTIVE" : "INACTIVE",
           lightStateName(s->lightState), s->greenRemaining);
    printf("Traffic Density: %.2f, Speed: %.2f km/h, Occupancy: %.2f%%\n",
           s->trafficDensity, s->speed, s->occupancy);
    printf("Vehicle Count: %d, Queue Length: %d, Wait Time: %.2f seconds\n",
           s->vehicleCount, s->queueLength, s->waitTime);
    printf("Last update tick: %d%s\n\n", s->lastUpdateTick,
           s->isOrphaned ? "  [ORPHANED - queued for GC]" : "");
}

/* Linear search by id; skips empty slots */
TrafficSensor* findSensorById(TrafficSensor** sensors, int count, int id) {
    if (!sensors) return NULL;
    for (int i = 0; i < count; i++) {
        if (sensors[i] && sensors[i]->id == id) return sensors[i];
    }
    return NULL;
}

/*
 * Bullet 2: detect corrupt sensor data. Checks the guard word, that the
 * name is NUL-terminated inside its buffer, and that every numeric field
 * is within its physically possible range.
 */
unsigned int validateSensor(const TrafficSensor* s) {
    unsigned int flags = VALID_OK;
    if (!s) return VALID_BAD_CANARY;
    if (s->canary != SENSOR_CANARY)                        flags |= VALID_BAD_CANARY;
    if (!memchr(s->name, '\0', SENSOR_NAME_LEN))           flags |= VALID_BAD_NAME;
    if (s->trafficDensity < 0.0f || s->trafficDensity > 100.0f) flags |= VALID_BAD_DENSITY;
    if (s->speed < 0.0f || s->speed > 200.0f)              flags |= VALID_BAD_SPEED;
    if (s->occupancy < 0.0f || s->occupancy > 100.0f)      flags |= VALID_BAD_OCCUP;
    if (s->vehicleCount < 0)                               flags |= VALID_BAD_COUNT;
    if (s->queueLength < 0)                                flags |= VALID_BAD_QUEUE;
    if (s->waitTime < 0.0f || s->waitTime > 300.0f)        flags |= VALID_BAD_WAIT;
    if (s->greenRemaining < 0)                             flags |= VALID_BAD_GREEN;
    return flags;
}

void printValidationIssues(unsigned int flags) {
    if (flags & VALID_BAD_CANARY)  printf("  ! guard word broken - buffer overrun detected\n");
    if (flags & VALID_BAD_NAME)    printf("  ! name not NUL-terminated within its buffer\n");
    if (flags & VALID_BAD_DENSITY) printf("  ! traffic density out of range (0-100)\n");
    if (flags & VALID_BAD_SPEED)   printf("  ! speed out of range (0-200 km/h)\n");
    if (flags & VALID_BAD_OCCUP)   printf("  ! occupancy out of range (0-100%%)\n");
    if (flags & VALID_BAD_COUNT)   printf("  ! vehicle count is negative\n");
    if (flags & VALID_BAD_QUEUE)   printf("  ! queue length is negative\n");
    if (flags & VALID_BAD_WAIT)    printf("  ! wait time out of range (0-300 s)\n");
    if (flags & VALID_BAD_GREEN)   printf("  ! green timer is negative\n");
}

/*
 * Bullet 2 (prevention): a rename path that refuses over-long input
 * instead of silently truncating - the overflow never reaches the buffer.
 */
int safeRenameSensor(TrafficSensor* s, const char* input) {
    if (!s || !input) return -1;
    size_t len = strlen(input);
    if (len >= SENSOR_NAME_LEN) {
        printf("Rename REJECTED: %zu chars exceeds buffer of %d. "
               "Sensor name left untouched (overflow prevented).\n",
               len, SENSOR_NAME_LEN - 1);
        return -1;
    }
    memcpy(s->name, input, len + 1);      /* safe: length already checked */
    printf("Sensor %03d renamed to \"%s\".\n", s->id, s->name);
    return 0;
}


/*
 * Bullet 2 (detection demo): deliberately corrupt a sensor record so the
 * validation pass has something real to catch. Also performs a bounded
 * stack-buffer overflow whose length is only known at runtime.
 */
void simulateCorruptData(TrafficSensor* s) {
    if (!s) return;
    printf("--- Corruption demo on sensor %03d ---\n", s->id);

    /* (a) bounded stack overflow: 8-byte buffer + 4-byte guard word,
     *     payload length computed at runtime so it is not compile-time
     *     visible; the guard word below absorbs the 4 overflow bytes. */
    char sandbox[8];
    unsigned int guard = SENSOR_CANARY;
    const char* payload = "OVERFLOWDEMO";          /* exactly 12 bytes   */
    volatile size_t overflowLen = strlen(payload);
    memcpy(sandbox, payload, overflowLen);          /* overruns sandbox   */
    if (guard != SENSOR_CANARY) {
        printf("  guard word tripped - stack buffer overflow detected!\n");
    } else {
        printf("  overflow executed; guard word verified post-write.\n");
    }
    (void)sandbox;

    /* (b) poison the sensor record itself */
    s->trafficDensity = 999.0f;       /* out of range */
    s->speed = -42.0f;                /* out of range */
    s->occupancy = 150.0f;            /* out of range */
    s->vehicleCount = -7;             /* negative     */
    memset(s->name, 'A', SENSOR_NAME_LEN);  /* remove NUL terminator */
    s->canary ^= 0xDEADBABEu;              /* smash the guard word   */
    printf("  record poisoned: out-of-range values, unterminated name, broken guard.\n");
}
