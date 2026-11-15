#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdint.h>

/* State numbers match the four pages in traffic_light_state_machines(1).drawio. */
typedef enum { MODE_AUTO, MODE_MANUAL, MODE_TUNING, MODE_SWITCH_SAFE } traffic_mode_t;
typedef enum { RED1_GREEN2, RED1_AMBER2, GREEN1_RED2, AMBER1_RED2 } traffic_phase_t;
typedef enum { SET_RED, SET_GREEN, SET_AMBER } tuning_state_t;
typedef enum { LIGHT_RED, LIGHT_GREEN, LIGHT_AMBER } traffic_color_t;
typedef struct { uint8_t red, green, amber; } traffic_timings_t;

#define FSM_PERIOD_MS 10U
/* Design choice: the drawing names Tclear but does not specify its duration. */
#define FSM_CLEARANCE_MS 1000U
#define FSM_B1 (1U << 0)
#define FSM_B2 (1U << 1)
#define FSM_B3 (1U << 2)
#define FSM_B4 (1U << 3)

typedef struct {
    traffic_mode_t mode;
    traffic_mode_t pending_mode;
    traffic_phase_t phase;
    tuning_state_t selection;
    traffic_timings_t active;
    traffic_timings_t draft;
    traffic_color_t road1;
    traffic_color_t road2;
    uint32_t remaining_ms;
    uint8_t validation_error;
} traffic_state_t;

/* Main/scheduler context only. Drivers read this state; FSMs own its writes. */
extern traffic_state_t traffic;
void traffic_phase_enter(traffic_phase_t phase);
uint8_t traffic_timings_valid(const traffic_timings_t *timings);
/* AUTO only: red countdown includes the following amber phase. */
uint8_t traffic_countdown(uint8_t road);

#endif
