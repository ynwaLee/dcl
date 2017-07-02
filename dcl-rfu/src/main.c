
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "../../common/applog.h"
#include "../../common/daemon.h"
#include "config.h"
#include "variable.h"
#include "das_operator.h"
#include "svm_operator.h"
#include "wireless_svm_operator.h"
#include "ma_operator.h"

int gi_sn = 0;
char gac_config_path[500] = {0};

int main_body()
{
    fd_set rfds;         
    int i_ret = -1;
    struct timeval timeout = {1,0};
    unsigned int ui_tv = 0;

    while( 1 )
    {
        FD_ZERO (&rfds);
        das_add_detect_fd (&rfds);
        ma_add_detect_fd (&rfds);

        i_ret = select(1024, &rfds, NULL, NULL, &timeout);
        
        if( i_ret<0 )
        {
            applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "call select function failed, %s", strerror(errno));
            continue;
        }
        else if( i_ret==0 )
        {
            timeout.tv_sec = 1;    
            timeout.tv_usec = 0;
            ui_tv++;

            das_timeout (ui_tv);
            ma_timeout ();
            svm_timeout (ui_tv);
            wireless_timeout (ui_tv);
        }
        else
        {
            das_info_dispose (&rfds);
            ma_data_dispose (&rfds);
        }
    }

    return 0;
}

void exit_proccess(int sig)
{
    applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "recv SIGINT signal, exit rfu!");
    exit(0);
}

int rfu_init()
{
    signal(SIGINT, exit_proccess);    
    
    svm_container_init ();
    wireless_container_init ();
    operator_struct_init ();

    //不能把das_init放到前面去，先初始化上面三个
    ma_init ();    
    das_init ();

    return 0;
}

int dispose_cmd_argv(int argc, char **argv)
{
    int i;    

    for( i=0; i<argc; ++i )
    {
        if( strcmp(argv[i], "--sn")==0 )
        {
            if( (i+1)<argc )
            {
                ++i;
                gi_sn = atoi(argv[i]);     
            }
            else
            {
                applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "in the command line have --sn option, but not found value!");    
            }
        }
        
        if( strcmp(argv[i], "--config")==0 )
        {
            if( (i+1)<argc )
            {
                ++i;
                if( strlen(argv[i])<=200 )
                {
                    strcpy(gac_config_path, argv[i]);
                }
                else
                {
                    applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "in the command line have --config option, but value too long(%s)!", argv[i]);    
                }
            }
            else
            {
                applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "command line have config opetion but not fount value!");    
            }
        }
    }
    
    if( gi_sn==0 )
    {
        gst_conf.if_link_ma_flg = 0;
    }
    else
    {
        gst_conf.if_link_ma_flg = 1;
    }
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "whether to link ma: %s", gst_conf.if_link_ma_flg?"yes":"no");

    if( strlen(gac_config_path)==0 )
    {
        strcpy(gac_config_path, CONFIG_DIR);
    }
    else
    {
        if( check_directory(gac_config_path) )     
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "command line --config value error(%s), use default value: %s", gac_config_path, CONFIG_DIR);    
            strcpy(gac_config_path, CONFIG_DIR);
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    if((argc == 2) && (strcmp(argv[1], "-v") == 0))
    {
        printf("version: %s\n", VERSION);
        exit(EXIT_SUCCESS);
    }
    
	struct rlimit rlim;
    rlim.rlim_cur = RLIM_INFINITY;
    rlim.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rlim);          
    /* Must initialize libcurl before any threads are started */
    curl_global_init (CURL_GLOBAL_ALL);

    daemonize ();
    dispose_cmd_argv (argc, argv);
    open_applog("rfu", LOG_NDELAY, DEF_APP_SYSLOG_FACILITY);
    read_config ();    
    rfu_init ();
    main_body ();    
    close_applog();
    
    curl_global_cleanup ();    /*add by edgeyang*/

    return 0;
}


