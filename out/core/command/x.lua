-- extract command

assert(..., "no path specified")
local path, out = ...
local h = assert(fs_archive(path), "could not open archive at " .. path)
local list = h:list_files()
if out then
	out = out:gsub("\\", "/"):gsub("//", "/")
	if out:sub(-1, -1) ~= "/" then
		out = out .. "/"
	end
	for i = 1, #list do
		local v = list[i]
		h:select(v)
		log_info("extracting " .. v)
		v = out .. v:gsub("\\", "/"):gsub("[A-Za-z0-9]*:/", "")
		local _, last_slash = v:find(".+/")
		log_info(v:sub(1, last_slash - 1))
		fs_mkdir(v:sub(1, last_slash - 1))
		local oh = io.open(v, "wb")
		local txt = h:read_string()
		oh:write(txt)
		oh:close()
		log_info(h:filename())
	end
else
	log_info("files in archive: " .. table.concat(list, ", "))
end