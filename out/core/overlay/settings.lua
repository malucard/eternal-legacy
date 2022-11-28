function eui_build(params)
	local ty = params[1]
	local elem
	if ty == "layout" then
		local has_window_bg, scrollable = params.has_window_bg, params.scrollable
		elem = EuiLayout(params.horizontal, has_window_bg and not scrollable)
		local len = #params
		for i = 2, len do
			elem:add_child(eui_build(params[i]))
		end
		if scrollable then
			local scrollable = EuiScrollable(has_window_bg)
			scrollable.elem = elem
			elem = scrollable
		end
	elseif ty == "label" then
		elem = EuiLabel(params[2])
	elseif ty == "button" then
		elem = EuiButton(params[2], params[3])
	elseif ty == "toggle" then
		elem = EuiToggle(params[2], params[3], params.activated)
	elseif ty == "separator" then
		elem = EuiSeparator()
	end
	local id = params.id
	if id then
		elem.id = id
	end
	local expand = params.expand
	if expand then
		elem.expand = true
	end
	return elem
end
settings_menu = eui_build {"layout"; has_window_bg = true; expand = true;
	{"label"; "Settings"};
	{"separator"};
	{"layout"; horizontal = true; expand = true;
		{"layout";
			{"button"; "Mods"; function()
			end};
			{"separator"};
			{"button"; "Packages"; function()
			end};
			{"button"; "Languages"; function()
			end};
		};
		{"separator"};
		{"layout"; expand = true;
			{"layout"; expand = true;
				{"layout"; horizontal = true;
					{"button"; "↑"; function()
					end};
					{"button"; "↓"; function()
					end};
					{"label"; "English"};
				};
				{"separator"};
				{"layout"; horizontal = true;
					{"button"; "↑"; function()
					end};
					{"button"; "↓"; function()
					end};
					{"label"; "Japanese"};
				};
				{"separator"};
				{"layout"; horizontal = true;
					{"button"; "↑"; function()
					end};
					{"button"; "↓"; function()
					end};
					{"label"; "Português"};
				};
			};
			{"layout";
				{"toggle"; "Prefer system language"; function()
				end};
			};
		};
	};
}

function draw_settings_menu()
	settings_menu:draw(0, 0, g_width, g_height)
end