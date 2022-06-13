#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define HCI_VSC_WAKE_ON_BLE         0xFE54
#define CUSTOMER_AMLOGIC            0x05
#define MANUFACTURE_PATTERN         0x01
/********************************************************************
* VSC + CUSTOMER + MANUFACTURE_PATTERN(0x01) + PATTERN + PATTERN_MASK
*********************************************************************/
#define DEFAULT_HOSTWAKE_PIN_STATE  0x02
/********************************************************************
* VSC + CUSTOMER + DEFAULT_HOSTWAKE_PIN_STATE + state.  1 is default high, 0 is default low
*********************************************************************/
#define HOSTWAKE_PIN_PATTERN 0x03
/********************************************************************
* VSC + CUSTOMER + HOSTWAKE_PIN_PATTERN + [idx][key][pulse_time].  The real pulse time is pulse_time*12.5ms
*********************************************************************/
#define POLLING_WAKE_ON_BLE 0x04
/********************************************************************
* VSC + CUSTOMER + POLLING_WAKE_ON_BLE
*********************************************************************/

void* wole_vsc_write_thread( void *ptr);
void wole_config_write_manufacture_pattern(void);
void wole_config_start();
void wole_config_write_pulse_time(const uint8_t *pattern,int len);
void wole_config_write_default_hostwake_state(int default_state);
