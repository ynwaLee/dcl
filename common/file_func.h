

void clear_file_set();
void check_file_add_fd(int fd);
int check_files_readable();
int file_readable(int fd);


int check_file_readable(int fd);
int sock_send_buf(int fd, unsigned char *buf, int size);
int sock_recv_buf(int fd, unsigned char *buf, int size);

