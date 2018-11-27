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

local mgpresets_path = core.get_builtin_path() .. DIR_DELIM .. "mapgen_presets"

local function load_preset_list()
	local preset_filenames = core.get_dir_list(mgpresets_path, false)
	local presets = {}
	for _, filename in ipairs(preset_filenames) do
		local path = mgpresets_path .. DIR_DELIM .. filename
		local settings = Settings(path)
		local name = settings:get("mg_preset")
		presets[name] = {
			path = path,
			mapgen = settings:get("mg_name")
		}
		print("Registering the preset " .. name .. " located at " .. path)
	end
	return presets
end

local preset_list = load_preset_list()

local function mapgen_params_formspec(data)
	local mgname = data.mgname
	local only_selected_mapgen = core.settings:get("mg_preset_only_selected_mapgen")

	local preset_text_list = ""
	local presetname = core.settings:get("mg_preset")
	local i = 0
	local selected_preset = 1
	for name, preset in pairs(preset_list) do
		if not only_selected_mapgen or preset.mapgen == mgname then
			i = i + 1
			if name == presetname then
				selected_preset = i
			end
			preset_text_list = preset_text_list .. name .. ","
		end
	end
	preset_text_list = preset_text_list:sub(1,-2) -- Remove the ending comma

	local retval =
		"size[11.5,6,true]" ..
		"label[2,0;" .. fgettext("Mapgen name") .. "]"..
		"label[4.2,0;" .. mgname .. "]" ..

		"checkbox[2,1;only_selected_mapgen;" .. fgettext("Show only presets of current mapgen") .. ";" .. tostring(only_selected_mapgen) .. "]" ..
		"label[2,2;" .. fgettext("Parameters preset") .. "]" ..
		"textlist[4.2,2;5.8,2.3;presets;" .. preset_text_list .. ";" .. selected_preset .. ";false]" ..

		"button[3.25,5;2.5,0.5;mapgen_params_confirm;" .. fgettext("Save") .. "]" ..
		"button[5.75,5;2.5,0.5;mapgen_params_cancel;" .. fgettext("Cancel") .. "]"
		
	return retval

end

local function mapgen_params_buttonhandler(this, fields)
	if fields["mapgen_params_confirm"] then
		-- To be implemented
		this:delete()
		return true
	end

	if fields["mapgen_params_cancel"] then
		this:delete()
		return true
	end

	return false
end


function create_mapgen_params_dlg(mgname)
	local retval = dialog_create("sp_mapgen_params",
					mapgen_params_formspec,
					mapgen_params_buttonhandler,
					nil)
	retval.data.mgname = mgname
	
	return retval
end
