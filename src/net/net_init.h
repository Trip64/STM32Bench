/* Network initialization and main loop polling */

#ifndef NET_INIT_H
#define NET_INIT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool Net_Init(void);
void Net_Poll(void);
bool Net_IsConnected(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_INIT_H */
