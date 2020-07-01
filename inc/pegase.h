#ifndef __pegase_H__
#define __pegase_H__

#include <app.h>
#include <storage.h>
#include <Elementary.h>
#include <system_settings.h>
#include <efl_extension.h>
#include <dlog.h>
#include <recorder.h>
#include <Ecore.h>
#include <unistd.h>
#include <privacy_privilege_manager.h>
#include <pthread.h>
#include <glib.h>
#include <wav_player.h>


#include "psound.h"
//#include "psap.h"
#include "pft.h"

#define FILENAME_PREFIX "AUDIO"

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "PGAS"

#if !defined(PACKAGE)
#define PACKAGE "com.jrk.pegase.watch"
#endif

Ecore_Timer *timer2;

#define RECORD_LIMIT 3.0
#define NUMBER_OF_PRIVILEGES 3
void    update_ui(char *data);


#endif /* __pegase_H__ */
