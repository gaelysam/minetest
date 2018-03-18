--Minetest
--Copyright (C) 2014 sapier
--
--This program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 2.1 of the License, or
--(at your option) any later version.
--
--This program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with this program; if not, write to the Free Software Foundation, Inc.,
--51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

local function mg_preset_edit_formspec(data)
	local mapgens = core.get_mapgen_names()
	local current_mg = data.mgname

	local mglist = ""
	local selindex = 1
	local i = 1
	for k,v in pairs(mapgens) do
		if current_mg == v then
			selindex = i
		end
		i = i + 1
		mglist = mglist .. v .. ","
	end
	mglist = mglist:sub(1, -2)

	local name = data.name or ""
	local author = data.author or core.settings:get("name")

	local retval =
		"size[11.5,6.5,true]" ..
		"label[0,0;" .. fgettext("Edit a mapgen preset") .. "]" ..

		"label[0,1;" .. fgettext("Preset name") .. "]"..
		"field[2,1.4;3,0.5;te_preset_name;;" .. name .. "]" ..

		"label[0,2;" .. fgettext("Mapgen") .. "]" ..
		"dropdown[1.75,2;3;dd_mapgen;" .. mglist .. ";" .. selindex .. "]" ..

		"label[0,3;" .. fgettext("Author") .. "]" ..
		"field[2,3.4;3,0.5;te_preset_author;;" .. author .. "]" ..

		"label[5.5,0;" .. fgettext("Description") .. "]" ..
		"textarea[5.5,1;5.5,3;te_preset_desc;;]" ..

		"button[3.25,4;5,0.5;preset_edit_params;" .. fgettext("Edit parameters") .. "]" ..

		"button[3.25,6;2.5,0.5;preset_confirm;" .. fgettext("Save") .. "]" ..
		"button[5.75,6;2.5,0.5;preset_cancel;" .. fgettext("Cancel") .. "]"
		
	return retval

end

local mapgen_settings_paths = {
	v5 = {"Mapgen/Mapgen flags", "Mapgen/Mapgen V5", "Mapgen/Biome API temperature and humidity noise parameters"},
	v6 = {"Mapgen/Mapgen flags", "Mapgen/Mapgen V6"},
	v7 = {"Mapgen/Mapgen flags", "Mapgen/Mapgen V7", "Mapgen/Biome API temperature and humidity noise parameters"},
	valleys = {"Mapgen/Mapgen flags", "Mapgen/Mapgen Valleys", "Mapgen/Biome API temperature and humidity noise parameters"},
	carpathian = {"Mapgen/Mapgen flags", "Mapgen/Mapgen Carpathian", "Mapgen/Biome API temperature and humidity noise parameters"},
	fractal = {"Mapgen/Mapgen flags", "Mapgen/Mapgen Fractal", "Mapgen/Biome API temperature and humidity noise parameters"},
	flat = {"Mapgen/Mapgen flags", "Mapgen/Mapgen Flat", "Mapgen/Biome API temperature and humidity noise parameters"},
	singlenode = {},
}

function get_mapgen_settings_path(mgname)
	return mapgen_settings_paths[mgname]
end

local paths

local function mg_preset_edit_buttonhandler(this, fields)
	if fields["preset_confirm"] then
		-- To be implemented
		this:delete()
		return true
	end

	if fields["preset_cancel"] then
		this:delete()
		return true
	end

	if fields["preset_edit_params"] then
		local mgname = fields["dd_mapgen"]
		local adv_settings_dlg = create_adv_settings_dlg(mgname, paths)
		adv_settings_dlg:set_parent(this)
		this:hide()
		adv_settings_dlg:show()
		return true
	end

	return false
end


function create_mg_preset_edit_dlg(mgname, name, author)
	local dlg = dialog_create("mg_preset_edit",
					mg_preset_edit_formspec,
					mg_preset_edit_buttonhandler,
					nil)
	dlg.data.mgname = mgname
	dlg.data.name = name
	dlg.data.author = author
	paths = mapgen_settings_paths[mgname]
	
	return dlg
end
