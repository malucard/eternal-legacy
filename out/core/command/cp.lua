-- copy command

assert(..., "no path specified")
local path, out = ...
local h = assert(fs_file(path), "could not open file at " .. path)
local oh = assert(io.open(out, "wb"), "could not open output file at " .. out)
local txt = h:read_string()
oh:write(txt)
oh:close()
log_info(h:filename())