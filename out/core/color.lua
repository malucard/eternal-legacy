-- included by main.lua

local color_meta
color_meta = {
	__add = function(a, b)
		return setmetatable({a[1] + b[1], a[2] + b[2], a[3] + b[3], a[4] + b[4]}, color_meta)
	end,
	__sub = function(a, b)
		return setmetatable({a[1] - b[1], a[2] - b[2], a[3] - b[3], a[4] - b[4]}, color_meta)
	end,
	__mul = function(a, b)
		if type(b) == "table" then
			return setmetatable({a[1] * b[1], a[2] * b[2], a[3] * b[3], a[4] * b[4]}, color_meta)
		end
		return setmetatable({a[1] * b, a[2] * b, a[3] * b, a[4] * b}, color_meta)
	end
}

function color(r, g, b, a)
	return setmetatable({r, g, b, a}, color_meta)
end