
multinode = {}

local function get_other_relative_positions(node, multinode_def)
	local offset = vector.new()
	for _, d in ipairs(multinode_def) do
		if d.name == node.name then
			offset = d.offset
			break
		end
	end
	local y_vec = core.facedir_to_top_dir(node.param2)
	local z_vec = core.facedir_to_dir(node.param2)
	local x_vec = vector.cross(y_vec, z_vec)
	local new_def = {}
	for i, d in ipairs(multinode_def) do
		local v = vector.subtract(d.offset, offset)
		new_def[i] = {name = d.name,
			offset = {x = v.x * x_vec.x + v.y * y_vec.x + v.z * z_vec.x,
			          y = v.x * x_vec.y + v.y * y_vec.y + v.z * z_vec.y,
			          z = v.x * x_vec.z + v.y * y_vec.z + v.z * z_vec.z}}
	end
	return new_def
end

function multinode.on_construct(multinode_def)
	return function(pos)
		local node = core.get_node(pos)
		local def = get_other_relative_positions(node, multinode_def)
		for _, d in ipairs(def) do
			local p = vector.add(pos, d.offset)
			local n = core.get_node_or_nil(p)
			local ndef = n and core.registered_items[n.name]
			local buildable_to = ndef and ndef.buildable_to
			if not n or ((n.name ~= d.name or n.param2 ~= node.param2) and not buildable_to) then
				multinode.destruct_raw(pos)
				return
			end
		end
		-- All nodes can be placed
		for _, d in ipairs(def) do
			local p = vector.add(pos, d.offset)
			local n = core.get_node(p)
			if n.name ~= d.name or n.param2 ~= node.param2 then
				core.set_node(p, {name = d.name, param2 = node.param2})
			end
		end
	end
end

function multinode.destruct_raw(pos)
	local meta = core.get_meta(pos)
	meta:set_int("multinode_destruct", 1)
	core.remove_node(pos)
end

function multinode.on_destruct(multinode_def)
	return function(pos)
		local node = core.get_node(pos)
		local meta = core.get_meta(pos)
		if meta:get_int("multinode_destruct") > 0 then
			return
		end
		local def = get_other_relative_positions(node, multinode_def)
		for _, d in ipairs(def) do
			local p = vector.add(pos, d.offset)
			local n = core.get_node_or_nil(p)
			if n and n.name == d.name and n.param2 == node.param2 and not vector.equals(p, pos) then
				multinode.destruct_raw(p)
			end
		end
	end
end

function multinode.on_rotate(multinode_def)
	return function(pos, node, user, mode, new_param2)
		local node = core.get_node(pos)
		local meta = core.get_meta(pos)
		local def = get_other_relative_positions(node, multinode_def)
		local n2 = {name = node.name, param2 = new_param2}
		local newdef = get_other_relative_positions(n2, multinode_def)
		local used = {}
		for _, d in ipairs(def) do
			local p = vector.add(pos, d.offset)
			local n = core.get_node_or_nil(p)
			if n and n.name == d.name and n.param2 == node.param2 then
				used[core.hash_node_position(d.offset)] = true
			end
		end
		for _, d in ipairs(newdef) do
			local p = vector.add(pos, d.offset)
			local n = core.get_node_or_nil(p)
			local ndef = n and core.registered_items[n.name]
			local buildable_to = ndef and ndef.buildable_to
			if not n or ((n.name ~= d.name or n.param2 ~= node.param2) and not buildable_to
					and not used[core.hash_node_position(d.offset)]) then
				return false
			end
		end
		for _, d in ipairs(def) do -- Destroy old node
			local p = vector.add(pos, d.offset)
			local n = core.get_node_or_nil(p)
			if n and n.name == d.name and n.param2 == node.param2 and not vector.equals(p, pos) then
				multinode.destruct_raw(p)
			end
		end
		core.swap_node(pos, n2)
		for _, d in ipairs(newdef) do -- And place new one
			local p = vector.add(pos, d.offset)
			local n = core.get_node(p)
			if n.name ~= d.name or n.param2 ~= node.param2 then
				core.set_node(p, {name = d.name, param2 = new_param2})
			end
		end
	end
end

function multinode.after_place_node(pos)
	return core.get_node(pos).name == "air" -- If the node was removed by on_construct, give the item back
end
