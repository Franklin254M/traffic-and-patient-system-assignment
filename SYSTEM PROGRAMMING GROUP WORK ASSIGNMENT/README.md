# Smart City Traffic Light Data System — ICS 2305 Question 2

C implementation of the traffic light data system required by the
ICS 2305 Systems Programming group assignment (Question 2).

## Assignment requirements → where they live

| Requirement | Where |
|---|---|
| Dynamic allocation/deallocation of traffic sensors | `sensor.c` — `createSensor()`, `destroySensor()`; array growth/shrink in `main.c` and `gc.c` (`compactSensors`) |
| Buffer overflow handling / corrupt sensor data | `sensor.c` — `validateSensor()` (guard word + NUL-termination + range checks), `safeRenameSensor()` (rejects over-long input), `simulateCorruptData()` (demo); hardened input in `main.c` (`safeInputInt`, `safeReadLine`) |
| Memory leak prevention | Allocation registry in `gc.c` (`registryOnAlloc/OnFree`, `memStatsReport`, `leakCheckOnExit`); every exit path frees everything and reports |
| Simulated garbage collection of orphaned data | `gc.c` — `markOrphans()` (mark phase: corrupt or stale sensors) + `garbageCollect()` (sweep phase: free + compact + report) |

## Files

- `traffic.h` — shared types (`TrafficSensor`, `LightState`, `MemStats`), constants, prototypes
- `sensor.c` — sensor lifecycle, readings, validation, corruption demo
- `gc.c` — allocation registry, removal, mark-and-sweep garbage collector
- `main.c` — menu front end and hardened input
- `Makefile` — `mingw32-make` build (gcc, `-Wall -Wextra -Wpedantic -Werror`)
- `tests/basic.txt` — scripted session exercising every feature
- `traffic_sample_output.txt` — captured session output for the report
- `traffic.c` — original single-file starting point (kept for reference)

## Building

```
mingw32-make            # or: make
```

or without make:

```
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -g main.c sensor.c gc.c -o traffic.exe
```

## Running

```
traffic.exe
```

Scripted test (same session used for `traffic_sample_output.txt`):

```
traffic.exe < tests\basic.txt
```

## Notes

- New sensor ids are `maxExistingId + 1`, so removals can never create duplicate ids.
- Sensors inactive for 3 consecutive ticks (`STALE_TICKS`) or with corrupt data are
  marked orphaned; the garbage collector frees them on demand (menu 9).
- Every shutdown path (menu 11, EOF, allocation failure) frees all sensors and
  prints a leak check — expected result: *No memory leaks detected*.
