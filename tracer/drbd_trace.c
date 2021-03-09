#include "tracer.h"

#define DRBD_LOGFILE_SIZE 0x40000000

void usage () {
	printf("Usage: drbd_tracer [-d] for daemon process\n [-l logfile_name]\n [-h] for help\n");
}

int main(int argc, char *argv[]) {
	int fd = -1, i = 0, ret = 0, dflag = 0, lflag = 0;
	char opt = '\0';
	struct user_data udata;
	char ts[16], logfile[1024] = {'\0'};
	int log_fd = -1;
	char print_buffer[1024] = {'\0'};
	struct stat	st;
	memset(ts, 0, sizeof(ts));

	while ((opt = getopt(argc, argv, "hdl")) != -1) {
		switch (opt) {
			case 'd':
				dflag = 1;
				break;
			case 'l':
				lflag = 1;
				if (!optarg) {
					printf("Provide the logfile name\n");
					usage();
					return -1;
				}
				snprintf(logfile, sizeof(logfile), optarg);
				break;
			case 'h':
				usage();
				return 0;
				break;
			default:
				break;
		}
	}
	if (dflag == 0)  {
		log_fd = 1;
	} else {
		if(fork() == 0) {
			close (0);
			close (1);
			close (2);
			if (lflag == 1) {
				log_fd = open(logfile, O_CREAT|O_RDWR|O_APPEND, 0666);
				if (log_fd < 0) {
					return -1;
				}
			} else {
				snprintf(logfile, sizeof(logfile), "/var/log/drbd_log.txt");
				log_fd = open(logfile, O_CREAT|O_RDWR|O_APPEND, 0666);
				if (log_fd < 0) {
					return -1;
				}
			}
		} else {
			exit(0);
		}
	}

retry:
	fd = open("/dev/drbd_tracer", O_RDONLY);
	if (fd < 0) {
		printf("open failed /dev/drbd_tracer\n");
		goto retry;
	}
retry1:
	udata.u_data = malloc(sizeof(struct trace_data) * 8192);
	if (!udata.u_data) {
		printf("udata.u_data allocation failed\n");
		goto retry1;
	}

	udata.u_size = 8192;
	udata.ret_val = 0;
	
	for (i = 0; i < udata.u_size; i++) {
		udata.p_data_tbl[i] = (struct p_data *) malloc (sizeof(struct p_data));
	}
	
	while(1) {
		if (dflag) {
			stat(logfile, &st);
			if (st.st_size >= DRBD_LOGFILE_SIZE) {
				close(log_fd);
				remove(logfile);
				log_fd = open(logfile, O_CREAT|O_RDWR|O_APPEND, 0666);
				if (log_fd < 0) {
					printf("open fail for %s\n", logfile);
					return -1;
				}
			}
		}
		if ((ioctl(fd, TRACE_DRBD_DATA, &udata) == 0)) {
			ret = udata.ret_val;
			for (i = 0; i < ret; i++) {
				strftime(ts, 16, "%b %d %T", localtime(&(udata.u_data[i].time_insec)));

				snprintf(print_buffer, sizeof(print_buffer),
				"%-15s jiffies=%llu msg_type=%d cmd=%d bi_size=%llu "
				"seq_no=%u dp_flag=%u sector_no=%llx block_no=%llx buf_addr=0x%llx\n",
				ts,udata.u_data[i].jiffies, udata.u_data[i].msg_type, udata.u_data[i].cmd,
				udata.u_data[i].bi_size, udata.p_data_tbl[i]->seq_num,
				udata.p_data_tbl[i]->dp_flags, udata.p_data_tbl[i]->sector, 
				udata.p_data_tbl[i]->block_id, udata.u_data[i].buf_ptr);

				write(log_fd, print_buffer, strlen(print_buffer));
			}
			udata.ret_val = 0;
			ret = 0;
		}
	}
	for (i = 0; i < udata.u_size; i++) {
		free(udata.p_data_tbl[i]);
	}
	free(udata.u_data);
	return 0;
}
