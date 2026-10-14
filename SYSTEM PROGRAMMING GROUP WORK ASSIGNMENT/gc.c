/*
 * gc.c — Allocation registry (leak prevention) and simulated garbage
 * collection for orphaned data. Bullets 3 and 4 of the assignment.
 */
#include <stdio.h>
#include <stdlib.h>
#include "traffic.h"

static MemStats g_stats = {0, 0, 0, 0, 0};

/* ---- allocation registry ------------------------------------------------ */

void registryOnAlloc(size_t bytes) {
    g_stats.live++;
    g_stats.totalCreated++;
    g_stats.bytes += bytes;
    if (g_stats.bytes > g_stats.peak) g_stats.peak = g_stats.bytes;
}

void registryOnFree(size_t bytes) {
    if (g_stats.live > 0) g_stats.live--;
    g_stats.totalFreed++;
    g_stats.bytes = (g_stats.bytes >= bytes) ? g_stats.bytes - bytes : 0;
}

const MemStats* memStats(void) {
    return &g_stats;
}

void memStatsReport(void) {
    const MemStats* m = &g_stats;
    printf("\n===== Memory statistics =====\n");
    printf("Live allocations : %d sensor(s), %zu bytes\n", m->live, m->bytes);
    printf("Peak usage       : %zu bytes\n", m->peak);
    printf("Created / freed  : %d / %d\n", m->totalCreated, m->totalFreed);
    printf("=============================\n");
}

/* Bullet 3: prove on shutdown that nothing leaked */
int leakCheckOnExit(void) {
    if (g_stats.live == 0) {
        printf("No memory leaks detected (0 live allocations).\n");
        return 1;
    }
    printf("LEAK WARNING: %d allocation(s) totalling %zu bytes were not freed!\n",
           g_stats.live, g_stats.bytes);
    return 0;
}

/*
 * Compact the array to writeIdx entries. Shrinking is best-effort: if
 * realloc fails the old (larger, still valid) block is kept — the
 * pointer is never lost and nothing leaks.
 */
static void compactSensors(TrafficSensor*** sensors, int* count, int writeIdx) {
    *count = writeIdx;
    if (writeIdx > 0) {
        TrafficSensor** temp = (TrafficSensor**)
            realloc(*sensors, (size_t)writeIdx * sizeof *temp);
        if (temp) {
            *sensors = temp;
        } else {
            printf("  (array shrink skipped: realloc failed, original block kept)\n");
        }
    }
}

/* Bullet 3: immediate, safe removal of a faulty sensor */
int removeSensorById(TrafficSensor*** sensors, int* count, int id) {
    if (!sensors || !*sensors || *count <= 0) {
        printf("No sensors registered.\n");
        return -1;
    }
    TrafficSensor** arr = *sensors;
    for (int i = 0; i < *count; i++) {
        if (arr[i] && arr[i]->id == id) {
            printf("Removing sensor ID %03d (%.*s) - %zu bytes freed.\n",
                   id, (int)SENSOR_NAME_LEN, arr[i]->name, sizeof(TrafficSensor));
            destroySensor(arr[i]);
            int w = 0;
            for (int r = 0; r < *count; r++) {
                if (r != i) arr[w++] = arr[r];
            }
            compactSensors(sensors, count, w);
            return 0;
        }
    }
    printf("Sensor ID %03d not found.\n", id);
    return -1;
}

/*
 * Bullet 4, phase 1 (mark): a sensor becomes an orphan when its data
 * fails validation (corrupt) or when it has been inactive for
 * STALE_TICKS consecutive ticks (stopped reporting).
 */
int markOrphans(TrafficSensor** sensors, int count, int tick) {
    int marked = 0;
    if (!sensors) return 0;
    for (int i = 0; i < count; i++) {
        TrafficSensor* s = sensors[i];
        if (!s || s->isOrphaned) continue;
        unsigned int flags = validateSensor(s);
        if (flags != VALID_OK) {
            s->isOrphaned = 1;
            s->status = 0;
            printf("MARK: sensor ");
            /* bounded print: name may be deliberately unterminated */
            printf("%.*s", (int)SENSOR_NAME_LEN, s->name);
            printf(" (ID %03d) corrupt (flags 0x%X) -> orphaned.\n", s->id, flags);
            marked++;
        } else if (s->status == 0 && (tick - s->lastUpdateTick) >= STALE_TICKS) {
            s->isOrphaned = 1;
            printf("MARK: sensor %.*s (ID %03d) silent for %d ticks -> orphaned.\n",
                   (int)SENSOR_NAME_LEN, s->name, s->id, STALE_TICKS);
            marked++;
        }
    }
    return marked;
}

/*
 * Bullet 4, phase 2 (sweep): free every marked sensor, compact the
 * array and report exactly what was reclaimed.
 */
int garbageCollect(TrafficSensor*** sensors, int* count, int tick) {
    if (!sensors || !*sensors || !count || *count <= 0) {
        printf("GC: nothing to collect.\n");
        return 0;
    }
    TrafficSensor** arr = *sensors;
    printf("\n--- Garbage collection pass (tick %d) ---\n", tick);
    int marked = markOrphans(arr, *count, tick);
    int reclaimed = 0;
    size_t freedBytes = 0;
    int w = 0;
    for (int r = 0; r < *count; r++) {
        TrafficSensor* s = arr[r];
        if (s && s->isOrphaned) {
            printf("SWEEP: reclaiming orphan %.*s (ID %03d).\n",
                   (int)SENSOR_NAME_LEN, s->name, s->id);
            destroySensor(s);
            freedBytes += sizeof(TrafficSensor);
            reclaimed++;
        } else {
            arr[w++] = s;
        }
    }
    compactSensors(sensors, count, w);
    printf("GC complete: %d marked, %d reclaimed, %zu bytes freed, %d sensor(s) remain.\n",
           marked, reclaimed, freedBytes, *count);
    return reclaimed;
}