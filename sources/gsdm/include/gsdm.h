/*
 *  Copyright (c) 2012,
 *  General server development mode (gsdm)
 *  it is easy to development and have high performance 
 *  sunlei (sswin0922@163.com)
 */

#ifndef _GSDM_COMMON_H
#define _GSDM_COMMON_H

#include <buffering/iobuffer.h>
#include <logging/logger.h>
#include <eventdrive/basecmd.h>
#include <eventdrive/baseprocess.h>
#include <eventdrive/baseworkerex.h>
#include <eventdrive/manager.h>
#include <eventdrive/httpclient.h>
#include <pooldrive/manager.h>
#include <pooldrive/poolworkerex.h>
#include <pooldrive/poolprocess.h>
#include <platform/linuxplatform.h>
#include <protocols/wsprotocol.h>
#include <protocols/httpprotocol.h>
#include <protocols/urlcode.h>
#include <utils/container.h>
#include <utils/crypto.h>
#include <utils/file.h>
#include <utils/gsdmconfig.h>
#include <utils/timersmanager.h>

namespace gsdm {

// current millisecond in global
#define gsdm_current_msec getCurrentGsdmMsec()
#define gsdm_current_msec_str getCurrentGsdmMsecStr()

}



#endif /* _GSDM_COMMON_H */
