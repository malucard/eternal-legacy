local function class_new(self, ...)
	local instance = {}
	for i, v in pairs(self) do
		--[[if type(i) ~= "string" or i:sub(1, 1) ~= "_priv_" then
			instance[i] = v
		end]]
		instance[i] = v
	end
	local init = self._init
	if init then
		local res = init(instance, ...)
		if res ~= nil then
			return res
		end
	end
	return instance
end

local function class_instance_of(self, c)
	return self._class == c or self._class_parent.instance_of(self, c)
end

function class(content)
	if content._class then
		return function(child_content)
			local keys = {}
			for i in pairs(child_content) do
				keys[i] = true
			end
			for i, v in pairs(content) do
				if not keys[i] and i ~= "_class" then
					child_content[i] = v
				end
			end
			child_content._class_parent = content
			return class(child_content)
		end
	end
	content._class = content
	setmetatable(content, {__call = class_new})
	--content.new = class_new
	--content.instance_of = class_instance_of
	return content
end
