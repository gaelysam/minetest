

class LuaPostMapgen : public ModApiBase
{
private:
	static const char className[];
	static const luaL_Reg methods[];

	static int l_set_area(lua_State *L);

	static int l_generate_biomes(lua_State *L);

	static int l_generate_ores(lua_State *L);
	static int l_generate_decorations(lua_State *L);

	static int l_dust_top_nodes(lua_State *L);

public:
	MapgenBasic *mg;

	LuaPostMapgen(MapgenParams *params, EmergeParams *emerge);
	~LuaPostMapgen();
};

//MODIFIED
