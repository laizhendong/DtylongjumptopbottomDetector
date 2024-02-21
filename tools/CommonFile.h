/******************************************************************************
 Copyright (C), 2013-2014, HotVision Tech. Co., Ltd.
******************************************************************************
File Name     : CommonFile.h
Version       : Initial Draft
Author        : cwluo
Created       : 2013/04/12
Last Modified :
Description   : The common include file defination
Function List :
History       :
******************************************************************************/
#ifndef __COMMONFILE_H__
#define __COMMONFILE_H__

#include <minmax.h>

#ifdef _WIN32
#include <WinSock2.h>
#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#include <string.h>
#include <cstringt.h>
#include <iphlpapi.h>
#include <time.h>
#include <winsvc.h>
#include <memory.h>
#include <string>
#include <cstring>
#include <memory>  
#include <fstream>  
#include <iostream>  
#include <tuple>  
#include <sstream>
#include <io.h>
#include <direct.h>
#include <Mmsystem.h>
#include <GdiPlus.h>
#include <atlimage.h>
#include <algorithm>
#else
#include <sys/types.h> 
#include <sys/times.h> 
#include <sys/select.h> 
#include <sys/socket.h>
#include <sys/ipc.h>  
#include <sys/msg.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <dirent.h>
//#include <net/if.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/vfs.h>
#include <syslog.h>
#include <signal.h> 
#include <cstring>
#include <memory>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include "netinet/tcp.h"
#include <errno.h>	
#include <linux/rtc.h>
#include <linux/serial.h>
#include <linux/sockios.h>  
#include <linux/ethtool.h> 
#include <linux/route.h>
#include <linux/netlink.h> 
#include <linux/rtnetlink.h> 
#include <linux/fb.h>
#include <sys/socket.h>
#include <netdb.h>
#include <asm/ioctls.h>
#include <malloc.h> 
#include <cinttypes>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <set>
#include <cstdint>
#include <vector>
#include <atomic>
#include <climits>

/* SvrFund*/
//#include "svrfund/cfg-file.h"
//#include "svrfund/logger.h"
//#include "svrfund/tcp-service.h"
//#include "svrfund/udp-service.h"
//#include "svrfund/dyn-proto.h"
//#include "svrfund/init-winsock.h"
//#include "svrfund/http-filter.h"
//#include "svrfund/scope-exit.h"
//#include "svrfund/uri.h"
//
//#include "errCode.h"
//#include "protocol.h"


#ifndef STDERR_FILENO
#define STDERR_FILENO	2
#endif

#define ENDLINE				"\r\n"
#ifdef _WIN32
#define PATH_DELIMITER		'\\'
#define PATH_DELIMITER_S	"\\"
#else
#define PATH_DELIMITER		'/'
#define PATH_DELIMITER_S	"/"
#endif

#ifndef _WIN32
#define sscanf_s sscanf
#define sprintf_s sprintf
#endif

//#ifdef _WIN32
//#ifndef PRId64
//#define PRId64	"I64d"
//#endif
//#endif
#endif /* __COMMONFILE_H__ */