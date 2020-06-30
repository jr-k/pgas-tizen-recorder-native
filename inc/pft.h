#ifndef __pft_H__
#define __pft_H__

#include "pegase.h"
#include <glib.h>
#include <sap.h>
#include <sap_file_transfer.h>

#define FT_PROFILE_ID "/sample/filetransfer"

gboolean ft_find_peers(void);
void     ft_send_file(void);
void ft_initialize_sap(void);


#endif /* __pft_H__ */
