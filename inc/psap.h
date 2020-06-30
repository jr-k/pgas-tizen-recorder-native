#ifndef __psap_H__
#define __psap_H__

#include "pegase.h"
#include <glib.h>
#include <sap.h>
#include <sap_message_exchange.h>

#define MEX_PROFILE_ID "/sample/hellomessage"

void     initialize_sap();
gboolean find_peers();
void     mex_send(char *message, int length, gboolean is_secured);

#endif /* __psap_H__ */
