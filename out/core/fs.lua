function fs_resolve(path)
	local game_root
	for i = 2, 100 do
		--local res, env = pcall(getfenv, i)
		local env = getfenv(i)
		if env then
			game_root = env.GAME_ROOT
			if game_root then
				break
			end
		else
			game_root = CUR_GAME and CUR_GAME.GAME_ROOT or ""
			break
		end
	end
	if path:sub(1, 4) == "et:/" then
		return (ROOT_DIR .. path:sub(5)):gsub("///", "/"):gsub("//", "/")
	elseif path:sub(1, 5) == "pkg:/" then
		return (ROOT_DIR .. game_root .. path:sub(6)):gsub("///", "/"):gsub("//", "/")
	else
		local limit = path:find(":[^/:]", 6)
		if fs_exists(limit and path:sub(1, limit - 1) or path, true) then
			return path
		else
			local p = (ROOT_DIR .. path):gsub("///", "/"):gsub("//", "/")
			limit = p:find(":[^/:]", 6)
			if fs_exists(limit and p:sub(1, limit - 1) or p, true) then
				return p
			else
				return (ROOT_DIR .. game_root .. path):gsub("///", "/"):gsub("//", "/")
			end
		end
	end
end

-- we proceed to do a little trickery to make lua built in functions call fs_resolve
-- this is necessary because while on PC we have core/ and others in the CWD,
-- on game consoles we don't and generally need full paths
-- lua doesn't have another way to fix this

local ogloadfile = loadfile
local ogdofile = dofile
function dofile(fn, ...)
	--local game_env = getfenv(2)
	return assert(ogloadfile(fs_resolve(fn), 'bt'))(...)
end

function loadfile(fn, ...)
	return ogloadfile(fs_resolve(fn), ...)
end
