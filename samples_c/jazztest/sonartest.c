/* sonartest.c */ 
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
 * Defining max SPAM speed.
 */

#define TOP_SPEED 85 

/*
 * Using definitions to speed up test development
 */
 
#define SONAR_PORT            NXT_PORT_S1
#define LIGHTS_PORT           NXT_PORT_S2

/*
 * Defining Motor ports. They will hardly change, but it's for compatibility
 */

#define BOTH_MOTORS           NXT_PORT_BC
#define LEFT_MOTOR            NXT_PORT_C
#define RIGHT_MOTOR           NXT_PORT_B
#define SONAR_MOTOR           NXT_PORT_A

/* OSEK declarations */
DeclareCounter(SysTimerCnt);
DeclareTask(Task1);
DeclareTask(Task2);
DeclareTask(DegPSCounter);

/* Global Declarations */

U32 degpsreadings[1000];
int i = 0;

U32 getdegpscount() {
  U32 degsum = 0;
  
  for (int j = 0; j <= 1000; j++) {
    degsum += degpsreadings[j];
  }
  
  return degsum / 1000;
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
TASK (Task1) {
	
  ecrobot_get_sonar_sensor(SONAR_PORT);
	
 	TerminateTask();
}

TASK (Task2) {

  nxt_motor_set_speed(RIGHT_MOTOR, 50, 1);
  
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

TASK (DegPSCounter){
  if (i >= 1000) {
    i = 0;
  };
  degpsreadings[i++] = nxt_motor_get_count(RIGHT_MOTOR);
  
}
