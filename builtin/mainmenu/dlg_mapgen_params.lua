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

local mgname

local function mapgen_params_formspec(data)
	local mapgens = core.get_mapgen_names()
	local preset_list = "Original,Voxelproof,Wider,Custom"

	local mglist = "all,"
	local selindex = 1
	local i = 1
	for k,v in pairs(mapgens) do
		if mgname == v then
			selindex = i+1
		end
		i = i + 1
		mglist = mglist .. v .. ","
	end
	mglist = mglist:sub(1, -2)

	local retval =
		"size[11.5,6.5,true]" ..
		"label[2,0;" .. fgettext("Mapgen") .. "]"..
		"dropdown[4.2,0;2.5;dd_mapgen;" .. mglist .. ";" .. selindex .. "]" ..

		"label[2,1;" .. fgettext("Mapgen presets") .. "]" ..
		"textlist[2,1.5;5.2,2.6;presets;" .. preset_list .. ";" .. "1" .. ";false]" ..
		"button[7.5,1.5;2.5,0.5;mg_preset_edit;" .. fgettext("Edit") .. "]" ..
		"button[7.5,2.5;2.5,0.5;mg_preset_copy_edit;" .. fgettext("Edit a copy") .. "]" ..
		"button[7.5,3.5;2.5,0.5;mg_preset_remove;" .. fgettext("Remove") .. "]" ..

		"button[2,4.5;8,0.5;mg_preset_new;" .. fgettext("New preset with current parameters") .. "]" ..

		"button[3.25,6;2.5,0.5;mg_presets_confirm;" .. fgettext("Save") .. "]" ..
		"button[5.75,6;2.5,0.5;mg_presets_cancel;" .. fgettext("Cancel") .. "]"
		
	return retval

end

local function mapgen_params_buttonhandler(this, fields)
	if fields["mg_presets_confirm"] then
		-- To be implemented
		this:delete()
		return true
	end

	if fields["mg_presets_cancel"] then
		this:delete()
		return true
	end

	if fields["mg_preset_new"] then
		local mg_preset_dlg = create_mg_preset_edit_dlg(mgname)
		mg_preset_dlg:set_parent(this)
		this:hide()
		mg_preset_dlg:show()
		return true
	end

	return false
end


function create_mapgen_params_dlg(arg_mgname)
	local retval = dialog_create("sp_mapgen_params",
					mapgen_params_formspec,
					mapgen_params_buttonhandler,
					nil)
	mgname = arg_mgname
	
	return retval
end
