PackageSmallView = class(EuiLayout) {
	horizontal = false;

	_init = function(self, url)
		local index = dofile(url .. "index.lua")
		self[1] = EuiLayout {
			min_h = 64;
			expand = true;
			horizontal = true;
			EuiTexView {
				min_w = 128;
				src = url .. "icon.png";
			};
			EuiLayout {
				expand = true;
				EuiLabel {
					text = index.name;
					font_size = 16;
				};
				EuiLabel {
					text = index.desc;
					font_size = 16;
				};
			};
			EuiButton {
				text = "Install (Default)";
			};
		}
		self[2] = EuiSeparator {}
		EuiLayout._init(self, self)
	end;
}