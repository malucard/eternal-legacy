bit = {
	band = function(a, b) return a & b end,
	bor = function(a, b) return a | b end,
	bxor = function(a, b) return a ~ b end
}

unpack = table.unpack