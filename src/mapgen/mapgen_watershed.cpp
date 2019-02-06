/*
Minetest
Copyright (C) 2019 paramat

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


#include <cmath>
#include "mapgen.h"
#include "voxel.h"
#include "noise.h"
#include "mapblock.h"
#include "mapnode.h"
#include "map.h"
#include "content_sao.h"
#include "nodedef.h"
#include "voxelalgorithms.h"
//#include "profiler.h" // For TimeTaker
#include "settings.h" // For g_settings
#include "emerge.h"
#include "dungeongen.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "cavegen.h"
#include "mapgen_watershed.h"


FlagDesc flagdesc_mapgen_watershed[] = {
	{"vents", MGWATERSHED_VENTS},
	{NULL   , 0}
};


MapgenWatershed::MapgenWatershed(int mapgenid, MapgenWatershedParams *params,
	EmergeManager *emerge)
	: MapgenBasic(mapgenid, params, emerge)
{
	spflags          = params->spflags;
	div              = std::fmax(params->map_scale, 1.0f);
	sea_y            = params->sea_y;
	flat_y           = params->flat_y;
	continent_area   = params->continent_area;
	river_width      = params->river_width;
	river_depth      = params->river_depth;
	river_bank       = params->river_bank;
	big_dungeon_ymin = params->big_dungeon_ymin;
	big_dungeon_ymax = params->big_dungeon_ymax;

	cave_width       = params->cave_width;
	large_cave_depth = params->large_cave_depth;
	lava_depth       = params->lava_depth;
	cavern_limit     = params->cavern_limit;
	cavern_taper     = params->cavern_taper;
	cavern_threshold = params->cavern_threshold;
	dungeon_ymin     = params->dungeon_ymin;
	dungeon_ymax     = params->dungeon_ymax;

	// Density gradient
	vertical_scale = 128.0f;

	// Calculate density value of terrain base at flat area level, before division of
	// vertical_scale by div.
	base_flat = (flat_y - sea_y) / vertical_scale;

	// Create noise parameter copies to enable scaling without altering configured or
	// stored noise parameters.
	np_vent_div        = params->np_vent;
	np_continent_div   = params->np_continent;
	np_base_div        = params->np_base;
	np_flat_div        = params->np_flat;
	np_river1_div      = params->np_river1;
	np_river2a_div      = params->np_river2a;
	np_river2b_div      = params->np_river2b;
	np_mountain_div    = params->np_mountain;
	np_plateau_div     = params->np_plateau;
	np_plat_select_div = params->np_plat_select;
	np_3d_div          = params->np_3d;

	// If in scaled mode, divide vertical scale and noise spreads
	if (div != 1.0f) {
		vertical_scale            /= div;

		np_vent_div.spread        /= v3f(div, div, div);
		np_continent_div.spread   /= v3f(div, div, div);
		np_base_div.spread        /= v3f(div, div, div);
		np_flat_div.spread        /= v3f(div, div, div);
		np_river1_div.spread      /= v3f(div, div, div);
		np_river2a_div.spread     /= v3f(div, div, div);
		np_river2b_div.spread     /= v3f(div, div, div);
		np_mountain_div.spread    /= v3f(div, div, div);
		np_plateau_div.spread     /= v3f(div, div, div);
		np_plat_select_div.spread /= v3f(div, div, div);
		np_3d_div.spread          /= v3f(div, div, div);
	}

	//// 2D Terrain noise
	noise_vent        = new Noise(&np_vent_div,        seed, csize.X, csize.Z);
	noise_continent   = new Noise(&np_continent_div,   seed, csize.X, csize.Z);
	noise_base        = new Noise(&np_base_div,        seed, csize.X, csize.Z);
	noise_flat        = new Noise(&np_flat_div,        seed, csize.X, csize.Z);
	noise_river1      = new Noise(&np_river1_div,      seed, csize.X, csize.Z);
	noise_river2a      = new Noise(&np_river2a_div,    seed, csize.X, csize.Z);
	noise_river2b      = new Noise(&np_river2b_div,    seed, csize.X, csize.Z);
	noise_mountain    = new Noise(&np_mountain_div,    seed, csize.X, csize.Z);
	noise_plateau     = new Noise(&np_plateau_div,     seed, csize.X, csize.Z);
	noise_plat_select = new Noise(&np_plat_select_div, seed, csize.X, csize.Z);

	noise_filler_depth = new Noise(&params->np_filler_depth, seed, csize.X, csize.Z);

	//// 3D terrain noise
	// 1 up 1 down overgeneration
	noise_3d =
		new Noise(&np_3d_div, seed, csize.X, csize.Y + 2, csize.Z);
	// 1 down overgeneration
	MapgenBasic::np_cave1 = params->np_cave1;
	MapgenBasic::np_cave2 = params->np_cave2;
	MapgenBasic::np_cavern = params->np_cavern;

	//// Dungeon noises
	// Dummy dungeon density noise for dungeongen.cpp
	np_dungeon = NoiseParams(32.0, 0.0, v3f(128, 128, 128), 0, 1, 0.5, 2.0);
	// Actual big dungeon density noise
	np_big_dungeon = params->np_big_dungeon;

	//// Resolve nodes to be used
	c_volcanic_rock = ndef->getId("mapgen_volcanic_rock");
	if (c_volcanic_rock == CONTENT_IGNORE)
		c_volcanic_rock = c_stone;
}


MapgenWatershed::~MapgenWatershed()
{
	delete noise_vent;
	delete noise_continent;
	delete noise_base;
	delete noise_flat;
	delete noise_river1;
	delete noise_river2a;
	delete noise_river2b;
	delete noise_mountain;
	delete noise_plateau;
	delete noise_plat_select;
	delete noise_3d;

	delete noise_filler_depth;
}


MapgenWatershedParams::MapgenWatershedParams()
{
	spflags          = MGWATERSHED_VENTS;
	map_scale        = 1.0;
	sea_y            = 1.0;
	flat_y           = 7.0;
	continent_area   = -1.0;
	river_width      = 0.06;
	river_depth      = 0.25;
	river_bank       = 0.01;
	big_dungeon_ymin = -31000;
	big_dungeon_ymax = 31000;

	cave_width       = 0.1;
	large_cave_depth = -33;
	lava_depth       = -256;
	cavern_limit     = -256;
	cavern_taper     = 256;
	cavern_threshold = 0.7;
	dungeon_ymin     = -31000;
	dungeon_ymax     = 31000;

	np_vent         = NoiseParams(-1.0,  1.07, v3f(48,   48,   48),   692,   1, 0.5,  2.0);
	np_continent    = NoiseParams(0.0,   1.0,  v3f(12288,12288,12288),4001,  3, 0.5,  2.0);
	np_base         = NoiseParams(0.0,   1.0,  v3f(2048, 2048, 2048), 106,   3, 0.5,  2.0);
	np_flat         = NoiseParams(0.0,   0.4,  v3f(2048, 2048, 2048), 909,   3, 0.5,  2.0);
	np_river1       = NoiseParams(0.0,   1.0,  v3f(1024, 1024, 1024), 2177,  5, 0.5,  2.0);
	np_river2a      = NoiseParams(0.0,   1.0,  v3f(512,  512,  512),  5003,  5, 0.5,  2.0);
	np_river2b      = NoiseParams(0.0,   1.0,  v3f(512,  512,  512),  8839,  5, 0.5,  2.0);
	np_mountain     = NoiseParams(2.0,   -1.0, v3f(1536, 1536, 1536), 50001, 7, 0.6,  2.0,
		NOISE_FLAG_EASED | NOISE_FLAG_ABSVALUE);
	np_plateau      = NoiseParams(0.5,   0.2,  v3f(1024, 1024, 1024), 8111,  4, 0.4,  2.0);
	np_plat_select  = NoiseParams(-2.0,  6.0,  v3f(2048, 2048, 2048), 30089, 8, 0.7,  2.0);
	np_3d           = NoiseParams(0.0,   1.0,  v3f(384,  384,  384),  70033, 5, 0.63, 2.0);
	np_big_dungeon  = NoiseParams(0.0,   1.25, v3f(128,  128,  128),  23,    1, 0.5,  2.0);

	np_filler_depth = NoiseParams(0.0,   1.0,  v3f(128,  128,  128),  261,   3, 0.7,  2.0);
	np_cave1        = NoiseParams(0.0,   12.0, v3f(61,   61,   61),   52534, 3, 0.5,  2.0);
	np_cave2        = NoiseParams(0.0,   12.0, v3f(67,   67,   67),   10325, 3, 0.5,  2.0);
	np_cavern       = NoiseParams(0.0,   1.0,  v3f(384,  128,  384),  723,   5, 0.63, 2.0);
}


void MapgenWatershedParams::readParams(const Settings *settings)
{
	settings->getFlagStrNoEx("mgwatershed_spflags", spflags, flagdesc_mapgen_watershed);
	settings->getFloatNoEx("mgwatershed_map_scale",      map_scale);
	settings->getFloatNoEx("mgwatershed_sea_y",          sea_y);
	settings->getFloatNoEx("mgwatershed_flat_y",         flat_y);
	settings->getFloatNoEx("mgwatershed_continent_area", continent_area);
	settings->getFloatNoEx("mgwatershed_river_width",    river_width);
	settings->getFloatNoEx("mgwatershed_river_depth",    river_depth);
	settings->getFloatNoEx("mgwatershed_river_bank",     river_bank);
	settings->getS16NoEx("mgwatershed_big_dungeon_ymin", big_dungeon_ymin);
	settings->getS16NoEx("mgwatershed_big_dungeon_ymax", big_dungeon_ymax);

	settings->getFloatNoEx("mgwatershed_cave_width",       cave_width);
	settings->getS16NoEx("mgwatershed_large_cave_depth",   large_cave_depth);
	settings->getS16NoEx("mgwatershed_lava_depth",         lava_depth);
	settings->getS16NoEx("mgwatershed_cavern_limit",       cavern_limit);
	settings->getS16NoEx("mgwatershed_cavern_taper",       cavern_taper);
	settings->getFloatNoEx("mgwatershed_cavern_threshold", cavern_threshold);
	settings->getS16NoEx("mgwatershed_dungeon_ymin",       dungeon_ymin);
	settings->getS16NoEx("mgwatershed_dungeon_ymax",       dungeon_ymax);

	settings->getNoiseParams("mgwatershed_np_vent",        np_vent);
	settings->getNoiseParams("mgwatershed_np_continent",   np_continent);
	settings->getNoiseParams("mgwatershed_np_base",        np_base);
	settings->getNoiseParams("mgwatershed_np_flat",        np_flat);
	settings->getNoiseParams("mgwatershed_np_river1",      np_river1);
	settings->getNoiseParams("mgwatershed_np_river2a",     np_river2a);
	settings->getNoiseParams("mgwatershed_np_river2b",     np_river2b);
	settings->getNoiseParams("mgwatershed_np_mountain",    np_mountain);
	settings->getNoiseParams("mgwatershed_np_plateau",     np_plateau);
	settings->getNoiseParams("mgwatershed_np_plat_select", np_plat_select);
	settings->getNoiseParams("mgwatershed_np_3d",          np_3d);
	settings->getNoiseParams("mgwatershed_np_big_dungeon", np_big_dungeon);

	settings->getNoiseParams("mgwatershed_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgwatershed_np_cave1",        np_cave1);
	settings->getNoiseParams("mgwatershed_np_cave2",        np_cave2);
	settings->getNoiseParams("mgwatershed_np_cavern",       np_cavern);
}


void MapgenWatershedParams::writeParams(Settings *settings) const
{
	settings->setFlagStr("mgwatershed_spflags", spflags, flagdesc_mapgen_watershed, U32_MAX);
	settings->setFloat("mgwatershed_map_scale",      map_scale);
	settings->setFloat("mgwatershed_sea_y",          sea_y);
	settings->setFloat("mgwatershed_flat_y",         flat_y);
	settings->setFloat("mgwatershed_continent_area", continent_area);
	settings->setFloat("mgwatershed_river_width",    river_width);
	settings->setFloat("mgwatershed_river_depth",    river_depth);
	settings->setFloat("mgwatershed_river_bank",     river_bank);
	settings->setS16("mgwatershed_big_dungeon_ymin", big_dungeon_ymin);
	settings->setS16("mgwatershed_big_dungeon_ymax", big_dungeon_ymax);

	settings->setFloat("mgwatershed_cave_width",       cave_width);
	settings->setS16("mgwatershed_large_cave_depth",   large_cave_depth);
	settings->setS16("mgwatershed_lava_depth",         lava_depth);
	settings->setS16("mgwatershed_cavern_limit",       cavern_limit);
	settings->setS16("mgwatershed_cavern_taper",       cavern_taper);
	settings->setFloat("mgwatershed_cavern_threshold", cavern_threshold);
	settings->setS16("mgwatershed_dungeon_ymin",       dungeon_ymin);
	settings->setS16("mgwatershed_dungeon_ymax",       dungeon_ymax);

	settings->setNoiseParams("mgwatershed_np_vent",        np_vent);
	settings->setNoiseParams("mgwatershed_np_continent",   np_continent);
	settings->setNoiseParams("mgwatershed_np_base",        np_base);
	settings->setNoiseParams("mgwatershed_np_flat",        np_flat);
	settings->setNoiseParams("mgwatershed_np_river1",      np_river1);
	settings->setNoiseParams("mgwatershed_np_river2a",     np_river2a);
	settings->setNoiseParams("mgwatershed_np_river2b",     np_river2b);
	settings->setNoiseParams("mgwatershed_np_mountain",    np_mountain);
	settings->setNoiseParams("mgwatershed_np_plateau",     np_plateau);
	settings->setNoiseParams("mgwatershed_np_plat_select", np_plat_select);
	settings->setNoiseParams("mgwatershed_np_3d",          np_3d);
	settings->setNoiseParams("mgwatershed_np_big_dungeon", np_big_dungeon);

	settings->setNoiseParams("mgwatershed_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgwatershed_np_cave1",        np_cave1);
	settings->setNoiseParams("mgwatershed_np_cave2",        np_cave2);
	settings->setNoiseParams("mgwatershed_np_cavern",       np_cavern);
}


int MapgenWatershed::getSpawnLevelAtPoint(v2s16 p)
{
	return water_level + 64;
}


void MapgenWatershed::makeChunk(BlockMakeData *data)
{
	// Pre-conditions
	assert(data->vmanip);
	assert(data->nodedef);
	assert(data->blockpos_requested.X >= data->blockpos_min.X &&
			data->blockpos_requested.Y >= data->blockpos_min.Y &&
			data->blockpos_requested.Z >= data->blockpos_min.Z);
	assert(data->blockpos_requested.X <= data->blockpos_max.X &&
			data->blockpos_requested.Y <= data->blockpos_max.Y &&
			data->blockpos_requested.Z <= data->blockpos_max.Z);

	//TimeTaker t("makeChunk");

	this->generating = true;
	this->vm         = data->vmanip;
	this->ndef       = data->nodedef;

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	blockseed = getBlockSeed2(full_node_min, seed);

	// Generate terrain
	s16 stone_surface_max_y = generateTerrain();

	// If not scaled mode
	if (div == 1.0f) {
		// Create heightmap
		updateHeightmap(node_min, node_max);

		// Init biome generator, place biome-specific nodes, and build biomemap
		if (flags & MG_BIOMES) {
			biomegen->calcBiomeNoise(node_min);
			generateBiomes();
		}

		// Generate caverns, tunnels and classic caves
		if (flags & MG_CAVES) {
			// Generate tunnels first as caverns confuse them
			generateCavesNoiseIntersection(stone_surface_max_y);

			// Generate caverns
			bool near_cavern = generateCavernsNoise(stone_surface_max_y);

			// Generate large randomwalk caves
			if (near_cavern)
				// Disable large randomwalk caves in this mapchunk by setting
				// 'large cave depth' to world base. Avoids excessive liquid in large
				// caverns and floating blobs of overgenerated liquid.
				// Avoids liquids leaking at world edge.
				generateCavesRandomWalk(stone_surface_max_y, -MAX_MAP_GENERATION_LIMIT);
			else
				generateCavesRandomWalk(stone_surface_max_y, large_cave_depth);
		}

		// Generate the registered ores
		m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

		// Generate dungeons
		if (flags & MG_DUNGEONS) {
			float n_mdun = NoisePerlin3D(&np_big_dungeon,
				node_min.X, node_min.Y, node_min.Z, seed);
			if (n_mdun > 1.0f && node_min.Y < stone_surface_max_y &&
					full_node_min.Y >= big_dungeon_ymin &&
					full_node_max.Y <= big_dungeon_ymax) {
				// Get biome at mapchunk centre
				v3s16 chunk_mid = node_min + (node_max - node_min) / v3s16(2, 2, 2);
				Biome *biome = (Biome *)biomegen->getBiomeAtPoint(chunk_mid);

				DungeonParams dp;

				dp.seed             = seed;
				dp.only_in_ground   = false;
				dp.corridor_len_min = 2;
				dp.corridor_len_max = 16;
				dp.rooms_min        = 32;
				dp.rooms_max        = 32;

				dp.np_density  = np_dungeon;
				dp.np_alt_wall = nparams_dungeon_alt_wall;

				if (biome->c_dungeon != CONTENT_IGNORE) {
					// Biome-defined dungeon nodes
					dp.c_wall = biome->c_dungeon;
					// Stairs fall back to 'c_dungeon' if not defined by biome
					dp.c_stair = (biome->c_dungeon_stair != CONTENT_IGNORE) ?
						biome->c_dungeon_stair : biome->c_dungeon;
				} else {
					// Fallback to using biome 'node_stone'
					dp.c_wall  = biome->c_stone;
					dp.c_stair = biome->c_stone;
				}
				dp.c_alt_wall = CONTENT_IGNORE;

				dp.diagonal_dirs       = false;
				dp.holesize            = v3s16(3, 3, 3);
				dp.room_size_min       = v3s16(8, 4, 8);
				dp.room_size_max       = v3s16(16, 8, 16);
				dp.room_size_large_min = v3s16(8, 4, 8);
				dp.room_size_large_max = v3s16(16, 8, 16);
				dp.notifytype          = GENNOTIFY_DUNGEON;

				DungeonGen dgen(ndef, &gennotify, &dp);
				dgen.generate(vm, blockseed, full_node_min, full_node_max);
			} else if (full_node_min.Y >= dungeon_ymin &&
					full_node_max.Y <= dungeon_ymax) {
				generateDungeons(stone_surface_max_y);
			}
		}

		// Generate the registered decorations
		if (flags & MG_DECORATIONS)
			m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

		// Sprinkle some dust on top after everything else was generated
		if (flags & MG_BIOMES)
			dustTopNodes();
	}

	// Update liquids
	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	// Calculate lighting
	if (flags & MG_LIGHT)
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
			full_node_min, full_node_max, true);

	this->generating = false;

	//printf("makeChunk: %dms\n", t.stop());
}


int MapgenWatershed::generateTerrain()
{
	MapNode mn_air(CONTENT_AIR);
	MapNode mn_stone(c_stone);
	MapNode mn_water(c_water_source);
	MapNode mn_river_water(c_river_water_source);
	MapNode mn_magma(c_lava_source);
	MapNode mn_volcanic(c_volcanic_rock);
	//MapNode mn_sandstone(c_sandstone);

	noise_vent       ->perlinMap2D(node_min.X, node_min.Z);
	noise_continent  ->perlinMap2D(node_min.X, node_min.Z);
	noise_base       ->perlinMap2D(node_min.X, node_min.Z);
	noise_flat       ->perlinMap2D(node_min.X, node_min.Z);
	noise_river1     ->perlinMap2D(node_min.X, node_min.Z);
	noise_river2a    ->perlinMap2D(node_min.X, node_min.Z);
	noise_river2b    ->perlinMap2D(node_min.X, node_min.Z);
	noise_mountain   ->perlinMap2D(node_min.X, node_min.Z);
	noise_plateau    ->perlinMap2D(node_min.X, node_min.Z);
	noise_plat_select->perlinMap2D(node_min.X, node_min.Z);

	noise_3d         ->perlinMap3D(node_min.X, node_min.Y - 1, node_min.Z);

	// Cache density_gradient values
	float density_grad_cache[csize.Y + 2];
	u8 density_grad_index = 0; // Index zero at column base
	for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++, density_grad_index++) {
		density_grad_cache[density_grad_index] = (sea_y - y) / vertical_scale;
	}

	int stone_max_y = -MAX_MAP_GENERATION_LIMIT;
	const v3s16 &em = vm->m_area.getExtent();
	u32 index2d = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index2d++) {
		//// Terrain base / riverbank level.
		// Continental variation.
		// n_continent is roughly -1 to 1.
		float n_continent = continent_area +
			std::fabs(noise_continent->result[index2d]) * 2.0f;
		// n_cont_tanh is mostly -1 or 1, and smooth transition between
		float n_cont_tanh = std::tanh(n_continent * 4.0f);
		float n_base = noise_base->result[index2d];
		// Set continental and base amplitudes, and master offset here
		float n_tbase = n_cont_tanh * 0.6f + n_base * 1.0f - 0.2f;

		// Shape base terrain, add coastal flat areas
		float n_flat = std::fmax(noise_flat->result[index2d], 0.0f);
		float n_base_shaped = base_flat;
		if (n_tbase < base_flat) {
			// Set ocean depth here
			n_base_shaped = base_flat - (base_flat - n_tbase) * 0.2f;
		} else if (n_tbase > base_flat + n_flat) {
			// Set land height here
			n_base_shaped = base_flat +
				std::pow(n_tbase - (base_flat + n_flat), 1.5f) * 1.4f;
		}

		//// River valleys.
		// River valley 1.
		float n_river1 = noise_river1->result[index2d];
		float n_river1_abs = std::fabs(n_river1);
		// River valley 2: take noise 2A if noise 1 is positive, and noise 2B is noise 1 is negative.
		float n_river2 = (n_river1 > 0 ? noise_river2a : noise_river2b)->result[index2d];
		float n_river2_abs = std::fabs(n_river2);
		// River valley cross sections sink below 0 to define river area and width.
		// Set river source altitude here.
		float sink = (0.8f - n_base_shaped) * river_width;
		float n_valley1_sunk = n_river1_abs - sink;
		float n_valley2_sunk = n_river2_abs - sink;
		// Combine river valleys.
		// Smoothly interpolate between valleys using difference between them.
		float verp = std::tanh((n_valley2_sunk - n_valley1_sunk) * 16.0f) * 0.5f + 0.5f;
		float n_valley_sunk = verp * n_valley1_sunk + (1.0f - verp) * n_valley2_sunk;

		// Hills between rivers, and river channel
		float n_valley_shaped;
		if (n_valley_sunk > 0.0f) { // Between rivers
			// Smoothly blend valley amplitude into flat areas or coast
			float n_val_amp = 0.0f;
			// Set blend distance here
			float blend = (n_tbase - (base_flat + n_flat)) / 0.3f;
			if (blend >= 1.0f)
				n_val_amp = 1.0f;
			else if (blend > 0.0f && blend < 1.0f)
				// Smoothstep
				n_val_amp = blend * blend * (3.0f - 2.0f * blend);
			// Set valley hills amplitude here
			n_valley_shaped = std::pow(n_valley_sunk, 1.5f) * n_val_amp * 0.5f;
		} else { // River channels
			float river_depth_shaped = (n_base_shaped < 0.0f) ?
				// Remove channels below sea level
				std::fmax(river_depth + n_base_shaped * 4.0f, 0.0f) :
				river_depth;
			n_valley_shaped = -std::sqrt(-n_valley_sunk) * river_depth_shaped;
		}

		//// Mountains
		// Increase above river source level.
		// Set river source level here.
		float n_mount_amp = n_base_shaped - 0.8f;
		float n_mount = -1000.0f;
		if (n_mount_amp > 0.0f) {
			float n_mountain = noise_mountain->result[index2d];
			// Set mountain height here
			n_mount = n_mountain * n_mount_amp * n_mount_amp * 1.0f;
		}

		//// Plateaus.
		// The 2 terrains switched between.
		float n_lowland = n_base_shaped + std::fmax(n_valley_shaped, n_mount);
		float n_plateau = std::fmax(noise_plateau->result[index2d], n_lowland);
		// Plateau select.
		// 3D noise is added in Y loop.
		// Plateau select for plateau areas.
		float n_plat_select = noise_plat_select->result[index2d];
		// Plateau select for coastal cliffs
		float n_plat_sel_coast = (n_tbase + 0.1f) * 16.0f;
		// Plateau select for canyons
		float n_plat_sel_canyon = -1000.0f;
		if (n_valley_sunk > 0)
			n_plat_sel_canyon = n_base_shaped + std::pow(n_valley_sunk, 3.0f) * 1024.0f;

		//// Magma vents
		float n_vent = noise_vent->result[index2d];
		float mod = std::fmax(1.5f - n_tbase, 0.0f);
		// Restrict to higher land
		float n_vent_shaped = n_vent - mod * mod;

		//// Y loop
		// Indexes at column base
		density_grad_index = 0;
		u32 vi = vm->m_area.index(x, node_min.Y - 1, z);
		u32 index3d = (z - node_min.Z) * zstride_1u1d + (x - node_min.X);

		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1;
				y++,
				VoxelArea::add_y(em, vi, 1),
				index3d += ystride,
				density_grad_index++) {
			if (vm->m_data[vi].getContent() != CONTENT_IGNORE)
				continue;

			// 3D noise
			float n_3d = noise_3d->result[index3d];
			// Terrain select
			float n_select = std::fmin(
				std::fmin(n_plat_select, n_plat_sel_coast) + n_3d * 2.0f,
				n_plat_sel_canyon);
			// Terrain density noise
			float n_terrain = rangelim(n_select, n_lowland, n_plateau);
			// Density gradient
			float density_grad = density_grad_cache[density_grad_index];
			// Terrain base / riverbank level
			float density_base = n_base_shaped + density_grad;
			// Terrain level
			float density = n_terrain + density_grad;

			if (density >= 0.0f) {
				// Maximum vent wall thickness
				float vent_wall = 0.05f + std::fabs(n_3d) * 0.05f;
				if ((spflags & MGWATERSHED_VENTS) &&
						n_vent_shaped >= -vent_wall) { // Vent
					if (n_vent_shaped > 0.0f) { // Magma channel
						if (density_base >= 0.0f)
							vm->m_data[vi] = mn_magma; // Magma
						else
							vm->m_data[vi] = mn_air; // Magma channel air
					} else { // Vent wall and taper
						float cone = (n_vent_shaped + vent_wall) /
							vent_wall * 0.2f;
						if (density >= cone)
							vm->m_data[vi] = mn_volcanic; // Vent wall
						else
							vm->m_data[vi] = mn_air; // Taper air
					}
				} else {
					/*if (n_valley_shaped < n_mount)
						vm->m_data[vi] = mn_sandstone;
					else 
						vm->m_data[vi] = mn_stone;*/
					vm->m_data[vi] = mn_stone; // Stone
					if (y > stone_max_y)
						stone_max_y = y;
				}
			} else if (y <= water_level) {
				vm->m_data[vi] = mn_water; // Sea water
			} else if (density_base >= river_bank) {
				vm->m_data[vi] = mn_river_water; // River water
			} else {
				vm->m_data[vi] = mn_air; // Air
			}
		}
	}

	return stone_max_y;
}
