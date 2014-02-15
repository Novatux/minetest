/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DUNGEONGEN_HEADER
#define DUNGEONGEN_HEADER

#include "voxel.h"
#include "noise.h"

#define VMANIP_FLAG_DUNGEON_INSIDE VOXELFLAG_CHECKED1
#define VMANIP_FLAG_DUNGEON_PRESERVE VOXELFLAG_CHECKED2
#define VMANIP_FLAG_DUNGEON_UNTOUCHABLE (\
		VMANIP_FLAG_DUNGEON_INSIDE|VMANIP_FLAG_DUNGEON_PRESERVE)

class ManualMapVoxelManipulator;
class INodeDefManager;
class Mapgen;

v3POS rand_ortho_dir(PseudoRandom &random, bool diagonal_dirs);
v3POS turn_xz(v3POS olddir, int t);
v3POS random_turn(PseudoRandom &random, v3POS olddir);
int dir_to_facedir(v3POS d);


struct DungeonParams {
	content_t c_water;
	content_t c_cobble;
	content_t c_moss;
	content_t c_stair;

	int notifytype;
	bool diagonal_dirs;
	float mossratio;
	v3POS holesize;
	v3POS roomsize;

	NoiseParams np_rarity;
	NoiseParams np_wetness;
	NoiseParams np_density;
};

class DungeonGen {
public:
	ManualMapVoxelManipulator *vm;
	Mapgen *mg;
	u32 blockseed;
	PseudoRandom random;
	v3POS csize;

	content_t c_torch;
	DungeonParams dp;
	
	//RoomWalker
	v3POS m_pos;
	v3POS m_dir;

	DungeonGen(Mapgen *mg, DungeonParams *dparams);
	void generate(u32 bseed, v3POS full_node_min, v3POS full_node_max);
	
	void makeDungeon(v3POS start_padding);
	void makeRoom(v3POS roomsize, v3POS roomplace);
	void makeCorridor(v3POS doorplace, v3POS doordir,
					  v3POS &result_place, v3POS &result_dir);
	void makeDoor(v3POS doorplace, v3POS doordir);
	void makeFill(v3POS place, v3POS size, u8 avoid_flags, MapNode n, u8 or_flags);
	void makeHole(v3POS place);

	bool findPlaceForDoor(v3POS &result_place, v3POS &result_dir);
	bool findPlaceForRoomDoor(v3POS roomsize, v3POS &result_doorplace,
			v3POS &result_doordir, v3POS &result_roomplace);
			
	void randomizeDir()
	{
		m_dir = rand_ortho_dir(random, dp.diagonal_dirs);
	}
};

extern NoiseParams nparams_dungeon_rarity;
extern NoiseParams nparams_dungeon_wetness;
extern NoiseParams nparams_dungeon_density;

#endif
