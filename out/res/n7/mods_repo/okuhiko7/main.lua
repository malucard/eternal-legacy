vn_add_hook("message", function(data)
	if data.speaker.id ~= "okuhiko" then
		data.skip()
	end
end)
