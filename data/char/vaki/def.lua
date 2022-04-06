--Tells the engine what files it should use.

graphics = {
	images = { --List of non chunk images
		"effect/stars.lzs3",
		"effect/circle.lzs3"
	},
	{ --Chunk images start.
		type = lzs3, --Probably should be removed. It can autodetect.
		image = "vaki8.lzs3",
		vertex = "vaki.vt1",
	},
	{
		type = lzs3,
		image = "vaki32.lzs3",
		vertex = "vaki.vt4",
	}
}

palettes = "palettes.pal4"