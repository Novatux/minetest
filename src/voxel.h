/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef VOXEL_HEADER
#define VOXEL_HEADER

#include "irrlichttypes.h"
#include "irr_v3d.h"
#include <iostream>
#include "debug.h"
#include "exceptions.h"
#include "mapnode.h"
#include <set>
#include <list>
#include <map>

class INodeDefManager;

// For VC++
#undef min
#undef max

/*
	A fast voxel manipulator class.

	In normal operation, it fetches more map when it is requested.
	It can also be used so that all allowed area is fetched at the
	start, using ManualMapVoxelManipulator.

	Not thread-safe.
*/

/*
	Debug stuff
*/
extern u32 emerge_time;
extern u32 emerge_load_time;

/*
	This class resembles aabbox3d<s16> a lot, but has inclusive
	edges for saner handling of integer sizes
*/
class VoxelArea
{
public:
	// Starts as zero sized
	VoxelArea():
		MinEdge(1,1,1),
		MaxEdge(0,0,0)
	{
	}
	VoxelArea(v3POS min_edge, v3POS max_edge):
		MinEdge(min_edge),
		MaxEdge(max_edge)
	{
	}
	VoxelArea(v3POS p):
		MinEdge(p),
		MaxEdge(p)
	{
	}

	/*
		Modifying methods
	*/

	void addArea(VoxelArea &a)
	{
		if(getExtent() == v3POS(0,0,0))
		{
			*this = a;
			return;
		}
		if(a.MinEdge.X < MinEdge.X) MinEdge.X = a.MinEdge.X;
		if(a.MinEdge.Y < MinEdge.Y) MinEdge.Y = a.MinEdge.Y;
		if(a.MinEdge.Z < MinEdge.Z) MinEdge.Z = a.MinEdge.Z;
		if(a.MaxEdge.X > MaxEdge.X) MaxEdge.X = a.MaxEdge.X;
		if(a.MaxEdge.Y > MaxEdge.Y) MaxEdge.Y = a.MaxEdge.Y;
		if(a.MaxEdge.Z > MaxEdge.Z) MaxEdge.Z = a.MaxEdge.Z;
	}
	void addPoint(v3POS p)
	{
		if(getExtent() == v3POS(0,0,0))
		{
			MinEdge = p;
			MaxEdge = p;
			return;
		}
		if(p.X < MinEdge.X) MinEdge.X = p.X;
		if(p.Y < MinEdge.Y) MinEdge.Y = p.Y;
		if(p.Z < MinEdge.Z) MinEdge.Z = p.Z;
		if(p.X > MaxEdge.X) MaxEdge.X = p.X;
		if(p.Y > MaxEdge.Y) MaxEdge.Y = p.Y;
		if(p.Z > MaxEdge.Z) MaxEdge.Z = p.Z;
	}

	// Pad with d nodes
	void pad(v3POS d)
	{
		MinEdge -= d;
		MaxEdge += d;
	}

	/*void operator+=(v3POS off)
	{
		MinEdge += off;
		MaxEdge += off;
	}

	void operator-=(v3POS off)
	{
		MinEdge -= off;
		MaxEdge -= off;
	}*/

	/*
		const methods
	*/

	v3POS getExtent() const
	{
		return MaxEdge - MinEdge + v3POS(1,1,1);
	}
	s32 getVolume() const
	{
		v3POS e = getExtent();
		return (s32)e.X * (s32)e.Y * (s32)e.Z;
	}
	bool contains(const VoxelArea &a) const
	{
		// No area contains an empty area
		// NOTE: Algorithms depend on this, so do not change.
		if(a.getExtent() == v3POS(0,0,0))
			return false;

		return(
			a.MinEdge.X >= MinEdge.X && a.MaxEdge.X <= MaxEdge.X &&
			a.MinEdge.Y >= MinEdge.Y && a.MaxEdge.Y <= MaxEdge.Y &&
			a.MinEdge.Z >= MinEdge.Z && a.MaxEdge.Z <= MaxEdge.Z
		);
	}
	bool contains(v3POS p) const
	{
		return(
			p.X >= MinEdge.X && p.X <= MaxEdge.X &&
			p.Y >= MinEdge.Y && p.Y <= MaxEdge.Y &&
			p.Z >= MinEdge.Z && p.Z <= MaxEdge.Z
		);
	}
	bool contains(s32 i) const
	{
		return (i >= 0 && i < getVolume());
	}
	bool operator==(const VoxelArea &other) const
	{
		return (MinEdge == other.MinEdge
				&& MaxEdge == other.MaxEdge);
	}

	VoxelArea operator+(v3POS off) const
	{
		return VoxelArea(MinEdge+off, MaxEdge+off);
	}

	VoxelArea operator-(v3POS off) const
	{
		return VoxelArea(MinEdge-off, MaxEdge-off);
	}

	/*
		Returns 0-6 non-overlapping areas that can be added to
		a to make up this area.

		a: area inside *this
	*/
	void diff(const VoxelArea &a, std::list<VoxelArea> &result)
	{
		/*
			This can result in a maximum of 6 areas
		*/

		// If a is an empty area, return the current area as a whole
		if(a.getExtent() == v3POS(0,0,0))
		{
			VoxelArea b = *this;
			if(b.getVolume() != 0)
				result.push_back(b);
			return;
		}

		assert(contains(a));

		// Take back area, XY inclusive
		{
			v3POS min(MinEdge.X, MinEdge.Y, a.MaxEdge.Z+1);
			v3POS max(MaxEdge.X, MaxEdge.Y, MaxEdge.Z);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

		// Take front area, XY inclusive
		{
			v3POS min(MinEdge.X, MinEdge.Y, MinEdge.Z);
			v3POS max(MaxEdge.X, MaxEdge.Y, a.MinEdge.Z-1);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

		// Take top area, X inclusive
		{
			v3POS min(MinEdge.X, a.MaxEdge.Y+1, a.MinEdge.Z);
			v3POS max(MaxEdge.X, MaxEdge.Y, a.MaxEdge.Z);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

		// Take bottom area, X inclusive
		{
			v3POS min(MinEdge.X, MinEdge.Y, a.MinEdge.Z);
			v3POS max(MaxEdge.X, a.MinEdge.Y-1, a.MaxEdge.Z);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

		// Take left area, non-inclusive
		{
			v3POS min(MinEdge.X, a.MinEdge.Y, a.MinEdge.Z);
			v3POS max(a.MinEdge.X-1, a.MaxEdge.Y, a.MaxEdge.Z);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

		// Take right area, non-inclusive
		{
			v3POS min(a.MaxEdge.X+1, a.MinEdge.Y, a.MinEdge.Z);
			v3POS max(MaxEdge.X, a.MaxEdge.Y, a.MaxEdge.Z);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

	}

	/*
		Translates position from virtual coordinates to array index
	*/
	s32 index(POS x, POS y, POS z) const
	{
		v3POS em = getExtent();
		v3POS off = MinEdge;
		s32 i = ((s32)(z-off.Z))*em.Y*em.X + ((s32)(y-off.Y))*em.X + (s32)(x-off.X);
		//dstream<<" i("<<x<<","<<y<<","<<z<<")="<<i<<" ";
		return i;
	}
	s32 index(v3POS p) const
	{
		return index(p.X, p.Y, p.Z);
	}

	// Translate index in the X coordinate
	void add_x(const v3POS &extent, u32 &i, POS a)
	{
		i += a;
	}
	// Translate index in the Y coordinate
	void add_y(const v3POS &extent, u32 &i, POS a)
	{
		i += a * extent.X;
	}
	// Translate index in the Z coordinate
	void add_z(const v3POS &extent, u32 &i, POS a)
	{
		i += a * extent.X*extent.Y;
	}
	// Translate index in space
	void add_p(const v3POS &extent, u32 &i, v3POS a)
	{
		i += a.Z*extent.X*extent.Y + a.Y*extent.X + a.X;
	}

	/*
		Print method for debugging
	*/
	void print(std::ostream &o) const
	{
		v3POS e = getExtent();
		o<<"("<<MinEdge.X
		 <<","<<MinEdge.Y
		 <<","<<MinEdge.Z
		 <<")("<<MaxEdge.X
		 <<","<<MaxEdge.Y
		 <<","<<MaxEdge.Z
		 <<")"
		 <<"="<<e.X<<"x"<<e.Y<<"x"<<e.Z<<"="<<getVolume();
	}

	// Edges are inclusive
	v3POS MinEdge;
	v3POS MaxEdge;
};

// Hasn't been copied from source (emerged)
#define VOXELFLAG_NOT_LOADED (1<<0)
// Checked as being inexistent in source
#define VOXELFLAG_INEXISTENT (1<<1)
// Algorithm-dependent
#define VOXELFLAG_CHECKED1 (1<<2)
// Algorithm-dependent
#define VOXELFLAG_CHECKED2 (1<<3)
// Algorithm-dependent
#define VOXELFLAG_CHECKED3 (1<<4)
// Algorithm-dependent
#define VOXELFLAG_CHECKED4 (1<<5)

enum VoxelPrintMode
{
	VOXELPRINT_NOTHING,
	VOXELPRINT_MATERIAL,
	VOXELPRINT_WATERPRESSURE,
	VOXELPRINT_LIGHT_DAY,
};

class VoxelManipulator /*: public NodeContainer*/
{
public:
	VoxelManipulator();
	virtual ~VoxelManipulator();

	/*
		Virtuals from NodeContainer
	*/
	/*virtual u16 nodeContainerId() const
	{
		return NODECONTAINER_ID_VOXELMANIPULATOR;
	}
	bool isValidPosition(v3POS p)
	{
		emerge(p);
		return !(m_flags[m_area.index(p)] & VOXELFLAG_INEXISTENT);
	}*/

	/*
		These are a bit slow and shouldn't be used internally.
		Use m_data[m_area.index(p)] instead.
	*/
	MapNode getNode(v3POS p)
	{
		emerge(p);

		if(m_flags[m_area.index(p)] & VOXELFLAG_INEXISTENT)
		{
			/*dstream<<"EXCEPT: VoxelManipulator::getNode(): "
					<<"p=("<<p.X<<","<<p.Y<<","<<p.Z<<")"
					<<", index="<<m_area.index(p)
					<<", flags="<<(int)m_flags[m_area.index(p)]
					<<" is inexistent"<<std::endl;*/
			throw InvalidPositionException
			("VoxelManipulator: getNode: inexistent");
		}

		return m_data[m_area.index(p)];
	}
	MapNode getNodeNoEx(v3POS p)
	{
		emerge(p);

		if(m_flags[m_area.index(p)] & VOXELFLAG_INEXISTENT)
		{
			return MapNode(CONTENT_IGNORE);
		}

		return m_data[m_area.index(p)];
	}
	MapNode getNodeNoExNoEmerge(v3POS p)
	{
		if(m_area.contains(p) == false)
			return MapNode(CONTENT_IGNORE);
		if(m_flags[m_area.index(p)] & VOXELFLAG_INEXISTENT)
			return MapNode(CONTENT_IGNORE);
		return m_data[m_area.index(p)];
	}
	// Stuff explodes if non-emerged area is touched with this.
	// Emerge first, and check VOXELFLAG_INEXISTENT if appropriate.
	MapNode & getNodeRefUnsafe(v3POS p)
	{
		return m_data[m_area.index(p)];
	}
	u8 & getFlagsRefUnsafe(v3POS p)
	{
		return m_flags[m_area.index(p)];
	}
	bool exists(v3POS p)
	{
		return m_area.contains(p) &&
			!(getFlagsRefUnsafe(p) & VOXELFLAG_INEXISTENT);
	}
	MapNode & getNodeRef(v3POS p)
	{
		emerge(p);
		if(getFlagsRefUnsafe(p) & VOXELFLAG_INEXISTENT)
		{
			/*dstream<<"EXCEPT: VoxelManipulator::getNode(): "
					<<"p=("<<p.X<<","<<p.Y<<","<<p.Z<<")"
					<<", index="<<m_area.index(p)
					<<", flags="<<(int)getFlagsRefUnsafe(p)
					<<" is inexistent"<<std::endl;*/
			throw InvalidPositionException
			("VoxelManipulator: getNode: inexistent");
		}
		return getNodeRefUnsafe(p);
	}
	void setNode(v3POS p, const MapNode &n)
	{
		emerge(p);

		m_data[m_area.index(p)] = n;
		m_flags[m_area.index(p)] &= ~VOXELFLAG_INEXISTENT;
		m_flags[m_area.index(p)] &= ~VOXELFLAG_NOT_LOADED;
	}
	// TODO: Should be removed and replaced with setNode
	void setNodeNoRef(v3POS p, const MapNode &n)
	{
		setNode(p, n);
	}

	/*void setExists(VoxelArea a)
	{
		emerge(a);
		for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
		for(s32 y=a.MinEdge.Y; y<=a.MaxEdge.Y; y++)
		for(s32 x=a.MinEdge.X; x<=a.MaxEdge.X; x++)
		{
			m_flags[m_area.index(x,y,z)] &= ~VOXELFLAG_INEXISTENT;
		}
	}*/

	/*MapNode & operator[](v3POS p)
	{
		//dstream<<"operator[] p=("<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;
		if(isValidPosition(p) == false)
			emerge(VoxelArea(p));

		return m_data[m_area.index(p)];
	}*/

	/*
		Set stuff if available without an emerge.
		Return false if failed.
		This is convenient but slower than playing around directly
		with the m_data table with indices.
	*/
	bool setNodeNoEmerge(v3POS p, MapNode n)
	{
		if(m_area.contains(p) == false)
			return false;
		m_data[m_area.index(p)] = n;
		return true;
	}
	bool setNodeNoEmerge(s32 i, MapNode n)
	{
		if(m_area.contains(i) == false)
			return false;
		m_data[i] = n;
		return true;
	}
	/*bool setContentNoEmerge(v3POS p, u8 c)
	{
		if(isValidPosition(p) == false)
			return false;
		m_data[m_area.index(p)].d = c;
	}*/

	/*
		Control
	*/

	virtual void clear();

	void print(std::ostream &o, INodeDefManager *nodemgr,
			VoxelPrintMode mode=VOXELPRINT_MATERIAL);

	void addArea(VoxelArea area);

	/*
		Copy data and set flags to 0
		dst_area.getExtent() <= src_area.getExtent()
	*/
	void copyFrom(MapNode *src, VoxelArea src_area,
			v3POS from_pos, v3POS to_pos, v3POS size);

	// Copy data
	void copyTo(MapNode *dst, VoxelArea dst_area,
			v3POS dst_pos, v3POS from_pos, v3POS size);

	/*
		Algorithms
	*/

	void clearFlag(u8 flag);

	// TODO: Move to voxelalgorithms.h

	void unspreadLight(enum LightBank bank, v3POS p, u8 oldlight,
			std::set<v3POS> & light_sources, INodeDefManager *nodemgr);
	void unspreadLight(enum LightBank bank,
			std::map<v3POS, u8> & from_nodes,
			std::set<v3POS> & light_sources, INodeDefManager *nodemgr);

	void spreadLight(enum LightBank bank, v3POS p, INodeDefManager *nodemgr);
	void spreadLight(enum LightBank bank,
			std::set<v3POS> & from_nodes, INodeDefManager *nodemgr);

	/*
		Virtual functions
	*/

	/*
		Get the contents of the requested area from somewhere.
		Shall touch only nodes that have VOXELFLAG_NOT_LOADED
		Shall reset VOXELFLAG_NOT_LOADED

		If not found from source, add with VOXELFLAG_INEXISTENT
	*/
	virtual void emerge(VoxelArea a, s32 caller_id=-1)
	{
		//dstream<<"emerge p=("<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;
		addArea(a);
		for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
		for(s32 y=a.MinEdge.Y; y<=a.MaxEdge.Y; y++)
		for(s32 x=a.MinEdge.X; x<=a.MaxEdge.X; x++)
		{
			s32 i = m_area.index(x,y,z);
			// Don't touch nodes that have already been loaded
			if(!(m_flags[i] & VOXELFLAG_NOT_LOADED))
				continue;
			m_flags[i] = VOXELFLAG_INEXISTENT;
		}
	}

	/*
		Member variables
	*/

	/*
		The area that is stored in m_data.
		addInternalBox should not be used if getExtent() == v3POS(0,0,0)
		MaxEdge is 1 higher than maximum allowed position
	*/
	VoxelArea m_area;

	/*
		NULL if data size is 0 (extent (0,0,0))
		Data is stored as [z*h*w + y*h + x]
	*/
	MapNode *m_data;

	/*
		Flags of all nodes
	*/
	u8 *m_flags;

	//TODO: Use these or remove them
	//TODO: Would these make any speed improvement?
	//bool m_pressure_route_valid;
	//v3POS m_pressure_route_surface;

	/*
		Some settings
	*/
	//bool m_disable_water_climb;

private:
};

#endif

