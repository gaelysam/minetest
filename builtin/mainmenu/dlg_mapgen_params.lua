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

local function mapgen_params_formspec(data)
	local mgname = data.mgname
	local preset_list = "Original,Voxelproof,Wider,Custom"

	local retval =
		"size[11.5,4.5,true]" ..
		"label[2,0;" .. fgettext("Mapgen name") .. "]"..
		"label[4.2,0;" .. mgname .. "]" ..

		"label[2,1;" .. fgettext("Parameters preset") .. "]" ..
		"textlist[4.2,1;5.8,2.3;presets;" .. preset_list .. ";" .. "1" .. ";false]" ..

		"button[3.25,4;2.5,0.5;mapgen_params_confirm;" .. fgettext("Save") .. "]" ..
		"button[5.75,4;2.5,0.5;mapgen_params_cancel;" .. fgettext("Cancel") .. "]"
		
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
