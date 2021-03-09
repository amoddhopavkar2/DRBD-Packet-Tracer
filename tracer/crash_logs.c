#include <defs.h>

void log_init(void);
void log_fini(void);

void cmd_log(void);
char *help_log[];

static struct command_table_entry command_table[] = {
	{"drbd_log",cmd_log,help_log,0},
	{NULL},
};

void __attribute__((constructor)) log_init(void){
	register_extension(command_table);
}

void __attribute__((destructor)) log_fini(void){

}

void cmd_log(void){
	
}

char *help_log[] = {
	"drbd_log",
	"prints packets to file",
	" ",

	" ",
	NULL
};
