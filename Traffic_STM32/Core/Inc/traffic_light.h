#ifndef INC_TRAFFIC_LIGHT_H_
#define INC_TRAFFIC_LIGHT_H_

#include "main.h"
#include <stdint.h>

/* Traffic light states */
typedef enum {
    TRAFFIC_OFF = 0,
    TRAFFIC_GREEN,
    TRAFFIC_AMBER,
    TRAFFIC_RED
} TrafficColor;

/* Short names for state machine code */
#define OFF     TRAFFIC_OFF
#define GREEN   TRAFFIC_GREEN
#define AMBER   TRAFFIC_AMBER
#define YELLOW  TRAFFIC_AMBER
#define RED     TRAFFIC_RED

#define ROAD_1  1
#define ROAD_2  2

/* Init traffic light pins (turn all off) */
void traffic_init(void);

/* Set light for a road: traffic_set(1, RED), traffic_set(2, GREEN) */
void traffic_set(uint8_t road, TrafficColor color);

/* Get current color of a road */
TrafficColor traffic_get(uint8_t road);

/* Toggle between color and OFF (used for blinking in tuning mode) */
void traffic_toggle(uint8_t road, TrafficColor color);

/* Turn off both roads */
void traffic_all_off(void);

#define traffic_set_state(road, state) traffic_set((road), (state))

#endif /* INC_TRAFFIC_LIGHT_H_ */
