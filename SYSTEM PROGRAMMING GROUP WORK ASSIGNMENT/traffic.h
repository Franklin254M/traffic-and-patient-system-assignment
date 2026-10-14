/*
 * traffic.h — Smart City Traffic Light Data System (ICS 2305, Question 2)
 * Shared types, constants, validation flags and function prototypes.
 */
#ifndef TRAFFIC_H
#define TRAFFIC_H

#include <stddef.h>

#define SENSOR_NAME_LEN 20               /* including the NUL terminator  */
#define MAX_SENSORS     100              /* hard cap on live sensors      */
#define STALE_TICKS     3                /* idle ticks before orphaning   */
#define SENSOR_CANARY   0xC0FFEE42u      /* guard word vs. buffer overrun */

/* Signal phase a sensor currently feeds */
typedef enum {
    LIGHT_RED = 0,
    LIGHT_AMBER = 1,
    LIGHT_GREEN = 2
} LightState;

/* Structure to represent a traffic sensor */
typedef struct TrafficSensor {
    int   id;
    char  name[SENSOR_NAME_LEN];
    int   status;            /* 0 = inactive, 1 = active            */
    int   isOrphaned;        /* GC mark bit: 1 = queued for sweep   */
    LightState lightState;   /* signal this sensor currently feeds  */
    int   greenRemaining;    /* seconds of green time remaining     */
    float trafficDensity;    /* 0 – 100 (%)                        */
    float speed;             /* 0 – 200 (km/h)                     */
    float occupancy;         /* 0 – 100 (%)                        */
    int   vehicleCount;      /* >= 0                               */
    int   queueLength;       /* >= 0                               */
    float waitTime;          /* 0 – 300 (seconds)                  */
    int   lastUpdateTick;    /* simulation tick of last reading    */
    unsigned int canary;     /* overrun guard (SENSOR_CANARY)      */
} TrafficSensor;

/* Bit flags returned by validateSensor() */
#define VALID_OK          0u
#define VALID_BAD_CANARY  (1u << 0)
#define VALID_BAD_NAME    (1u << 1)
#define VALID_BAD_DENSITY (1u << 2)
#define VALID_BAD_SPEED   (1u << 3)
#define VALID_BAD_OCCUP   (1u << 4)
#define VALID_BAD_COUNT   (1u << 5)
#define VALID_BAD_QUEUE   (1u << 6)
#define VALID_BAD_WAIT    (1u << 7)
#define VALID_BAD_GREEN   (1u << 8)

/* Allocation registry (gc.c) — leak prevention bookkeeping */
typedef struct {
    int    live;         /* live sensor allocations right now      */
    size_t bytes;        /* bytes held by live allocations         */
    size_t peak;         /* high-water mark of bytes held          */
    int    totalCreated; /* sensors created since program start    */
    int    totalFreed;   /* sensors destroyed since program start  */
} MemStats;

/* sensor.c — lifecycle, data and validation ------------------------------ */
float          generateRandomFloat(float min, float max);
TrafficSensor* createSensor(int id, int tick);
void           destroySensor(TrafficSensor* s);
int            updateReadings(TrafficSensor* s, int tick);
void           displaySensor(const TrafficSensor* s);
TrafficSensor* findSensorById(TrafficSensor** sensors, int count, int id);
unsigned int   validateSensor(const TrafficSensor* s);
void           printValidationIssues(unsigned int flags);
const char*    lightStateName(LightState st);
int            safeRenameSensor(TrafficSensor* s, const char* input);
void           simulateCorruptData(TrafficSensor* s);

/* gc.c — registry, removal and garbage collection ------------------------ */
void            registryOnAlloc(size_t bytes);
void            registryOnFree(size_t bytes);
const MemStats* memStats(void);
void            memStatsReport(void);
int             leakCheckOnExit(void);
int             removeSensorById(TrafficSensor*** sensors, int* count, int id);
int             markOrphans(TrafficSensor** sensors, int count, int tick);
int             garbageCollect(TrafficSensor*** sensors, int* count, int tick);

#endif /* TRAFFIC_H */