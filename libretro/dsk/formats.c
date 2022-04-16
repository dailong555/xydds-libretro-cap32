/****************************************************************************
 *  Caprice32 libretro port
 *
 *  code adapted from libcpccat, pituka, wiituka ported to retroarch by
 *   David Colmenero - D_Skywalk (2019-2021)
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "formats.h"
#include "catalog.h"

#define EXTENTSIZE	16384	/* 16 kByte, constant for CP/M */
#define SECTOR_NOT_FOUND 0xFF
//#define FORMAT_DEBUG

DPB_list formats_list;
int blknr_size    = 1;
int blknr         = 16;

DPB_type dsk_type_data = {
   0x0c1, /* SEC1 side 1 */
   0x0c1, /* SEC1 side 2 */
   9,  /* SECS */
   40,  /* TRKS */
   1,  /* HDS */
   512,  /* BPS */

   36,  /* SPT */
   3,  /* BSH */
   7,  /* BLM */
   0,  /* EXM */
   179,  /* DSM */
   63,  /* DRM */
   0xC0,  /* AL0 */
   0x00,  /* AL1 */
   16,  /* CKS */
   0,  /* OFS */
   0,0,0,
   ORDER_SIDES,0,0,5,0,NULL
};

DPB_type dsk_type_data_big = {
   0x0c1, /* SEC1 side 1 */
   0x0c1, /* SEC1 side 2 */
   10,  /* SECS */
   40,  /* TRKS */
   1,  /* HDS */
   512,  /* BPS */

   36,  /* SPT */
   3,  /* BSH */
   7,  /* BLM */
   0,  /* EXM */
   179,  /* DSM */
   63,  /* DRM */
   0xC0,  /* AL0 */
   0x00,  /* AL1 */
   16,  /* CKS */
   0,  /* OFS */
   0,0,0,
   ORDER_SIDES,0,0,5,0,NULL
};

DPB_type dsk_type_system = {
   0x041, /* SEC1 side 1 */
   0x041, /* SEC1 side 2 */
   9,  /* SECS */
   40,  /* TRKS */
   1,  /* HDS */
   512,  /* BPS */

   36,  /* SPT */
   3,  /* BSH */
   7,  /* BLM */
   0,  /* EXM */
   170,  /* DSM */
   63,  /* DRM */
   0xC0,  /* AL0 */
   0x00,  /* AL1 */
   16,  /* CKS */
   2,  /* OFS */
   0,0,0,
   ORDER_SIDES,0,0,5,0,NULL
};

DPB_type dsk_type_system_big = {
   0x041, /* SEC1 side 1 */
   0x041, /* SEC1 side 2 */
   10,  /* SECS */
   40,  /* TRKS */
   1,  /* HDS */
   512,  /* BPS */

   36,  /* SPT */
   3,  /* BSH */
   7,  /* BLM */
   0,  /* EXM */
   170,  /* DSM */
   63,  /* DRM */
   0xC0,  /* AL0 */
   0x00,  /* AL1 */
   16,  /* CKS */
   2,  /* OFS */
   0,0,0,
   ORDER_SIDES,0,0,5,0,NULL
};

// FIXME: remove
char * _upper(char *s)  {
  char  *p=s;
  while (*s)
  {
    *s = toupper(*s);
    s++;
  }
  return p;
}

void _list_add_entry(DPB_list *list, DPB_list_entry *entry)
{
   DPB_list_entry *cur_entry;

   /* if list empty add as first item */
   if (list->first==NULL)
   {
      list->first = entry;
   } else {
      /* add to end of list */
      cur_entry = list->first;

      while (cur_entry->next!=NULL) {
         cur_entry = cur_entry->next;
      }
      cur_entry->next = entry;
   }
}

void _list_add_item(DPB_list *list,char *description, char *ident, DPB_type *dpb)
{
   DPB_list_entry *entry = (DPB_list_entry *) malloc(sizeof(DPB_list_entry));
   if (entry!=NULL) {
      /* copy dpb */
      memcpy(&entry->dpb, dpb, sizeof(DPB_type));

      if (description == NULL) {
         entry->description = NULL;
      } else {
         entry->description = (char *) malloc(strlen(description) + 1);
         if (entry->description != NULL) {
            strcpy(entry->description, description);
         }
      }

      if (ident == NULL) {
         entry->ident = NULL;
      } else {
         entry->ident = (char *) malloc(strlen(ident) + 1);
         if (entry->ident != NULL) {
            strcpy(entry->ident, ident);
            _upper(entry->ident);
         }

         entry->dpb.label = (char *) malloc(strlen(ident) + 1);
         if (entry->dpb.label != NULL) {
            strcpy(entry->dpb.label, entry->ident);
         }
      }
      /* if cylinders is 40, but used in a 80 track drive, then the drive
      head is automatically double stepped */
      /* want to use cylinders from .def because this is good for discs
      where only 60 tracks are used */
      entry->next = NULL;
   }

   _list_add_entry(list, entry);
}

int _get_bit_count_from_mask(unsigned long mask)
{
  int bit_count = 0;

   while ((mask & 1) != 0) {
      bit_count++;
      mask = mask>>1;
   }

   return bit_count;
}

void formats_init() {
   formats_list.first = NULL;
   _list_add_item(&formats_list,"data", "data", &dsk_type_data);
   // fix big data format (ex: Prehistorik II)
   _list_add_item(&formats_list,"data_b", "data_b", &dsk_type_data_big);

   /* system */
   _list_add_item(&formats_list,"system", "system", &dsk_type_system);
   _list_add_item(&formats_list,"system_b", "system_b", &dsk_type_system_big);
}

void _list_clean_item(DPB_list_entry *entry)
{
   // printf("[FORMAT] item-cleaned: %s\n", entry->description);
   if (entry->description != NULL) {
      free(entry->description);
      entry->description = NULL;
   }

   if (entry->ident != NULL) {
      free(entry->ident);
      entry->ident = NULL;
   }

   if (entry->dpb.label != NULL) {
      free(entry->dpb.label);
      entry->dpb.label = NULL;
   }
}

void _list_clean_entry(DPB_list_entry *entry)
{
   if (entry == NULL)
      return;

   _list_clean_entry(entry->next);
   // printf("[FORMAT] entry-cleaned: %s\n", entry->description);
   _list_clean_item(entry);
   free(entry->next);
   entry->next = NULL;
}


void formats_clean() {
   DPB_list_entry *cur_entry = formats_list.first;
   // recursive call
   _list_clean_entry(cur_entry);
   free(cur_entry);
   formats_list.first = cur_entry = NULL;
}

int sector_find(t_track *track, unsigned short sector_signature)
{
   for (int i = 0; i < track->sectors; i++) {
      #ifdef FORMAT_DEBUG
      printf("[LOADER] [FMT]: %i !!%x == %x [%02x.%02x.%02x]\n", i,
         track->sector[i].CHRN.sector,
         sector_signature,
         track->sector[i].CHRN.side,
         track->sector[i].CHRN.cylinder,
         track->sector[i].CHRN.bps
      );
      #endif
      if (track->sector[i].CHRN.sector == sector_signature) {
         return i;
      }
   }

   return SECTOR_NOT_FOUND;
}

void calc_dpb(DPB_type *dpb)
{
      // calc dpb->DSM, dpb->DRM, dpb->BLS
   int blknr_max;
   unsigned long max_size;

   dpb->BLS = 1 << (dpb->BSH + 7); /* or 2^BSH*128 */

   if (dpb->DSM >= 256) {
      /* 2 byte pointer and 8 pointer per entry max */
      blknr_size = 2;
      blknr_max = 8;
   } else {
      /* 1 byte pointer and 16 pointer per entry max */
      blknr_size = 1;
      blknr_max = 16;
   }

   /* if the extent mask is a real mask, then we can calculate the number of 1 bits in it.
      From this we can calculate the number of extents */
   /* the function will only work if EXM is 1,3,7,15.... i.e. if bit 3 is set, bits 2,1 and 0 must be set */
   /* it will not handle isolated bits! e.g. EXM=8*/
   /* get number of extents from exm bit mask */
   dpb->num_extents = 1 << _get_bit_count_from_mask(dpb->EXM);

   /* I hope this should correctly get the number of blocks per directory entry */
   /* a directory entry can only describe a maximum of 16384 bytes */

   /* calculate the max size based on the max number of blocks that
   can be stored in the directory entry. The max number of blocks
   is dependant on DSM. If DMS<=255, then 16 blocks can be stored,
   else 8 blocks can be stored */
   max_size = blknr_max * dpb->BLS;

   /* divide max size by EXM+1. If this is greater than 16384,
   calculate the number of blocks we can store */
   if ((max_size / dpb->num_extents) > EXTENTSIZE) {
     blknr = (EXTENTSIZE * dpb->num_extents) / dpb->BLS;
   } else {
     /* set to max allowed */
     blknr = blknr_max;
   }

   #ifdef FORMAT_DEBUG
   printf("[LOADER] [FMT]: upd: %02x.%02x.%u.%u.%lu.%02x\n",
      dpb->BLS,
      dpb->DSM,
      blknr_size,
      dpb->num_extents,
      max_size,
      blknr
   );
   #endif
}

DPB_type *format_find (t_drive *drive)
{
   int found_count = 0;
   DPB_list_entry *cur_entry;
   DPB_type *dpb_found = NULL;

   t_track *first_track = &drive->track[0][0];

   /* go through list of supported formats */
   for (cur_entry = formats_list.first; cur_entry!=NULL; cur_entry = cur_entry->next)
   {
      unsigned short catalogue_sector = sector_find(first_track, cur_entry->dpb.SEC1_side1);
      if ((catalogue_sector == SECTOR_NOT_FOUND)
         || (first_track->sectors != cur_entry->dpb.SECS)
         || (drive->sides != cur_entry->dpb.HDS - 1) // zero base number of sides
         || (drive->tracks < cur_entry->dpb.TRKS)
      ) {
         continue;
      }

      #ifdef FORMAT_DEBUG
      printf(">>> %02x.%02x.%02x.%u [CAT: %u]\n",
         cur_entry->dpb.SEC1_side1,
         cur_entry->dpb.SECS,
         cur_entry->dpb.HDS - 1, // heads
         cur_entry->dpb.TRKS,
         catalogue_sector
      );

      printf(" >>> %02x.%u [%02x.%02x.%02x.%02x][%02x.%02x.%02x.%02x].%u.%u.%u\n",
         first_track->sectors,
         first_track->size,
         first_track->sector[0].CHRN.data[0],
         first_track->sector[0].CHRN.data[1],
         first_track->sector[0].CHRN.data[2],
         first_track->sector[0].CHRN.data[3],
         first_track->sector[0].CHRN.cylinder,
         first_track->sector[0].CHRN.side,
         first_track->sector[0].CHRN.sector,
         first_track->sector[0].CHRN.bps,
         drive->extended,
         drive->sides,
         drive->tracks
      );
      printf("[LOADER] >>> format: \"%s\" matching disk.\n", dpb_found->label);
      #endif

      dpb_found = &cur_entry->dpb;

      // calc catalogue size
      dpb_found->DBL = catalogue_sector
         ? (catalogue_sector + 1) * CAT_ENTRIES
         : dpb_found->DRM;

      found_count++;
   }

   if (found_count != 1) {
      return NULL;
   }

   calc_dpb(dpb_found);

   return dpb_found;
}

#define HEXAGON_TRACK_SIZE    0x1400
#define HEXAGON_TRACK_SIZE_V2 0x1800
#define HEXAGON_SECTOR_SIZE   0xA
#define HEXAGON_SECTOR_POS    0x8
#define HEXAGON_FILE_POS      0x1
#define HEXAGON_FILE_STR      "DISK"
#define HEXAGON_HEADER_POS    0x1C
#define HEXAGON_HEADER_STR    "HEXAGON"

bool _hexagon_protection_check (t_track *first_track, int tracks, int sectors, int header_token, int file_token) {
   if (first_track->size != tracks)
      return false;

   if (first_track->sectors != sectors)
      return false;

   t_sector * hexagon_sector = &first_track->sector[HEXAGON_SECTOR_POS];

   if (memcmp(
      hexagon_sector->data + header_token,
      HEXAGON_HEADER_STR,
      7
   ) != 0)
      return false;

   if (memcmp(
      hexagon_sector->data + file_token,
      HEXAGON_FILE_STR,
      4
   ) != 0)
      return false;

   #ifdef FORMAT_DEBUG
   printf("[LOADER] >>> protection: HEXAGON/DATA_B matching disk t(%i).\n", tracks);
   #endif

   return true;
}

bool format_hexagon_protection (t_drive *drive) {
   t_track *first_track = &drive->track[0][0];

   bool result = _hexagon_protection_check(
      first_track,
      HEXAGON_TRACK_SIZE_V2,
      HEXAGON_SECTOR_SIZE,
      HEXAGON_HEADER_POS,
      HEXAGON_FILE_POS
   );

   if (result)
      return true;

   #ifdef FORMAT_DEBUG
   printf("%i = %i, %i = %i\n",
      first_track->sectors, HEXAGON_SECTOR_SIZE,
      first_track->size, HEXAGON_TRACK_SIZE
   );
   #endif

   result = _hexagon_protection_check(
      first_track,
      HEXAGON_TRACK_SIZE,
      HEXAGON_SECTOR_SIZE,
      HEXAGON_HEADER_POS,
      HEXAGON_FILE_POS
   );

   return result;
}
