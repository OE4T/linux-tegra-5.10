/*
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File in accordance with the terms and conditions of the General
 * Public License Version 2, June 1991 (the "GPL License"), a copy of which is
 * available along with the File in the license.txt file or by writing to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
 * on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
 * DISCLAIMED. The GPL License provides additional details about this warranty
 * disclaimer.
 *
 */
#ifndef H_OAK_ETHTOOL
#define H_OAK_ETHTOOL

#include "oak_unimac.h" /* Include for relation to classifier oak_unimac */

/* Oak & Spruce max PCIe speed in Gbps */
#define OAK_MAX_SPEED    5
#define SPRUCE_MAX_SPEED 10

struct oak_s;
extern int debug;

/* Name      : get_stats
 * Returns   : void
 * Parameters:  oak_t * np,  uint64_t * data
 *  */
void oak_ethtool_get_stats(oak_t* np, uint64_t* data);

/* Name      : get_sscnt
 * Returns   : int
 * Parameters:  oak_t * np
 *  */
int oak_ethtool_get_sscnt(oak_t* np);

/* Name      : get_strings
 * Returns   : int
 * Parameters:  oak_t * np,  char * data
 *  */
int oak_ethtool_get_strings(oak_t* np, char* data);

/* Name      : cap_cur_speed
 * Returns   : int
 * Parameters:  oak_t * np,  int pspeed
 *  */
int oak_ethtool_cap_cur_speed(oak_t* np, int pspeed);

#endif /* #ifndef H_OAK_ETHTOOL */

