

LuaPostMapgen::LuaPostMapgen(MapgenParams *params, EmergeParams *emerge)
{
	mg = new MapgenBasic(MAPGEN_SINGLENODE, params, emerge)
}

LuaPostMapgen::~LuaPostMapgen()
{
	delete mg;
}

int LuaPostMapgen::l_set_area(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	mg->vm = LuaVoxelManip::checkobject(L, 1)->vm;

	v3s16 pmin = lua_istable(L, 2) ? check_v3s16(L, 2) :
			mg.vm->m_area.MinEdge + v3s16(1,1,1) * MAP_BLOCKSIZE;
	v3s16 pmax = lua_istable(L, 3) ? check_v3s16(L, 3) :
			mg.vm->m_area.MaxEdge - v3s16(1,1,1) * MAP_BLOCKSIZE;
	sortBoxVerticies(pmin, pmax);

	mg->node_min = pmin;
	mg->node_max = pmax;

	return 0;
}

int LuaPostMapgen::l_generate_biomes(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	mg->generating = true;
	mg->generateBiomes();
	mg->generating = false;

	return 0;
}

int LuaPostMapgen::l_generate_ores(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	mg->generating = true;
	mg->m_emerge->oremgr->placeAllOres(mg, mg->blockseed, mg->node_min, mg->node_max);
	mg->generating = false;

	return 0;
}

int LuaPostMapgen::l_generate_decorations(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	mg->generating = true;
	mg->m_emerge->decomgr->placeAllDecos(mg, mg->blockseed, mg->node_min, mg->node_max);
	mg->generating = false;

	return 0;
}

int LuaPostMapgen::l_dust_top_nodes(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	mg->generating = true;
	mg->dustTopNodes();
	mg->generating = false;

	return 0;
}

void LuaPostMapgen::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);

	markAliasDeprecated(methods);
	luaL_openlib(L, 0, methods, 0);
	lua_pop(L, 1);

	lua_register(L, className, create_object);
}

const char LuaPostMapgen::className[] = "PostMapgen";
const luaL_Reg methods[] = {
	luamethod(LuaPostMapgen, set_area),
	luamethod(LuaPostMapgen, generate_biomes),
	luamethod(LuaPostMapgen, generate_ores),
	luamethod(LuaPostMapgen, generate_decorations),
	luamethod(LuaPostMapgen, dust_top_nodes),
}

//MODIFIED
