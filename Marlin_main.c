/*
* Marlin ported to work on the Intel Edison
*
* Sean Smith 2015
*/

#include "Marlin.h"
#include "Configuration.h"
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>

//===========================================================================
//=============================public variables=============================
//===========================================================================
#ifdef SDSUPPORT
CardReader card;
#endif
float homing_feedrate[] = HOMING_FEEDRATE;
bool axis_relative_modes[] = AXIS_RELATIVE_MODES;
int feedmultiply=100; //100->1 200->2
int saved_feedmultiply;
int extrudemultiply=100; //100->1 200->2
float current_position[NUM_AXIS] = { 0.0, 0.0, 0.0, 0.0 };
float add_homeing[3]={0,0,0};
#ifdef DELTA
float endstop_adj[3]={0,0,0};
#endif
float min_pos[3] = { X_MIN_POS_DEFAULT, Y_MIN_POS_DEFAULT, Z_MIN_POS_DEFAULT };
float max_pos[3] = { X_MAX_POS_DEFAULT, Y_MAX_POS_DEFAULT, Z_MAX_POS_DEFAULT };
bool axis_known_position[3] = {false, false, false};

// Extruder offset
#if EXTRUDERS > 1
#ifndef DUAL_X_CARRIAGE
  #define NUM_EXTRUDER_OFFSETS 2 // only in XY plane
#else
  #define NUM_EXTRUDER_OFFSETS 3 // supports offsets in XYZ plane
#endif
float extruder_offset[NUM_EXTRUDER_OFFSETS][EXTRUDERS] = {
#if defined(EXTRUDER_OFFSET_X) && defined(EXTRUDER_OFFSET_Y)
  EXTRUDER_OFFSET_X, EXTRUDER_OFFSET_Y
#endif
};
#endif
uint8_t active_extruder = 0;
int fanSpeed=0;
#ifdef SERVO_ENDSTOPS
  int servo_endstops[] = SERVO_ENDSTOPS;
  int servo_endstop_angles[] = SERVO_ENDSTOP_ANGLES;
#endif
#ifdef BARICUDA
int ValvePressure=0;
int EtoPPressure=0;
#endif

#ifdef FWRETRACT
  bool autoretract_enabled=true;
  bool retracted=false;
  float retract_length=3, retract_feedrate=17*60, retract_zlift=0.8;
  float retract_recover_length=0, retract_recover_feedrate=8*60;
#endif

#ifdef ULTIPANEL
  #ifdef PS_DEFAULT_OFF
    bool powersupply = false;
  #else
	  bool powersupply = true;
  #endif
#endif

#ifdef DELTA
float delta[3] = {0.0, 0.0, 0.0};
#endif

  
//===========================================================================
//=============================private variables=============================
//===========================================================================
const char axis_codes[NUM_AXIS] = {'X', 'Y', 'Z', 'E'};
static float destination[NUM_AXIS] = {  0.0, 0.0, 0.0, 0.0};
static float offset[3] = {0.0, 0.0, 0.0};
static bool home_all_axis = true;
static float feedrate = 1500.0, next_feedrate, saved_feedrate;
static long gcode_N, gcode_LastN, Stopped_gcode_LastN = 0;

static bool relative_mode = false;  //Determines Absolute or Relative Coordinates

static char cmdbuffer[BUFSIZE][MAX_CMD_SIZE];
static bool fromsd[BUFSIZE];
static int bufindr = 0;
static int bufindw = 0;
static int buflen = 0;
//static int i = 0;
static char serial_char;
static int serial_count = 0;
static bool comment_mode = false;
static char *strchr_pointer; // just a pointer to find chars in the cmd string like X, Y, Z, E, etc

const int sensitive_pins[] = SENSITIVE_PINS; // Sensitive pin list for M42

//static float tt = 0;
//static float bt = 0;

//Inactivity shutdown variables
static unsigned long previous_millis_cmd = 0;
static unsigned long max_inactive_time = 0;
static unsigned long stepper_inactive_time = DEFAULT_STEPPER_DEACTIVE_TIME*1000l;

unsigned long starttime=0;
unsigned long stoptime=0;

static uint8_t tmp_extruder;


#if NUM_SERVOS > 0
  Servo servos[NUM_SERVOS];
#endif

bool CooldownNoWait = true;
bool target_direction;
bool Stopped = false;

/*typedef struct line {
	char *gcode;
	char *params;
	line *next;
} line;*/

char *file_buf = NULL;
int file_size;
int current_read = 0;

//===========================================================================
//=============================ROUTINES=============================
//===========================================================================
float code_value()
{
  return (strtod(&cmdbuffer[strchr_pointer - cmdbuffer + 1], NULL));
}

bool code_seen(char code)
{
  strchr_pointer = strchr(cmdbuffer, code);
  return (strchr_pointer != NULL);  //Return True if a character was found
}

void kill()
{
  TODO
}

#define XYZ_CONSTS_FROM_CONFIG(type, array, CONFIG) \
static const PROGMEM type array##_P[3] =        \
    { X_##CONFIG, Y_##CONFIG, Z_##CONFIG };     \
static inline type array(int axis)          \
    { return pgm_read_any(&array##_P[axis]); }

#define XYZ_DYN_FROM_CONFIG(type, array, CONFIG)	\
static inline type array(int axis)			\
    { type temp[3] = { X_##CONFIG, Y_##CONFIG, Z_##CONFIG };\
      return temp[axis];}

XYZ_DYN_FROM_CONFIG(float, base_home_pos,   HOME_POS);
XYZ_DYN_FROM_CONFIG(float, max_length, MAX_LENGTH);
XYZ_CONSTS_FROM_CONFIG(float, home_retract_mm, HOME_RETRACT_MM);
XYZ_CONSTS_FROM_CONFIG(signed char, home_dir,  HOME_DIR);

static void homeaxis(int axis) {
#define HOMEAXIS_DO(LETTER) \
  ((LETTER##_MIN_PIN > -1 && LETTER##_HOME_DIR==-1) || (LETTER##_MAX_PIN > -1 && LETTER##_HOME_DIR==1))

  if (axis==X_AXIS ? HOMEAXIS_DO(X) :
    axis==Y_AXIS ? HOMEAXIS_DO(Y) :
    axis==Z_AXIS ? HOMEAXIS_DO(Z) :
    0) {
    int axis_home_dir = home_dir(axis);

    current_position[axis] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);

    destination[axis] = 1.5 * max_length(axis) * axis_home_dir;
    feedrate = homing_feedrate[axis];
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();
    //TODO
}

/***********************************/
int setup(char *);
void loop();
//int parse(char *line, line *l);
void get_command();

int main(int argc, char *argv[]) {

  if (argc != 2) {
    printf("Wrong number of arguments provided... Provided %d instead \
            of 2\nmarlin /path/to/file\n", argc);
    exit(1);
  }

  int file = setup(argv[1]);

  loop(file);

  return 0;
}

int setup(char *path)
{
  int file;

  struct stat s;
  if (stat(path, &s) == -1) {
    printf("Error stating %s\n", path);
    exit(1);
  }

  file_size = s.st_size;

  if ((file = open(path, O_RDONLY))) {
    printf("Error opening %s\n", path);
    exit(1);
  }

  file_buf = (char *)malloc(file_size);
  if (read(file, file_buf, file_size) < file_size) {
    printf("Error reading file\n");
    exit(1);
  }

  return file;
}

/*
char *readline(char **line, int *len, int fd) {

	int count = 0;



	while (1) {
		// Check if you need to read more chars
		if (current_read >= buffer_size) {
			int read_len = read(fd, line_buf, MAX_LINE_BUF);
			if (read_len == -1) {
				printf("read error\n");
				exit(1);
			}
			buffer_size = read_len;
			current_read = 0;
		}

		if (line_buf[current_read] == '\n') {
			break;
		}

		count++;

		current_read++;
	}

	*line = malloc(count * sizeof(char));



	return ret;
}
*/

void loop(int fd) {


	// For line in file
/*
	char *line = NULL;
	int len = 0;
	ssize_t read;


	while ((read = readline(&line, &len, fd)) != -1) {
		printf("line of size %lu = %s\n", len, line);
		line *l = calloc(sizeof(line));
		parse(line, l);
		len = 0;
	}*/

  
  while (1) {
    if (get_command())
      process_commands();

    //check heater every n milliseconds
    //TODO
    manage_heater();
    manage_inactivity();
    checkHitEndstops();
  }
}

bool get_command()
{
  while (current_read < file_size) {
    serial_char = file_buf[current_read++];
    if(serial_char == '\n' ||
       serial_char == '\r' ||
       (serial_char == ':' && comment_mode == false) ||
       serial_count >= (MAX_CMD_SIZE - 1) )
    {
      if(!serial_count) { //if empty line
        comment_mode = false; //for new command
        return;
      }
      cmdbuffer[serial_count] = 0;  //terminate string
      if(!comment_mode){
        comment_mode = false; //for new command

        if(strchr(cmdbuffer, 'N') != NULL)
        {
          printf("line number support needed\n");
          exit(1);
        }
        if((strchr(cmdbuffer, '*') != NULL))
        {
          printf("checksum support needed\n");
          exit(1);
        }

        if((strchr(cmdbuffer, 'G') != NULL)){
          strchr_pointer = strchr(cmdbuffer, 'G');
          switch((int)((strtod(&cmdbuffer[strchr_pointer - cmdbuffer + 1], NULL)))){
          case 0:
          case 1:
          case 2:
          case 3:
            if(Stopped == false) { // If printer is stopped by an error the G[0-3] codes are ignored.
            }
            else {
              printf("MSG_ERR_STOPPED\n");
            }
            break;
          default:
            break;
          }
        }
        serial_count = 0; //clear buffer
      }
      else
      {
        if(serial_char == ';') comment_mode = true;
        if(!comment_mode) cmdbuffer[serial_count++] = serial_char;
      }
      serial_count = 0; //clear buffer
    }
    else
    {
      if(serial_char == ';') comment_mode = true;
      if(!comment_mode) cmdbuffer[serial_count++] = serial_char;
    }
  }

  return true;
}

#define HOMEAXIS(LETTER) homeaxis(LETTER##_AXIS)

void process_commands()
{
  unsigned long codenum; //throw away variable
  char *starpos = NULL;
#ifdef ENABLE_AUTO_BED_LEVELING
  float x_tmp, y_tmp, z_tmp, real_z;
#endif
  if(code_seen('G'))
  {
    switch((int)code_value())
    {
    case 0: // G0 -> G1
    case 1: // G1
      if(Stopped == false) {
        get_coordinates(); // For X Y Z E F
        prepare_move();
        //ClearToSend();
        return;
      }
      //break;
    }
/*
    case 28: //G28 Home all Axis one at a time
#ifdef ENABLE_AUTO_BED_LEVELING
      //Reset the plane ("erase" all leveling data)
      matrix_3x3_set_to_identity(&plan_bed_level_matrix);
#endif //ENABLE_AUTO_BED_LEVELING
      saved_feedrate = feedrate;
      saved_feedmultiply = feedmultiply;
      feedmultiply = 100;
      previous_millis_cmd = millis();

      enable_endstops(true);

      for(char i=0; i < NUM_AXIS; i++) {
        destination[i] = current_position[i];
      }
      feedrate = 0.0;

      home_all_axis = !((code_seen(axis_codes[0])) || (code_seen(axis_codes[1])) || (code_seen(axis_codes[2])));

      if((home_all_axis) || (code_seen(axis_codes[X_AXIS])))
        HOMEAXIS(X);
      TODO
*/
  }
}

void get_coordinates()
{
  bool seen[4]={false,false,false,false};
  for(char i=0; i < NUM_AXIS; i++) {
    if(code_seen(axis_codes[i]))
    {
      destination[i] = (float)code_value() + (axis_relative_modes[i] || relative_mode)*current_position[i];
      seen[i]=true;
    }
    else destination[i] = current_position[i]; //Are these else lines really needed?
  }
  if(code_seen('F')) {
    next_feedrate = code_value();
    if(next_feedrate > 0.0) feedrate = next_feedrate;
  }
}

void clamp_to_software_endstops(float target[3])
{
  if (min_software_endstops) {
    if (target[X_AXIS] < min_pos[X_AXIS]) target[X_AXIS] = min_pos[X_AXIS];
    if (target[Y_AXIS] < min_pos[Y_AXIS]) target[Y_AXIS] = min_pos[Y_AXIS];
    if (target[Z_AXIS] < min_pos[Z_AXIS]) target[Z_AXIS] = min_pos[Z_AXIS];
  }

  if (max_software_endstops) {
    if (target[X_AXIS] > max_pos[X_AXIS]) target[X_AXIS] = max_pos[X_AXIS];
    if (target[Y_AXIS] > max_pos[Y_AXIS]) target[Y_AXIS] = max_pos[Y_AXIS];
    if (target[Z_AXIS] > max_pos[Z_AXIS]) target[Z_AXIS] = max_pos[Z_AXIS];
  }
}

void prepare_move()
{
  clamp_to_software_endstops(destination);

  //TODO
  previous_millis_cmd = millis();

  // Do not use feedmultiply for E or Z only moves
  if( (current_position[X_AXIS] == destination [X_AXIS]) && (current_position[Y_AXIS] == destination [Y_AXIS])) {
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
  }
  else {
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate*feedmultiply/60/100.0, active_extruder);
  }

  for(char i=0; i < NUM_AXIS; i++) {
    current_position[i] = destination[i];
  }
}


/* vi: set et sw=2 sts=2: */
