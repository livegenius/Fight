
local key = {
	UP = 0x1,
	DOWN = 0x2,
	LEFT = 0x4,
	RIGHT = 0x8,
	A = 0x10,
	B = 0x20,
	C = 0x40,
	D = 0x80,
}

local g = global;

--behavior
local getCloser = false
local idle = true
local timer = 0

local action = {0}

function _ai(actor, enemy)
	local keyLeft = key.LEFT
	local keyRight = key.RIGHT
	if actor:GetSide() < 0 then
		keyLeft = key.RIGHT
		keyRight = key.LEFT
	end

	local x,y = actor:GetPosInt()
	local xV,yV = actor:GetVelInt()
	local ex,ey = enemy:GetPosInt()
	local distX = math.abs(x-ex);

	timer = timer - 1
	if timer < 0 then
		idle = true
		action = {0}
	end

	if idle and g.RandomChance(20) then --decide on next action
		idle = false
		
		
		if y > 32 and g.RandomChance(90) then
			if actor:ThrowCheck(enemy, 60, 120, -30) then
				-- "try throw"
				action = {0, keyRight | key.D}
				timer = 2
				return action
			elseif yV <= 0 and g.RandomChance(80) then
				return {0, key.C}
			elseif g.RandomChance(50) then
				return {0, keyLeft, 0, keyLeft}
			else 
				return {0, keyRight, 0, keyRight}
			end
		end

		local farChance = 10

		if distX > 80 then
			farChance = 90
		end

		if g.RandomChance(farChance) then
			timer = 0
			local what = g.RandomInt(0, 6)
			if what <= 1 then
				timer = g.RandomInt(5, 20)
				action = {keyRight}
			elseif what == 2 then
				timer = 20
				action = {0}
				return{0, keyRight | key.A | key.B}
			elseif what == 3 then --jump
				local what2 = g.RandomInt(0, 3)
				if what2 == 0 then
					timer = 10
					action = {keyRight | key.UP}
				elseif what2 == 1 then
					timer = g.RandomInt(1, 60)
					action = {key.UP}
					return {key.UP}
				elseif what2 == 2 then
					timer = g.RandomInt(1, 60)
					action = {keyLeft}
					return {key.UP}
				elseif what2 == 3 then
					timer = 10
					action = {keyLeft | key.UP}
				end
			elseif what == 4 then
				if distX < 200 then
					return {key.DOWN, key.DOWN | keyRight, keyRight, key.A}
				else
					return {key.DOWN, key.DOWN | keyRight, keyRight, key.B}
				end
			elseif what == 5 then
				if distX < 200 then
					return {keyRight, key.DOWN, key.DOWN | keyRight, key.A}
				else
					timer = g.RandomInt(5, 30)
					action = {key.DOWN}
					return action
				end
			elseif what == 6 then
				return {key.DOWN, key.DOWN | keyLeft, keyLeft, key.A}
			end
		else
			timer = 0
			
			if actor:ThrowCheck(g.GetTarget(), 50, 0 ,0) and g.RandomChance(25) then
				if g.RandomChance(50) then
					return {0, keyRight | key.D}
				else
					return {0, keyLeft | key.D}
				end
			end

			local what = g.RandomInt(0, 7)
			if what == 0 then
				return {0, key.DOWN | key.A}
			elseif what == 1 then
				return {0, key.DOWN | key.B}
			elseif what == 2 then
				return {0, key.B}
			elseif what == 3 then
				return {0, key.DOWN | key.C}
			elseif what == 4 then
				return {0, keyLeft | key.C}
			elseif what == 5 then
				return {0, key.A}
			elseif what == 6 then
				return {0, key.C}
			elseif what == 7 then
				timer = 40
				action = {keyLeft}
				return {0, keyLeft, 0, keyLeft}
			end
		end
	end

	return action
end