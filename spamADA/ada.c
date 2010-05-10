/* ada.c */
/*
 * ADA for SPAM, it was time to change that damn directory and project files :3
 *
 */ 
#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"

/*
 * Static definition of Upper and Lower Gradient bounds. I know its not really
 * clean, but it's only for test purposes... :3
 *
 */

#define UPPER_LIMIT 620
#define LOWER_LIMIT 540

/*
 * Defining PID controller's data.
 */

#define P   5           // k in the PID control digital equation
#define I   2           // k/Ti in the PID control digital equation
#define D   1           // kTd in the PID control digital equation

#define DELTA_T   0.01  // T is 10ms, in seconds :3

/*
 * Using definitions to speed up test development
 */
 
#define SONAR_PORT            NXT_PORT_S1
#define LIGHTS_PORT           NXT_PORT_S2

/*
 * Defining Motor ports. They will hardly change, but it's for compatibility
 */

#define LEFT_MOTOR            NXT_PORT_C
#define RIGHT_MOTOR           NXT_PORT_B
#define SONAR_MOTOR           NXT_PORT_A

/* OSEK declarations */
DeclareCounter(SysTimerCnt);
DeclareTask(Sonar_Check);
DeclareTask(LCD_Update);
DeclareTask(DPID_Controller);

/* Global Declarations */

U32 last_reading = 0;       // Last grad count reading 
U32 degpsreadings[10];      // Circular array of 10 - 10ms readings
U32 average_slot = 0;       // What reading in the circular array we should write into 
U32 speeds[3];              // Last 3 speed readings

U32 motor_power = 0;        // Motors power
U32 VTEC = 2;               // RPM Value, as in VTEC Just Kicked In, Yo!

U32 getdegpscount() {
  U32 degsum = 0;
  
  for (int j = 0; j < 10; j++) {
    degsum += degpsreadings[j];
  }
  
  return degsum * 10;
}

U32 DPID_output (U32 last, U32 target_speed) {
  float Ti = (P / I);
  float TD = (D / P);
  float next;
  
  /* PID Digital Equation FTW */
  
  next = last + P *
            ( ( ( 1 + ( DELTA_T / Ti ) + ( TD / DELTA_T ) ) * (target_speed - speeds[0] ) ) -
            ( ( 1 + 2 * ( TD / DELTA_T ) ) * ( target_speed - speeds[1] ) ) +
            ( ( TD / DELTA_T ) * ( target_speed - speeds[2] ) ) );
  
  return (U32)next;
  
}

/* 
 * LEJOS OSEK hooks ~ Initialization
 *
 * In this case we initialize every Sensor we need. We could also add some
 * juicy starting routines, let's see...
 *
 */
void ecrobot_device_initialize() {
	ecrobot_init_sonar_sensor(SONAR_PORT);
  ecrobot_set_light_sensor_active(LIGHTS_PORT); 
}

/*
 * LEJOS OSEK hooks ~ Shutting Down... :3
 */
void ecrobot_device_terminate() {
  display_clear(0);
  
  display_goto_xy(0, 1);
  display_string("Shut Down...");
  
  display_update();

  nxt_motor_set_speed (LEFT_MOTOR, 0, 1);
  nxt_motor_set_speed (RIGHT_MOTOR, 0, 1);
  
	ecrobot_term_sonar_sensor(SONAR_PORT);
  ecrobot_set_light_sensor_inactive(LIGHTS_PORT); 
}

/* LEJOS OSEK hook to be invoked from an ISR in category 2 */
void user_1ms_isr_type2(void)
{
  StatusType ercd;

  ercd = SignalCounter(SysTimerCnt); /* Increment OSEK Alarm Counter */
  if(ercd != E_OK) {
    ShutdownOS(ercd);
  }
}

/* Task1 executed every 50msec */
TASK (Sonar_Check) {
	
  ecrobot_get_sonar_sensor(SONAR_PORT);
	
 	TerminateTask();
}

TASK (LCD_Update) {
  
  //ecrobot_status_monitor("Sonar Test");
  display_clear(0);
  
  display_goto_xy(0, 1);
  display_string("DISTANCE:");
  display_unsigned(ecrobot_get_sonar_sensor(SONAR_PORT), 0);
  
  display_goto_xy(0, 2);
  display_string("LIGHT:");
  display_unsigned(ecrobot_get_light_sensor(LIGHTS_PORT), 0);
  
  display_goto_xy(0, 3);
  display_string("DegPS:");
  display_unsigned(getdegpscount(), 0);
  
  display_update();
  
  TerminateTask();
}

TASK (DPID_Controller){
  U32 now;
  
  /* Updating 10ms readings */
  now = nxt_motor_get_count(RIGHT_MOTOR);
  degpsreadings[average_slot] = now - last_reading;
  average_slot = ((average_slot == 9) ? 0 : average_slot+1);
  last_reading = now;
  
  /* Updating speed readings */
  speeds[2] = speeds[1];
  speeds[1] = speeds[0];
  speeds[0] = getdegpscount();
  
  /* Get Next Motors' Power */
  motor_power = DPID_output ( motor_power, (VTEC * 360) );
  
  /* Set Motors' Speed */
  nxt_motor_set_speed(RIGHT_MOTOR, motor_power , 0);
  
  TerminateTask();
  
}
