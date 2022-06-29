_states = {
	stand = 0,
	crouch = 1,
	air = 2
}

_seqTable = {
	hurtHi1 = 900,
	hurtHi2 = 903,
	hurtHi3 = 906,
	hurtLo1 = 901,
	hurtLo2 = 904,
	hurtLo3 = 907,
	croHurt1 = 902,
	croHurt2 = 905,
	croHurt3 = 908,
	trip = 29,
	down = 26,
	airDown = 30,
	airGuard = 19,
	guard = 17,
	croGuard = 18
}

_vectors = {
	hurtHi1 = { xSpeed = 900; xAccel = -70; ani = "hurtHi1"},
	hurtHi2 = { xSpeed = 1200; xAccel = -75; ani = "hurtHi2"},
	hurtHi3 = { xSpeed = 1600; xAccel = -80; ani = "hurtHi3"},
	hurtLo1 = { xSpeed = 900; xAccel = -70; ani = "hurtLo1"},
	hurtLo2 = { xSpeed = 1200; xAccel = -75; ani = "hurtLo2"},
	hurtLo3 = { xSpeed = 1600; xAccel = -80; ani = "hurtLo3"},
	croHurt1 = { xSpeed = 900; xAccel = -70; ani = "croHurt1"},
	croHurt2 = { xSpeed = 1200; xAccel = -75; ani = "croHurt2"},
	croHurt3 = { xSpeed = 1600; xAccel = -80; ani = "croHurt3"},
	block1 = { xSpeed = 900; xAccel = -70; ani = "guard"},
	block2 = { xSpeed = 1200; xAccel = -75; ani = "guard"},
	block3 = { xSpeed = 1600; xAccel = -80; ani = "guard"},
	croBlock1 = { xSpeed = 900; xAccel = -70; ani = "croGuard"},
	croBlock2 = { xSpeed = 1200; xAccel = -75; ani = "croGuard"},
	croBlock3 = { xSpeed = 1600; xAccel = -80; ani = "croGuard"},
	airBlock = { xSpeed = 1000, ySpeed = 300; yAccel = -150; ani = "airGuard"},
	jumpIn = {xSpeed = 800, xAccel = -40;ani = "hurtLo3"},
	jumpInBlock = {xSpeed = 800, xAccel = -40;ani = "guard"},
	airHit = {
		xSpeed = 800, ySpeed = 1700;
		xAccel = 0, yAccel = -150;
		maxTime = 8,
		ani = "down"
	},
	launch = {
		xSpeed = 200, ySpeed = 2700;
		xAccel = 0, yAccel = -150;
		maxTime = 8,
		ani = "airDown"
	},
	down = {
		xSpeed = 250, ySpeed = 2200;
		xAccel = 0, yAccel = -150;
		maxTime = 4,
		ani = "down"
	},
	trip = {
		xSpeed = 100, ySpeed = 1200;
		xAccel = 0, yAccel = -150;
		maxTime = 4,
		ani = "trip"
	},
	slam = {
		xSpeed = 2800, ySpeed = -2500;
		xAccel = 0, yAccel = -150;
		maxTime = 4,
		ani = "down",
		onBounce = "slamBounce"
	},
	slamBounce = {
		xSpeed = 700, ySpeed = 2800;
		xAccel = 0, yAccel = -150;
		maxTime = 0,
		ani = "airDown",
	},
	wallSlam = {
		xSpeed = 2800,
		maxTime = 0,
		ani = "down",
		onBounce = "wallSlamBounce"
	},
	wallSlamBounce = {
		xSpeed = -400, ySpeed = 2200;
		xAccel = 0, yAccel = -150;
		maxTime = 0,
		ani = "down",
	}
	
}

histopTbl = {
	weakest = 4,
	weak = 6,
	medium = 8,
	strong = 10,
	stronger = 15,
	strongest = 30,
}

local key = constant.key
local g = global
local s = _states
local v = _vectors
a_flag = {
	floorCheck = 0x1,
	followParent = 0x2,
}

local function runFrameTable(actor, table)
	local func = table[actor.currentFrame]
	if(func and actor.subframeCount == 0) then
		func(actor)
	end
end

function C_toCrouch()
	local crouchingSeq = player.currentSequence;
	if(crouchingSeq == 13 or crouchingSeq == 16 or crouchingSeq == 18) then
		return false
	end
	return true
end

function C_heightRestriction()
	local x, y = player:GetPos()
	return (y>>16) >= 70
end

function C_notHeldFromJump()
	local cs = player.currentSequence;
	if(cs >= 35 and cs <= 40 and (g.GetInputPrev() & key.up)~=0) then
		return false 
	end
	return true
end

function Actor:GroundLevel()
	local x,y = self:GetPos()
	if(y < 32<<16) then
		y = 32<<16
	end
	self:SetPos(x,y)
end

function Actor:SetPosRelTo(actor, x, y)
	local ax,ay = actor:GetPos()
	self:SetPos(ax+(x<<16)*actor:GetSide(), ay+(y<<16))
end

function A_spawnPosRel(actor, seq, x, y, flags, side)
	x = x or 0
	y = y or 0
	flags = flags or 0
	side = side or actor:GetSide()
	local ball = player:SpawnChild(seq)
	local xA, yA = actor:GetPos()
	x = xA + (x << 16) * actor:GetSide()
	y = yA + (y << 16)
	ball:SetPos(x,y)
	ball:SetSide(side)
	ball.flags = flags
	return ball
end

function hurtSound(actor)
	if(actor.totalSubframeCount == 0) then
		local chance = g.Random(0,1)
		if(chance < 0.5) then
			local str = "vaki/hurt"..g.RandomInt(1,10)
			g.PlaySound(str)
		end
	end
	
	if actor.subframeCount == 0 then
		if ((actor.currentSequence == 26 and actor.currentFrame == 7) or
			(actor.currentSequence == 29 and actor.currentFrame == 5) or
			(actor.currentSequence == 30 and actor.currentFrame == 11)
			) then
			global.PlaySound("bounce")
		end
	end
end

--Standing only
function turnAround()
	g.TurnAround(15)
end

function walk(actor)
	if(g.TurnAround(15)) then
		return
	end
	if((g.GetInput() & key.any) == 0) then
		actor:GotoSequence(0)
	end
end

function crouch(actor)
	g.TurnAround(16)
	if((g.GetInput() & key.down) == 0) then
		actor:GotoSequence(14)
	end
end

function doubleJump(actor)
	if actor.totalSubframeCount == 0 then
		if actor.currentSequence == 38 then --fwd
			global.TurnAround(40)
		elseif actor.currentSequence == 40 then --bak
			global.TurnAround(38)
		end
	end
end

function slide1(actor) --First neutral jump
	local frame = actor.currentFrame
	if(frame > 5 and frame < 12) then
		local x, y = actor:GetVel()
		local input = g.GetInput()
		if(input & key.right ~= 0) then
			x = x + 5000
		elseif(input & key.left ~= 0) then
			x = x - 5000
		end
		actor:SetVel(x,y)
	end
end

function neutralDoubleJump(actor)
	local frame = actor.currentFrame
	if (actor.totalSubframeCount == 0) then
		global.TurnAround(-1)
	
	--slide
	elseif(frame > 2 and frame < 7) then
		local x, y = actor:GetVel()
		local input = global.GetInput()
		if(input & key.right ~= 0) then
			x = x + 7000
		elseif(input & key.left ~= 0) then
			x = x - 7000
		end
		actor:SetVel(x,y)
	end
end

function speedLim(actor)
	local x, y = actor:GetVel()
	if(actor:GetSide()*x > 1000*constant.multiplier) then
		actor:SetVel(1000*constant.multiplier*actor:GetSide(), y)
	end
end

function blocking(actor)
	--[[
	if(actor.totalSubframeCount == 0) then
		actor:GotoFrame(2)
	end --]]--
	if(global.GetBlockTime() > 0) then
		actor:GotoFrame(2)
	end
end

function blockingAir(actor)
	if(actor.totalSubframeCount == 0) then
		global.SetBlockTime(0)
	end
end

--------------------- Normals -------------------------
function weakHit(hitdef, lo)
	if(lo) then
		hitdef:SetVectors(s.stand, v.hurtLo1, v.block1)
	else
		hitdef:SetVectors(s.stand, v.hurtHi1, v.block1)
	end
	hitdef:SetVectors(s.air, v.airHit, v.airBlock)
	hitdef:SetVectors(s.crouch, v.croHurt1, v.croBlock1)
end

function mediumHit(hitdef, lo)
	if(lo) then
		hitdef:SetVectors(s.stand, v.hurtLo2, v.block2)
	else
		hitdef:SetVectors(s.stand, v.hurtHi2, v.block2)
	end
	hitdef:SetVectors(s.air, v.airHit, v.airBlock)
	hitdef:SetVectors(s.crouch, v.croHurt2, v.croBlock2)
end

function strongHit(hitdef, lo)
	if(lo) then
		hitdef:SetVectors(s.stand, v.hurtLo3, v.block3)
	else
		hitdef:SetVectors(s.stand, v.hurtHi3, v.block3)
	end
	hitdef:SetVectors(s.air, v.airHit, v.airBlock)
	hitdef:SetVectors(s.crouch, v.croHurt3, v.croBlock3)
end

function s5a (actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		weakHit(hitdef)
		hitdef.hitStop = histopTbl.weak
		hitdef.blockStun = 14
		hitdef.damage = 300
		hitdef.attackFlags = g.hit.hitsAir
		hitdef.sound = "punchWeak"
	end
end

function s5b (actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		mediumHit(hitdef, true)
		hitdef.hitStop = histopTbl.medium
		hitdef.blockStun = 17
		hitdef.damage = 700
		hitdef.attackFlags = g.hit.hitsAir
		hitdef.sound = "kickMedium"
	end
end

function s5c (actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		strongHit(hitdef)
		hitdef.hitStop = histopTbl.strong
		hitdef.blockStun = 20
		hitdef.damage = 1400
		hitdef.shakeTime = 12
		hitdef.attackFlags = g.hit.hitsAir
		hitdef.sound = "kickStrong"
	elseif(actor.currentFrame == 27 and actor.subframeCount == 0) then
		local chance = g.Random(0,1)
		if(chance < 0.75) then
			g.PlaySound("vaki/076")
		end
	end
end

function s2a (actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		weakHit(hitdef)
		hitdef.hitStop = histopTbl.weak
		hitdef.blockStun = 14
		hitdef.damage = 350
		hitdef.attackFlags = g.hit.hitsStand
		hitdef.sound = "punchWeak"
	elseif(actor.currentFrame == 1 and actor.subframeCount == 0) then
		A_spawnPosRel(actor, 113)
	end
end

function s2b (actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		weakHit(hitdef, true)
		hitdef.hitStop = histopTbl.weakest
		hitdef.blockStun = 14
		hitdef.damage = 250
		hitdef.sound = "slash"
	elseif(actor.currentFrame == 3 and actor.subframeCount == 0) then
		A_spawnPosRel(actor, 104)
	end
end

function s2c (actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		hitdef:SetVectors(s.stand, v.trip, v.block3)
		hitdef:SetVectors(s.air, v.trip, v.airBlock)
		hitdef:SetVectors(s.crouch, v.trip, v.croBlock3)
		hitdef.hitStop = histopTbl.strong
		hitdef.blockStun = 20
		hitdef.damage = 1200
		hitdef.shakeTime = 12
		hitdef.attackFlags = g.hit.hitsStand | g.hit.hitsAir
		hitdef.sound = "kickStrong"
	end
end

function s4c (actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		hitdef:SetVectors(s.stand, v.down, v.block3)
		hitdef:SetVectors(s.air, v.down, v.airBlock)
		hitdef:SetVectors(s.crouch, v.down, v.croBlock3)
		hitdef.hitStop = histopTbl.medium
		hitdef.blockStun = 17
		hitdef.damage = 500
		hitdef.shakeTime = 12
		hitdef.attackFlags = g.hit.hitsAir
		hitdef.sound = "punchStrong"
	elseif actor.currentFrame == 3 and actor.subframeCount == 0 then
		A_spawnPosRel(actor, 110, 0, 8)
	end
end

function s6c (actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		strongHit(hitdef, true)
		hitdef.hitStop = histopTbl.strong
		hitdef.damage = 1700
		hitdef.blockStun = 20
		hitdef.shakeTime = 15
		hitdef.attackFlags = global.hit.hitsCrouch
		hitdef.sound = "kickStrong"
	end
end

function sja (actor)
	speedLim(actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		hitdef:SetVectors(s.stand, v.hurtHi1, v.block1)
		hitdef:SetVectors(s.air, v.airHit, v.airBlock)
		hitdef:SetVectors(s.crouch, v.airHit, v.croBlock1)
		hitdef.hitStop = histopTbl.weak
		hitdef.blockStun = 14
		hitdef.damage = 300
		hitdef.sound = "punchWeak"
	end
end

function sjb (actor)
	speedLim(actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		hitdef:SetVectors(s.stand, v.hurtHi2, v.block2)
		hitdef:SetVectors(s.air, v.airHit, v.airBlock)
		hitdef:SetVectors(s.crouch, v.airHit, v.croBlock2)
		hitdef.hitStop = histopTbl.medium
		hitdef.blockStun = 17
		hitdef.damage = 700
		hitdef.sound = "kickMedium"
	end
end

function sjc (actor)
	speedLim(actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		hitdef:SetVectors(s.stand, v.jumpIn, v.jumpInBlock)
		hitdef:SetVectors(s.air, v.down, v.airBlock)
		hitdef:SetVectors(s.crouch, v.jumpIn, v.jumpInBlock)
		hitdef.hitStop = histopTbl.strong
		hitdef.blockStun = 20
		hitdef.damage = 1000
		hitdef.shakeTime = 12
		hitdef.attackFlags = g.hit.hitsCrouch
		hitdef.sound = "punchStrong"
	elseif(actor.currentFrame == 4 and actor.subframeCount == 0) then
		A_spawnPosRel(actor, 138,0,0,a_flag.followParent):Attach(actor)
		A_spawnPosRel(actor, 139,0,0,a_flag.followParent):Attach(actor)
	end
end

function s3c (actor)
	if(actor.totalSubframeCount == 0) then
		local hitdef = actor.hitDef
		hitdef:SetVectors(s.stand, v.launch, v.block3)
		hitdef:SetVectors(s.air, v.launch, v.airBlock)
		hitdef:SetVectors(s.crouch, v.launch, v.croBlock3)
		hitdef.hitStop = histopTbl.medium
		hitdef.blockStun = 20
		hitdef.damage = 700
		hitdef.shakeTime = 8
		hitdef.sound = "punchStrong"
	elseif(actor.currentFrame == 4 and actor.subframeCount == 0) then
		A_spawnPosRel(actor, 102)
	end
end

--------------------- Other --------------------------
local t_sgthrow = {
	[0] = function (actor)
		local hitdef = actor.hitDef
		hitdef:SetVectors(s.stand, v.wallSlam, v.block3)
		hitdef:SetVectors(s.air, v.wallSlam, v.airBlock)
		hitdef:SetVectors(s.crouch, v.wallSlam, v.croBlock3)
		hitdef.hitStop = histopTbl.strong
		hitdef.blockStun = 20
		hitdef.damage = 700
		hitdef.shakeTime = 4
		hitdef.attackFlags = g.hit.wallBounce | g.hit.hitsAir | g.hit.disableCollision
		hitdef.sound = "slash"
	end,
	[3] = function (actor)
		if(actor:ThrowCheck(g.GetTarget(), 50, 0 ,0)) then
			actor.userData.t = g.GetTarget()
			if((g.GetInputRelative() & key.left) ~= 0) then
				actor:SetSide(-actor:GetSide())
				g.GetTarget():SetSide(-g.GetTarget():GetSide())
			end
			g.SetPriority(1)
			actor:GotoFrame(13)
			g.PlaySound("kickWeak")
			
			local enemy = actor.userData.t
			enemy:GotoSequence(350)
			enemy.frozen = true
			
			local x,y = actor:GetPos()
			enemy:SetPos(x+(26<<16)*actor:GetSide(), y)
		end
	end,
	[13] = function (actor) --TODO: Fix on engine side???
		print("fuck")
	end,
	[19] = function (actor)
		A_spawnPosRel(actor, 64, 43, 86)
		global.PlaySound("slash")
	end,
	[20] = function (actor)
		local enemy = actor.userData.t
		local x,y = actor:GetPos()
		enemy:SetPos(x+(41<<16)*actor:GetSide(), (89-40)<<16)
		global.ParticlesNormalRel(10, 45, 86)
		global.DamageTarget(170)
	end,
	[21] = function (actor)
		local enemy = actor.userData.t
		local x,y = actor:GetPos()
		enemy:SetPos(x+(43<<16)*actor:GetSide(), (89-40)<<16)
	end,
	[22] = function (actor)
		local enemy = actor.userData.t
		local x,y = actor:GetPos()
		enemy:SetPos(x+(41<<16)*actor:GetSide(), (89-40)<<16)
		enemy.frozen = false
		enemy:SetVector(_vectors.trip, actor:GetSide())
		enemy:GotoSequence(29)
	end
}
function sgthrow(actor)
	runFrameTable(actor, t_sgthrow)
	
	--Grabbed one follows the arm
	if actor.currentFrame >= 14 and actor.currentFrame <= 16 and actor.subframeCount == 0 then
		local enemy = actor.userData.t
		local x,y = actor:GetPos()
		local offset = actor.currentFrame-14
		enemy:SetPos(
			x+((42 + offset)<<16)*actor:GetSide(),
			(88-40 -offset)<<16
		)
	end
end

local t_sathrow_pre = {
	[0] = function(actor)
		speedLim(actor)
	end,
	[1] = function(actor)
		if(actor:ThrowCheck(global.GetTarget(), 60, 120, -30)) then
			actor.userData.t = global.GetTarget()
			actor:GotoSequence(272)
			global.SetPriority(1)
			global.PlaySound("kickWeak")
		end
	end
}
function sathrow_pre(actor)
	runFrameTable(actor, t_sathrow_pre)
end



local t_sathrow = {
	[1] = function(actor)
		local enemy = actor.userData.t
		enemy.frozen = true
		enemy:Attach(actor)
		enemy:SetPosRelTo(actor, 27, 79-45)
		enemy:GotoSequence(350)
		enemy:SetSide(-actor:GetSide())
	end,
	[2] = function(actor)
		local enemy = actor.userData.t
		enemy:SetPosRelTo(actor, 27, 80-45)
		enemy:RotateZP(20,0,60)
	end,
	[3] = function(actor)
		local enemy = actor.userData.t
		enemy:SetPosRelTo(actor, 35, 70-45)
		enemy:RotateZP(30,0,60)
	end,
	[4] = function(actor)
		local enemy = actor.userData.t
		enemy:SetPosRelTo(actor, 32, 57-45)
		enemy:RotateZP(55,0,60)
	end,
	[5] = function(actor)
		local enemy = actor.userData.t
		enemy:SetPosRelTo(actor, 22, 30-45)
		enemy:RotateZP(70,0,60)
	end,
	[6] = function(actor)
		local enemy = actor.userData.t
		enemy:SetPosRelTo(actor, 14, 20-45)
		enemy:RotateZP(90,0,60)
	end,
	[9] = function(actor)
		actor.landingFrame = 19
		local enemy = actor.userData.t
		enemy:Detach()
		enemy:GroundLevel()
		enemy:SetVector(_vectors.down, actor:GetSide())
		enemy:GotoSequence(26)
		enemy.frozen = false
		enemy:ResetTransform()
		actor.userData.t = nil
	end,
	[10] = function(actor)
		global.DamageTarget(1000)
		global.ParticlesNormalRel(20, 14, 20)
		global.PlaySound("punchStrong")
	end
}
function sathrow(actor)
	runFrameTable(actor, t_sathrow)
end


--------------------- Special moves -----------------------
local t_s623a = {
	[0] = function (actor)
		local hitdef = actor.hitDef
		hitdef:SetVectors(s.stand, v.wallSlam, v.block3)
		hitdef:SetVectors(s.air, v.wallSlam, v.airBlock)
		hitdef:SetVectors(s.crouch, v.wallSlam, v.croBlock3)
		hitdef.hitStop = histopTbl.strong
		hitdef.blockStun = 20
		hitdef.damage = 700
		hitdef.shakeTime = 4
		hitdef.attackFlags = g.hit.wallBounce | g.hit.hitsAir | g.hit.disableCollision
		hitdef.sound = "slash"
	end
}
function s623a (actor)
	local x = actor:GetVel()
	if(actor:GetSide()*x < 0) then
		actor:SetVel(0, 0)
	end
	runFrameTable(actor, t_s623a)
end

function s623b (actor)
	frame = actor.currentFrame
	if(frame == 0 and actor.subframeCount == 0) then
		local hitdef = actor.hitDef
		mediumHit(hitdef)
		hitdef.hitStop = histopTbl.weakest
		hitdef.blockStun = 17
		hitdef.damage = 200
		hitdef.sound = "punchMedium"
	elseif(frame == 14 and actor.subframeCount == 0) then
		local hitdef = actor.hitDef
		hitdef:SetVectors(s.stand, v.down, v.block1)
		hitdef:SetVectors(s.air, v.down, v.airBlock)
		hitdef:SetVectors(s.crouch, v.down, v.croBlock1)
		hitdef.hitStop = histopTbl.strong
		hitdef.blockStun = 14
		hitdef.damage = 500
		hitdef.shakeTime = 8
		hitdef.sound = "punchStrong"
	elseif(frame == 19 and actor.subframeCount == 0) then
		local hitdef = actor.hitDef
		hitdef:SetVectors(s.stand, v.slam, v.block3)
		hitdef:SetVectors(s.air, v.slam, v.airBlock)
		hitdef:SetVectors(s.crouch, v.slam, v.croBlock3)
		hitdef.hitStop = histopTbl.strong
		hitdef.blockStun = 20
		hitdef.damage = 500
		hitdef.sound = "kickStrong"
		hitdef.attackFlags = g.hit.bounce
	end
end

function s236c(actor)
	local fc = actor.totalSubframeCount
	if(fc == 16) then
		local hd = A_spawnPosRel(actor, 508, 280, 80).hitDef
		weakHit(hd)
		hd.damage = 100
		hd.blockStun = 14
		hd.hitStop = histopTbl.medium
		hd.sound = "burn"
	end
end

function s236a(actor)
	local fc = actor.totalSubframeCount
	if(fc == 16) then
		local hd = A_spawnPosRel(actor, 508, 140, 80).hitDef
		weakHit(hd)
		hd.damage = 100
		hd.blockStun = 14
		hd.hitStop = histopTbl.medium
		hd.sound = "burn"
	end
end

function s214a(actor)
	local f = actor.currentFrame
	if(f == 4 and actor.subframeCount == 0) then
		A_spawnPosRel(actor, 177, 40, 0)
		
	end
end

--From 214a
function sChargedTsuki(actor)
	local f = actor.currentFrame
	if(f == 1 and actor.subframeCount == 0) then
		local projectile = A_spawnPosRel(actor, 185, 0, 0)
		local hd = projectile.hitDef
		mediumHit(hd)
		hd.hitStop = histopTbl.weak
		hd.blockStun = 14
		hd.damage = 230
		hd.sound = "burn"
	end
end


--[[
rot = 0
function _update()
	rot = rot+10
	player:RotateZP(rot,0,50)
end
--]]

--[[ function _init()
	print(player.userData)
	player.userData = {}
	print(player.userData)
end ]]
