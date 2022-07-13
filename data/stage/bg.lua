stage = {
	scale = 0.5,
	width = 600,
	height = 450,
	layers = {
		{ --Lowermost layer
			y = 160,
			xParallax = 0.0,
			yParallax = 0.7,
			xScroll = 1.0,
			elements = {
				{id = 2}
			}
		},
		{
			xParallax = 0.6,
			yParallax = 0.9,
			elements = {
				{id = 1}
			}
		},
		{
			y = 32,
			xParallax = 0.8,
			yParallax = 1.0,
			mode = additive,
			elements = {
				{	id = 4; x = 456, y = 225;
					movement = {type = vertical; centerY=225; speedY = 1, accelY = 0.01;}
				},
				{	id = 5, x = 120, y = 280,
					movement = {type = vertical | horizontal; centerX = 200, centerY=265; accelX = 0.001, accelY = 0.01;}
				},
				{	id = 6, x = 800, y = 320,
					movement = {type = vertical | horizontal; centerX = 770, centerY=295; accelX = 0.001, accelY = 0.01;}
				},
				{	id = 7, x = 1000, y = 440,
					movement = {type = vertical; centerY=350; accelY = 0.01;}
				}
			}
		},
 		{ --Uppermost layer.
			xParallax = 1,
			elements = {
				{id = 0}
			}
		}
	}
}

graphics = {
	{
		image = "bg.lzs3",
		vertex = "bg.vt4",
		filter = true,
	}
}