-- test command

assert(..., "no path specified")
local path, out = ...
local h = assert(fs_file(path), "could not open file at " .. path)