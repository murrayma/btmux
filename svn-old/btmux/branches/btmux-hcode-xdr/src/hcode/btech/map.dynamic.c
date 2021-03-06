
/*
 * $Id: map.dynamic.c,v 1.1.1.1 2005/01/11 21:18:08 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Sun Oct 13 19:38:31 1996 fingon
 * Last modified: Sun Jun 14 14:54:11 1998 fingon
 *
 */

#include <stdio.h>

#include "create.h"
#include "mech.h"
#include "autopilot.h"
#include "p.econ_cmds.h"
#include "p.mech.restrict.h"
#include "p.mech.utils.h"
#include "p.map.conditions.h"

#define DYNAMIC_MAGIC 42

/* Code for saving / loading / setting / unsetting the dynamic pieces
   of map structure:
   - mechsOnMap
   - LOSinfo
   - mechflags
   */

#define CHELO(a,b,c,d) if (!(ugly_kludge++)) { \
    if ((tmp=fread(a,b,c,d)) != c) { fprintf (stderr, "Error loading mapdynamic for #%d - couldn't find enough entries! (found: %d, should: %d)\n", map->mynum, tmp, c); return; } } else { if ((tmp=fread(a,b,c,d)) != c) { fprintf (stderr, "Error loading mapdynamic for #%d - couldn't find enough entries! (found: %d, should: %d)\n", map->mynum, tmp, c); fflush(stderr); exit(1); } }
#define CHESA(a,b,c,d) if ((tmp=fwrite(a,b,c,d)) != c) { fprintf (stderr, "Error writing mapdynamic for #%d - couldn't find enough entries! (found: %d, should: %d)\n", map->mynum, tmp, c); fflush(stderr); exit(1); }

static int ugly_kludge = 0;		/* Nonfatal for _first_ */

void load_mapdynamic(FILE * f, MAP * map)
{
#if 0
	int count = map->first_free;
	int i, tmp;
	unsigned char tmpb;

	if(count > 0) {
		Create(map->mechsOnMap, dbref, count);

		CHELO(map->mechsOnMap, sizeof(map->mechsOnMap[0]), count, f);
		Create(map->mechflags, char, count);

		CHELO(map->mechflags, sizeof(map->mechflags[0]), count, f);
		Create(map->LOSinfo, unsigned int *, count);

		for(i = 0; i < count; i++) {
			Create(map->LOSinfo[i], unsigned int, count);

			CHELO(map->LOSinfo[i], sizeof(map->LOSinfo[i][0]), count, f);
		}
	} else {
		map->mechsOnMap = NULL;
		map->mechflags = NULL;
		map->LOSinfo = NULL;
	}
	CHELO(&tmpb, 1, 1, f);
	if(tmpb != DYNAMIC_MAGIC) {
		fprintf(stderr, "Error reading data for obj #%d (%d != %d)!\n",
				map->mynum, tmpb, DYNAMIC_MAGIC);
		fflush(stderr);
		exit(1);
	}
#endif
}

#define outbyte(a) tmpb=(a);CHESA(&tmpb, 1, 1, f);

void save_mapdynamic(FILE * f, MAP * map)
{
#if 0
	int count = map->first_free;
	int i, tmp;
	unsigned char tmpb;

	if(count > 0) {
		CHESA(map->mechsOnMap, sizeof(map->mechsOnMap[0]), count, f);
		CHESA(map->mechflags, sizeof(map->mechflags[0]), count, f);
		for(i = 0; i < count; i++)
			CHESA(map->LOSinfo[i], sizeof(map->LOSinfo[i][0]), count, f);
	}
	outbyte(DYNAMIC_MAGIC);
#endif
}

void remove_mech_from_map(MAP * map, MECH * mech)
{
    int loop = map->first_free;
    MECH *target;

    clear_mech_from_LOS(mech);
    mech->mapindex = -1;

    /* Removing the unit from the map */
    dllist_remove_node_matching_data(map->mechs, (void *) mech->mynum);

    if(map->first_free <= mech->mapnumber ||
            map->mechsOnMap[mech->mapnumber] != mech->mynum) {
        SendError(tprintf
                ("Map indexing error for mech #%d: Map index %d contains data for #%d instead.",
                 mech->mynum, mech->mapnumber,
                 map->mechsOnMap ? map->mechsOnMap[mech->mapnumber] : -1));
        if(map->mechsOnMap)
            for(loop = 0;
                    (loop < map->first_free) &&
                    (map->mechsOnMap[loop] != mech->mynum); loop++);
    } else
                loop = mech->mapnumber;
    mech->mapnumber = 0;
    if(loop != (map->first_free)) {
        map->mechsOnMap[loop] = -1;	/* clear it */
        map->mechflags[loop] = 0;
#if 0
        for(i = 0; i < map->first_free; i++)
            if(map->mechsOnMap[i] > 0 && i != loop)
                if((t = getMech(map->mechsOnMap[i])))
                    if(MechTeam(t) != MechTeam(mech) &&
                            (map->LOSinfo[i][loop] & MECHLOSFLAG_SEEN)) {
                        MechNumSeen(t) = MAX(0, MechNumSeen(t) - 1);
                    }
#endif
        if(loop == (map->first_free - 1))
            map->first_free--;	/* Who cares about some lost memory? In realloc
                                   we'll gain it back anyway */
    }

    if (Towed(mech)) {

        /* Check that the towing guy isn't left on the map */

        if (!(target = getMech(MechTowedBy(mech)))) {

            SendDebug("Mech #%d being towed by #%d but #%d doesn't exist anymore",
                    mech->mynum, MechTowedBy(mech), MechTowedBy(mech));
            MechStatus(mech) &= ~TOWED;
            MechTowedBy(mech) = -1;

        } else if (MechCarrying(target) != mech->mynum) {

            SendDebug("Mech #%d being towed by #%d but it's towing something else (#%d)",
                    mech->mynum, target->mynum, MechCarrying(target));
            MechStatus(mech) &= ~TOWED;
            MechTowedBy(mech) = -1;

        } else {

            if (mech->mapindex != target->mapindex) {

                SendDebug("Mech #%d being towed by #%d but was moved to a new map",
                        mech->mynum, target->mynum);
                SetCarrying(target, -1);
                MechStatus(mech) &= ~TOWED;
                MechTowedBy(mech) = -1;
            }

        }

    } 

    MechNumSeen(mech) = 0;

    if (IsDS(mech))
        SendDSInfo("DS #%d has left map #%d", mech->mynum, map->mynum);

}

void add_mech_to_map(MAP * newmap, MECH * mech)
{
    int loop, count, i;
    dllist_node *node;
    MECH *tempMech;
    MECH *target;
    AUTO *autopilot;

    for(loop = 0; loop < newmap->first_free; loop++)
        if(newmap->mechsOnMap[loop] == mech->mynum)
            break;
    if(loop != newmap->first_free)
        return;
    for(loop = 0; loop < newmap->first_free; loop++)
        if(newmap->mechsOnMap[loop] < 0)
            break;
    if(loop == newmap->first_free) {
        newmap->first_free++;
        count = newmap->first_free;
        ReCreate(newmap->mechsOnMap, dbref, count);
        ReCreate(newmap->mechflags, char, count);
        ReCreate(newmap->LOSinfo, unsigned int *, count);

        newmap->LOSinfo[count - 1] = NULL;
        for(i = 0; i < count; i++) {
            ReCreate(newmap->LOSinfo[i], unsigned int, count);

            newmap->LOSinfo[i][loop] = 0;
        }
        for(i = 0; i < count; i++)
            newmap->LOSinfo[loop][i] = 0;
    }

    /* dllist for storing mechs on map */
    /* Make sure the unit isn't already on the map first */
    switch (dllist_index(newmap->mechs, (void *) mech->mynum)) {

        /* dllist not setup or mech->mynum bad */
        case -1:
            SendDebug("Map ERROR: Trying to add unit #%d to map #%d but "
                    "the map or mech is bad",
                    mech->mynum, newmap->mynum);
            break;

        case 0:
            node = dllist_create_node((void *) mech->mynum);
            dllist_insert_end(newmap->mechs, node);

            /* Add the unit to the LOS tree of all the units on the map */
            for (node = dllist_head(newmap->mechs); node; node = dllist_next(node)) {

                tempMech = getMech((int) dllist_data(node));

                if (tempMech) {

                    /* Just so we don't accidently add ourselves to our own LOS list */
                    if (tempMech != mech) {
                        rb_insert(tempMech->UnitsInLOS, (void *) mech->mynum, 0);
                    }

                }

            }
            break;
        
        /* Unit already on list */
        default:
            SendDebug("Map ERROR: Trying to add unit #%d to map #%d but "
                    "the mech is already on the map (wasn't removed then "
                    "re-added properly)",
                    mech->mynum, newmap->mynum);
            break;
    }

    mech->mapindex = newmap->mynum;
    mech->mapnumber = loop;

    newmap->mechsOnMap[loop] = mech->mynum;
    newmap->mechflags[loop] = 0;

    /* Is there an autopilot */
    if (MechAuto(mech) > 0) {

        autopilot = getAutopilot(MechAuto(mech));

        /* Reset the AI's comtitle */
        if (autopilot)
            auto_set_comtitle(autopilot, mech);
    }

    if (Towed(mech)) {

        if (!(target = getMech(MechTowedBy(mech)))) {

            SendDebug("Mech #%d being towed by #%d but #%d doesn't exist anymore",
                    mech->mynum, MechTowedBy(mech), MechTowedBy(mech));
            MechStatus(mech) &= ~TOWED;
            MechTowedBy(mech) = -1;

        } else if (MechCarrying(target) != mech->mynum) {

            SendDebug("Mech #%d being towed by #%d but it's towing something else (#%d)",
                    mech->mynum, target->mynum, MechCarrying(target));
            MechStatus(mech) &= ~TOWED;
            MechTowedBy(mech) = -1;

        } else {

            if (mech->mapindex != target->mapindex) {
                SendDebug("Mech #%d being towed by #%d but was moved to a new map",
                        mech->mynum, target->mynum);
                SetCarrying(target, -1);
                MechStatus(mech) &= ~TOWED;
                MechTowedBy(mech) = -1;
            }

        }

    }

    MarkForLOSUpdate(mech);
    UnZombifyMech(mech);
    UpdateConditions(mech, newmap);

    if (IsDS(mech))
        SendDSInfo("DS #%d has entered map #%d", mech->mynum,
                newmap->mynum);
}

int mech_size(MAP * map)
{
    return (dllist_size(map->mechs) * (sizeof(dllist_node)) + sizeof(dllist));
}
