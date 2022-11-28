local semver_meta
local subver_letter_offset = string.byte("a") - 1
function semver_parse(str)
	local semver = {}
	local dash = str:find("-")
	if dash then
		local subver = str:sub(dash + 1)
		if not subver:match("^[a-z]+[0-9]*$") then
			log_err("error parsing semver '" .. str ..  "': can only have lowercase letters and a number after dash")
		end
		semver.subver_letter = str:sub(dash + 1, dash + 1):byte() - subver_letter_offset
		semver.subver_number = str:match("[0-9]+", dash + 1) or 1
		str = str:sub(1, dash - 1)
	end
	local nums = str:split("%.")
	for i = 1, #nums do
		nums[i] = tonumber(nums[i])
		if not nums[i] then
			log_err("error parsing semver '" .. str ..  "': can only have numbers and dots before dash")
		end
	end
	semver.nums = nums
	return setmetatable(semver, semver_meta)
end

-- -1 is a < b, 0 is a == b, 1 is a > b
function semver_cmp(a, b)
	a = type(a) == "string" and semver_parse(a) or a
	b = type(b) == "string" and semver_parse(b) or b
	local anumcount = #a.nums
	local bnumcount = #b.nums
	if anumcount > bnumcount then
		return 1
	elseif anumcount < bnumcount then
		return -1
	end
	for i = 1, anumcount do
		local anum = a.nums[i]
		local bnum = b.nums[i]
		if anum > bnum then
			return 1
		elseif anum < bnum then
			return -1
		end
	end
	if a.subver_letter then
		if b.subver_letter then
			if a.subver_letter > b.subver_letter then
				return 1
			elseif a.subver_letter < b.subver_letter then
				return -1
			elseif a.subver_number > b.subver_number then
				return 1
			elseif a.subver_number < b.subver_number then
				return -1
			else
				return 0
			end
		else
			return -1
		end
	else
		if b.subver_letter then
			return 1
		else
			return 0
		end
	end
end

semver_meta = {
	__eq = function(a, b)
		return semver_cmp(a, b) == 0
	end,
	__ne = function(a, b)
		return semver_cmp(a, b) ~= 0
	end,
	__lt = function(a, b)
		return semver_cmp(a, b) == -1
	end,
	__le = function(a, b)
		return semver_cmp(a, b) ~= 1
	end,
	__gt = function(a, b)
		return semver_cmp(a, b) == 1
	end,
	__ge = function(a, b)
		return semver_cmp(a, b) ~= -1
	end
}