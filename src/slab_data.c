/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file slab_data.c
 *     Map Slabs support functions.
 * @par Purpose:
 *     Definitions and functions to maintain map slabs.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     25 Apr 2009 - 12 May 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "slab_data.h"
#include "globals.h"

#include "bflib_memory.h"
#include "player_instances.h"
#include "config_terrain.h"
#include "map_blocks.h"
#include "ariadne.h"
#include "frontmenu_ingame_map.h"
#include "game_legacy.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
const short around_slab[] = {-86, -85, -84,  -1,   0,   1,  84,  85,  86};
const short small_around_slab[] = {-85,   1,  85,  -1};
struct SlabMap bad_slabmap_block;
/******************************************************************************/
DLLIMPORT long _DK_calculate_effeciency_score_for_room_slab(long a1, long plyr_idx);
DLLIMPORT void _DK_update_blocks_in_area(long sx, long sy, long ex, long ey);
DLLIMPORT void _DK_do_slab_efficiency_alteration(unsigned char a1, unsigned char a2);
/******************************************************************************/
/**
 * Returns slab number, which stores both X and Y coords in one number.
 */
SlabCodedCoords get_slab_number(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
  if (slb_x > map_tiles_x) slb_x = map_tiles_x;
  if (slb_y > map_tiles_y) slb_y = map_tiles_y;
  if (slb_x < 0)  slb_x = 0;
  if (slb_y < 0) slb_y = 0;
  return slb_y*(map_tiles_x) + (SlabCodedCoords)slb_x;
}

/**
 * Decodes X coordinate from slab number.
 */
MapSlabCoord slb_num_decode_x(SlabCodedCoords slb_num)
{
  return slb_num % (map_tiles_x);
}

/**
 * Decodes Y coordinate from slab number.
 */
MapSlabCoord slb_num_decode_y(SlabCodedCoords slb_num)
{
  return (slb_num/(map_tiles_x))%map_tiles_y;
}

/**
 * Returns SlabMap struct for given slab number.
 */
struct SlabMap *get_slabmap_direct(SlabCodedCoords slab_num)
{
  if ((slab_num < 0) || (slab_num >= map_tiles_x*map_tiles_y))
      return INVALID_SLABMAP_BLOCK;
  return &game.slabmap[slab_num];
}

/**
 * Returns SlabMap struct for given (X,Y) slab coords.
 */
struct SlabMap *get_slabmap_block(MapSlabCoord slab_x, MapSlabCoord slab_y)
{
  if ((slab_x < 0) || (slab_x >= map_tiles_x))
      return INVALID_SLABMAP_BLOCK;
  if ((slab_y < 0) || (slab_y >= map_tiles_y))
      return INVALID_SLABMAP_BLOCK;
  return &game.slabmap[slab_y*(map_tiles_x) + slab_x];
}

/**
 * Gives SlabMap struct for given (X,Y) subtile coords.
 * @param stl_x
 * @param stl_y
 */
struct SlabMap *get_slabmap_for_subtile(MapSubtlCoord stl_x, MapSubtlCoord stl_y)
{
    if ((stl_x < 0) || (stl_x >= map_subtiles_x))
        return INVALID_SLABMAP_BLOCK;
    if ((stl_y < 0) || (stl_y >= map_subtiles_y))
        return INVALID_SLABMAP_BLOCK;
    return &game.slabmap[subtile_slab_fast(stl_y)*(map_tiles_x) + subtile_slab_fast(stl_x)];
}

/**
 * Gives SlabMap struct for slab on which given thing is placed.
 * @param thing The thing which coordinates are used to retrieve SlabMap.
 */
struct SlabMap *get_slabmap_thing_is_on(const struct Thing *thing)
{
    if (thing_is_invalid(thing))
        return INVALID_SLABMAP_BLOCK;
    return get_slabmap_for_subtile(thing->mappos.x.stl.num, thing->mappos.y.stl.num);
}

/**
 * Returns if given SlabMap is not a part of the map.
 */
TbBool slabmap_block_invalid(const struct SlabMap *slb)
{
    if (slb == NULL)
        return true;
    if (slb == INVALID_SLABMAP_BLOCK)
        return true;
    return (slb < &game.slabmap[0]);
}

/**
 * Returns owner index of given SlabMap.
 */
long slabmap_owner(const struct SlabMap *slb)
{
    if (slabmap_block_invalid(slb))
        return 5;
    return slb->field_5 & 0x07;
}

/**
 * Sets owner of given SlabMap.
 */
void slabmap_set_owner(struct SlabMap *slb, PlayerNumber owner)
{
    if (slabmap_block_invalid(slb))
        return;
    slb->field_5 ^= (slb->field_5 ^ owner) & 0x07;
}

/**
 * Sets owner of a slab on given position.
 */
void set_whole_slab_owner(MapSlabCoord slb_x, MapSlabCoord slb_y, PlayerNumber owner)
{
    struct SlabMap *slb;
    long stl_x,stl_y;
    long i,k;
    stl_x = 3 * slb_x;
    stl_y = 3 * slb_y;
    for (i = 0; i < 3; i++)
    {
        for (k = 0; k < 3; k++)
        {
            slb = get_slabmap_for_subtile(stl_x + k, stl_y + i);
            slabmap_set_owner(slb, owner);
        }
    }
}

/**
 * Returns Water-Lava under Bridge flags for given SlabMap.
 */
unsigned long slabmap_wlb(struct SlabMap *slb)
{
    if (slabmap_block_invalid(slb))
        return 0;
    return (slb->field_5 >> 3) & 0x03;
}

/**
 * Sets Water-Lava under Bridge flags for given SlabMap.
 */
void slabmap_set_wlb(struct SlabMap *slb, unsigned long wlbflag)
{
    if (slabmap_block_invalid(slb))
        return;
    slb->field_5 ^= (slb->field_5 ^ (wlbflag << 3)) & 0x18;
}

/**
 * Returns slab number of the next tile in a room, after the given one.
 */
long get_next_slab_number_in_room(SlabCodedCoords slab_num)
{
    if ((slab_num < 0) || (slab_num >= map_tiles_x*map_tiles_y))
        return 0;
    return game.slabmap[slab_num].next_in_room;
}

TbBool slab_is_safe_land(PlayerNumber plyr_idx, MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    struct SlabMap *slb;
    struct SlabAttr *slbattr;
    int slb_owner;
    slb = get_slabmap_block(slb_x, slb_y);
    slbattr = get_slab_attrs(slb);
    slb_owner = slabmap_owner(slb);
    if ((slb_owner == plyr_idx) || (slb_owner == game.neutral_player_num))
    {
        return slbattr->is_safe_land;
    }
    return false;
}

TbBool slab_is_door(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    struct SlabMap *slb;
    slb = get_slabmap_block(slb_x, slb_y);
    return slab_kind_is_door(slb->kind);
}

TbBool slab_is_liquid(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    struct SlabMap *slb;
    slb = get_slabmap_block(slb_x, slb_y);
    return slab_kind_is_liquid(slb->kind);
}

TbBool slab_kind_is_animated(SlabKind slbkind)
{
    if (slab_kind_is_door(slbkind))
        return true;
    return false;
}

/**
 * Clears all SlabMap structures in the map.
 */
void clear_slabs(void)
{
    struct SlabMap *slb;
    unsigned long x,y;
    for (y=0; y < map_tiles_y; y++)
    {
        for (x=0; x < map_tiles_x; x++)
        {
          slb = &game.slabmap[y*map_tiles_x + x];
          LbMemorySet(slb, 0, sizeof(struct SlabMap));
          slb->kind = SlbT_ROCK;
        }
    }
}

long calculate_effeciency_score_for_room_slab(SlabCodedCoords slab_num, PlayerNumber plyr_idx)
{
    return _DK_calculate_effeciency_score_for_room_slab(slab_num, plyr_idx);
}

/**
 * Reveals the whole map for specific player.
 */
void reveal_whole_map(struct PlayerInfo *player)
{
    clear_dig_for_map_rect(player->id_number,0,map_tiles_x,0,map_tiles_y);
    reveal_map_rect(player->id_number,1,map_subtiles_x,1,map_subtiles_y);
    pannel_map_update(0, 0, map_subtiles_x+1, map_subtiles_y+1);
}

void update_blocks_in_area(MapSubtlCoord sx, MapSubtlCoord sy, MapSubtlCoord ex, MapSubtlCoord ey)
{
    //_DK_update_blocks_in_area(sx, sy, ex, ey); return;
    update_navigation_triangulation(sx, sy, ex, ey);
    ceiling_partially_recompute_heights(sx, sy, ex, ey);
    light_signal_update_in_area(sx, sy, ex, ey);
}

void update_blocks_around_slab(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    MapSubtlCoord stl_x,stl_y;
    MapSubtlCoord sx,sy,ex,ey;
    stl_x = 3 * slb_x;
    stl_y = 3 * slb_y;

    ey = stl_y + 5;
    if (ey > map_subtiles_y)
        ey = map_subtiles_y;
    ex = stl_x + 5;
    if (ex > map_subtiles_x)
        ex = map_subtiles_x;
    sy = stl_y - 3;
    if (sy <= 0)
        sy = 0;
    sx = stl_x - 3;
    if (sx <= 0)
        sx = 0;
    update_blocks_in_area(sx, sy, ex, ey);
}

void do_slab_efficiency_alteration(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    _DK_do_slab_efficiency_alteration(slb_x, slb_y); return;
}
/******************************************************************************/
#ifdef __cplusplus
}
#endif
