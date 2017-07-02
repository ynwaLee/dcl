#ifndef __TYPE_MACRO__
#define __TYPE_MACRO__


#define PCKT_HEAD_PRO_VER            1
#define PH_FLG_REPLY_NOT        0
#define PH_FLG_REPLY            1

#define NOW_TIME                     1000
#define OPTION_RESULT                1001
#define OPTION_ERR_CODE                1002
#define OPTION_FAIL_WHY                1003




//cmd start 1
#define  CMD_QUERY                    1

#define QUERY_OPETION                1
//cmd end 1

//cmd start 24
#define CMD_QUERY_STATISTICAL        24
//cmd end 24

//cmd start 25
#define CMD_QUERY_BUS_STATIS        19
//cmd end 25

//cmd start 27
#define CMD_QUERY_CONF_CTIME        27

#define QUERY_CONF_SNAME        1
#define QUERY_CONF_INFOMATION    2
#define QUERY_CONF_CONF_NAME    1
#define QUERY_CONF_CTIME        2
//cmd end 27




//cmd start 2
#define  CMD_RESULT                     2

#define  HARDWARE_MESSAGE             1
//#define  DEVICE_NAME                  1
#define  CPU_NUMBER                      2
#define  CPU_MESSAGE                  3
#define  DISK_TOTAL_SIZE              4
#define  MEM_SIZE                     5
#define  SYSTEM_BIT                   6
#define  SYSTEM_VERSION               7
#define  NETWORK_CARD                 8
#define  NETWORK_CARD_NAME                  1
#define  NETWORK_CARD_Kbps                    2
#define  NETWORK_STATE                      3
#define  MAC_ADDR                           4
#define  IP_ADDR                            5
#define  FDISK_INFO                   9
#define  FDISK_NAME                         1
#define  FDISK_SIZE                         2

#define  SOFTWARE_MESSAGE              2
#define  SOFT_NAME                    1
#define  SOFT_VERSION                 2
#define  SOFT_VERSION_VAL             3
#define  CONFIG_VERSION_VAL           4
#define  CONFIG_INFOMATION              5
#define  CONFIG_NAME                  1
#define     CONFIG_CTIME                  2

#define  SOFT_MSG_PROC_MSG              6 
#define  SOFT_MSG_PNAME                  1
#define  SOFT_MSG_RUN_MODEL              5
//#define  SOFT_MSG_PID                  2


#define  SOFT_START_OK                3
#define  SOFT_STARG_MSG                 1
#define  SOFT_START_OK_NAME           1
#define  SOFT_START_OK_ID             2
    
#define  REPORT_DEV_NAME_ID             4
#define  REPORT_DEV_NAME            1
#define  REPORT_DEV_ID                 2    
//end cmd 2


//start cmd 19
#define  CMD_BUS_STATISTICAL                19
#define BUS_STATIS_MA                    4
#define BUS_STATIS_MA_BODY                2


#define BUS_STATIS_SGUARD                5


#define BUS_STATIS_SNAME                1
#define BUS_STATIS_PROC_NAME                1004
//end cmd 19

//start cmd 3
#define  CMD_DEV_STATISTICAL            3    

#define  PROCESS_MESSAGE              1
#define  PROC_MSG_SNAME               1
#define  PROC_MSG_PROC_NAME           2 
#define  PROC_MSG_VIRT                3
#define  PROC_MSG_RES                 4
#define  PROC_MSG_SHR                 5
#define  PROC_MSG_CPU                 6
#define  PROC_MSG_MEM                  7
#define  PROC_MSG_TIME                  8


#define  SOFT_CONF_INFO                  3
#define  BELONG_SOFT_VERSOIN          14
#define  BELONG_SOFT_VER_VAR          15
#define  BELONG_CONF_VER_VAR          16 
#define  PROCESS_CONFIG_MSG              17
#define  BELONG_CONF_NAME              1
#define  BELONG_CONF_TIME              2 



#define  DEV_COUNT_MESSAGE            2
#define  SYS_CPU_USED_RATE            1
#define  SYS_MEM_USED_RATE            2
#define  SYS_DISK_INFO                  3
#define  SYS_DISK_NAME                  1
#define  SYS_DISK_USED_RATE           2
//end cmd 3

/*
//start cmd 4
#define  CMD_WARNING                    4

#define  WARNING_MESSAGE              1
#define  CPU_USED_LIMIT               1
#define  MEM_USED_LIMIT               2
#define  DISK_USED_LIMIT              3

#define  SOFT_START_FAIL              2

#define  SOFT_END                     3
#define  SOFT_END_MSG                     1
#define  SOFT_END_NAME                1
#define  SOFT_END_ID                  2
//end cmd 4
*/

//start cmd 5
#define  CMD_KEEPLINE                    5

#define  START_SOFT_MSG                1
#define  KEEP_DEV_NAME                1
#define  KEEP_SOFT_MSG                2
#define  KEEP_SOFT_NAME                1
#define     KEEP_PROC_MSG                2
#define  KEEP_PROC_ID                1
#define  KEEP_PROC_NAME                2

#define  PROCESS_END                  2
#define  PROCESS_END_NAME             1
#define  PROCESS_END_ID               2
#define  PROC_END_SOFT_NAME            3

#define  MU_CUT                        3

#define  BUS_TO_SG_KEEP                4
#define  SOFT_START_KEEP            1
#define  THROW_WHY_KEEP                2

//end cmd 5

//start cmd 7
#define  CMD_SOFT_UPDATE              7
#define  SOFT_UPDATE_NAME             1
#define  SOFT_UPDATE_VERSION          2
#define  SOFT_UPDATE_VER_VAL          3
#define  SOFT_UPDATE_ADDR             4
//end cmd 7

//start cmd 6
#define  CMD_INSTALL_SOFT                6
#define  INSTALL_SOFT_NAME                1
#define  DOWNLOAD_URL                    2
//end cmd 6

//start cmd 9
#define  CMD_RUN_SOFT                 9

#define RUN_SOFT_NAME                 1
#define RUN_SOFT_NEED_REPLY             2
//end cmd 9

//start cmd 10
#define  CMD_STOP_SOFT                 10

#define STOP_SOFT_NAME                 1
#define STOP_SOFT_WAY                 2
#define STOP_SOFT_NEED_REPLY         3 
//end cmd 10

//start cmd 28
#define CMD_RESTART_SOFT            28

#define RESTART_SNAME                1
#define RESTART_SOFT_LFG_REPLY         2
//end cmd 28

//start cmd 8
#define  CMD_UPDATE_CONFIG          8
#define  UPDATE_CONFIG_SOFT_NAME      1 
#define  UPDATE_CONFIG_VERSION_VAL    2 
#define  UPDATE_CONFIG_INFOMATION      3
#define  UPDATE_CONFIG_NAME            1
#define  UPDATE_CONFIG_ADDR            2
//end cmd 8

//start cmd 11
#define  CMD_SOFT_UNINSTALL                11

#define UNINSTALL_SOFT_NAME                 1
//end cmd 11    

//start cmd 21
#define  CMD_ALTER_DEV_MSG                21
#define  NEW_DEV_FLG                    2
#define  NEW_DEV_NAME                    1
//start cmd 21

/*
//start cmd 15
#define  CMD_EXE_RESULT            15

#define  SOFT_UPDATE_RESPOND          1
#define  SOFT_UPDATE_RES_NAME          1
#define  SOFT_UPDATE_RES_VERSION      2
#define  SOFT_UPDATE_RES_VER_VAL      3
#define  SOFT_UPDATE_RES_OPTION         4
#define  SOFT_UPDATE_RES_FAIL_WHY     5    

#define  RUN_SOFT_RESPOND                 2
#define  RUN_SOFT_RES_OPTION             4
#define  RUN_SOFT_RES_FAIL_WHY         5    

#define  CMD_STOP_SOFT                3
#define  STOP_SOFT_RES_OPTION             1
#define  STOP_SOFT_RES_FAIL_WHY         2    
//end cmd 15
*/

//start cmd 13
#define CMD_SOFT_HB_CUT                13  //host and backup cut
#define CMD_SOFT_HB_SNAME            1
//end  end 13


//start cmd 14
#define  CMD_SOFT_STAT_INFORM        14
#define  SSTAT_INFORM_ARM_SNAME        1
#define  SSTAT_STAT_INFO            2
#define  SSTAT_VALUE                1
#define  SSTAT_DEV_NAME                2
#define  SSTAT_INFORM_SOURCE_SNAME    3
//start end 14

//start cmd 16
#define CMD_SOFT_REGISTER            16

#define BUS_TO_SG_REGISTER            1
#define BUS_TO_SG_PRCO_NAME            1
#define BUS_TO_SG_PRCO_ID            2
#define BUS_TO_SG_SOFT_SEQ            3
#define BUS_TO_SG_SOFT_SID            4    
#define BUS_TO_SG_SOFT_SIP            5    

#define BUS_TO_MA_REGISTER            2
#define BUS_TO_MA_PROC_ID            1
#define BUS_TO_MA_SOFT_SEQ            2

#define SG_TO_MA_REGISTER            3
#define SG_TO_MA_SOFT_NAME            1
#define SG_TO_MA_PROC_NAME            2
#define SG_TO_MA_PROC_ID            3
#define SG_TO_MA_SOFT_SEQ            4
//end cmd 16

//start cmd 22
#define CMD_PUT_CONF                22

#define PUT_CONF_SNAME                1
#define PUT_CONF_URL                2
#define PUT_CONF_DIR                3

#define PUT_CONF_REPLY_SNAME        1
#define PUT_CONF_REPLY_MES            2
#define PUT_CONF_REPLY_CNAME        1
#define PUT_CONF_REPLY_CTIME        2
//end cmd 22

//start cmd 23
#define CMD_ANEW_READ_CONF            23

#define CONF_FILE_PATH_TYPE            1
//end cmd 23

//start cmd 29
#define CMD_PUT_COMMON_CONF            29

#define PUT_COMMON_CONF_CNAME        1
#define PUT_COMMON_CONF_URL            2
#define PUT_COMMON_CONF_DIR            3
//end cmd 29

//start cmd 30
#define CMDL_DEBUG_ON                30 
//end cmd 30 

//start cmd 31
#define CMDL_DEBUG_OFF                31
//end cmd 31

//start cmd 32
#define CMDL_SET_LOG_LEV            32
//end cmd 32

//start cmd 33
#define CMDL_SHOW_LOG_LEV            33
//end cmd 33

//start cmd 34
#define CMDL_SET_LOG_MASK            34
//end cmd 34

//start cmd 35
#define CMDL_SHOW_LOG_MASK            35
//end cmd 35

//start cmd  36
#define CMDL_RFU_SHOW_DAS_RULE        36        
//end cmd  36

//start cmd 37
#define CMDL_RFU_SHOW_WIRELESS_RULE        37    
//end cmd 37

//start cmd 38
#define CMDL_RFU_SHOW_SVM_RULE        38
//end cmd 38

//start cmd 39
#define CMD_COUNT_INFO_STATUS_REG        39
//end cmd 39

#endif



