
#ifndef __MA_OPERATOR_H__
#define __MA_OPERATOR_H__

extern int ma_init();
extern int ma_add_detect_fd(fd_set *rfds);
extern int ma_data_dispose(fd_set *pst_rfds);
extern int ma_timeout();

#endif



