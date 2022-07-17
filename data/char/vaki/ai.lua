
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

local fflags ={
	canMove = 0x1,
	dontWalk = 0x2,
	startHit = 0x80000000
}

local eComboType = {
	none = 0,
	hurt = 1,
	blocked = 2,
	counter = 3,
}

local sflags = {
	enterOnWhiff = 0x1
}

local g = global;

--Main behavior
local idle = true
local timer = 0 --Timer until next action
local action = {0} --Action to execute

--Sequence execution
local inSeq = false
local successfulInput = false

function _ai(actor, enemy, successfulInput_)
	successfulInput = successfulInput_
	keyLeft = key.LEFT
	keyRight = key.RIGHT
	if actor:GetSide() < 0 then
		keyLeft = key.RIGHT
		keyRight = key.LEFT
	end

	x,y = actor:GetPosInt()
	xV,yV = actor:GetVelInt()
	ex,ey = enemy:GetPosInt()
	distX = math.abs(x-ex)
	distY = math.abs(y-ey)
	side = actor:GetSide()
	distXrel = side*-(x-ex)

	if inSeq then
		return seqLoop(actor, enemy)
	end

--[[ 	if distXrel > 0 and distXrel < 65 and y > 32 and ey > 32 and distY < 30 then
		print (distY .. ' ' .. distXrel)
	end

	return {key.UP}
end
function etcetc() ]]

	timer = timer - 1
	if timer < 0 then
		action = {key.DOWN}
		idle = true
	end

	
	
	local combo = actor.comboType;
	combo = combo == eComboType.hurt or combo == eComboType.counter
	if combo then 
		--print (yV .. " " .. distXrel .. " " .. distY )
		if distXrel > 0 and distXrel < 80 and ey > 32 and ey-y < 40 and ey-y > -20 then
			return execSeq(actor, enemy,{
				{{key.UP| keyRight}, 0},
				{{0, key.A}, 10, sflags.enterOnWhiff},
				{{0, key.C}},
				{{0, key.UP | keyRight}, 0},
				{{0, key.B}, 10, sflags.enterOnWhiff},
				{{0, key.C}, 0},
				{{0, key.D}, 0},
			})
		elseif g.RandomChance(25) and distXrel > 0 and distXrel < 70 and y == 32 and distY < 50 then
			return execSeq(actor, enemy,{
				{{0, key.DOWN | keyRight | key.C}},
				{{key.UP| keyRight}, 0},
				{{0, key.A}, 4, sflags.enterOnWhiff},
				{{0, key.B}},
				{{0, key.C}},
				{{0, key.UP | keyRight}, 0},
				{{0, key.B}, 0, sflags.enterOnWhiff},
				{{0, key.C}, 0},
				{{0, key.D}, 0},
			})
		elseif yV >= -8 and distXrel > 0 and distXrel < 75 and y > 32 and ey > 32 and distY < 50 then
			return execSeq(actor, enemy,{
				{{0, key.A}},
				{{0, keyRight, 0, keyRight}},
				{{0, key.B}, 9, sflags.enterOnWhiff},
				{{0, key.UP | keyRight}, 0},
				{{0, key.B}, 3, sflags.enterOnWhiff},
				{{0, key.C}},
				{{0, key.D}},
			})
		end
	end
	

 	if idle then --decide on next action
		idle = false
		timer = 0

		local instantAction
		instantAction, action, timer = nextAction(actor, enemy)

		instantAction = instantAction or {0}
		action = action or {0}
		timer = timer or 0
		return instantAction
	end

	return action
end


local sequence = {}
local stepIndex = 1
local seqTimer = 0
local step = {}
local curSeq = -1
function seqLoop(actor, enemy)
	if seqTimer == 0 then
		local flag = step[3] or 0
		
		if flag & sflags.enterOnWhiff == 0 and curSeq ~= actor.currentSequence and g.GetWhiffed() then
			--print("whiffed")
			inSeq = false
			return {0}
		end

		if successfulInput then
			curSeq = actor.currentSequence
			if not nextStep() or seqTimer > 0 then
				return {0}
			end
		end

		return step[1] --or {0}
	else
		seqTimer = seqTimer - 1
	end

	return {0}
end

function nextStep()
	step = sequence[stepIndex]
	if step then
		--print("stepindex " .. stepIndex)
		--print(step[1][2])
		seqTimer = step[2] or 0
	elseif step == nil then
		inSeq = false
		return false
	end
	stepIndex = stepIndex + 1
	return true
end

function execSeq(actor, enemy, seq)
	curSeq = -1
	inSeq = true
	sequence = seq
	stepIndex = 1
	successfulInput = false
	if nextStep() then
		sequence[1][3] = sflags.enterOnWhiff
		return seqLoop(actor, enemy)
	else
		return {0}
	end
end

function tryJump()
	local what2 = g.RandomInt(0, 3)

	if what2     == 0 then
		return {key.RIGHT | key.UP}, {0}, 10
	elseif what2 == 1 then
		return {key.UP}, {key.LEFT}, g.RandomInt(1, 10)
	elseif what2 == 2 then
		return {key.UP}, {key.RIGHT}, g.RandomInt(1, 10)
	elseif what2 == 3 then
		return {key.LEFT | key.UP}, {0}, 10
	end
end

function nextAction(actor, enemy)
	local timer = 0
	local action = {0}

	--In air only
	if y > 32 then
		if actor:ThrowCheck(enemy, 60, 120, -30) and g.RandomChance(5) then
			action = {0, keyRight | key.D}
			return action, action, 2
		elseif yV <= 0 and xV ~= 0 and distX < 100 and ey < y and distY < 100 and g.RandomChance(95) then
			return {0, key.C}, {0}, 0 
		elseif distX > 130 and g.RandomChance(90) then
			if x > ex then
				return {0, key.LEFT, 0, key.LEFT}, {0}, g.RandomInt(0,20)
			else
				return {0, key.RIGHT, 0, key.RIGHT}, {0}, g.RandomInt(0,20)
			end
		else
			local what = g.RandomInt(1,4)
			if what == 1 then
				if g.RandomChance(60) then
					return {0, keyLeft, 0, keyLeft}
				else 
					return {0, keyRight, 0, keyRight}
				end
			elseif what == 2 then
				if(ey > y+10) then
					return {0, key.B}
				elseif y < 150 and yV <= 0 then
					return {0, key.C}
				else
					return {0, key.A}
				end
			elseif what == 4 then
				return tryJump()
			end
		end
	end


	local farChance = math.max(distX - 70, 0)
	if g.RandomChance(farChance) then
		local enemyCanMove = (enemy:GetFrameProperty().flags & fflags.canMove) ~= 0

		if not enemyCanMove and distY < 50 and g.RandomChance(50) then
			if distX > 250 and distX < 310 then
				return {key.DOWN, key.DOWN | keyRight, keyRight, key.B}
			elseif distX > 110 and distX < 170 then
				return {key.DOWN, key.DOWN | keyRight, keyRight, key.A}
			end
		end

		local what = g.RandomInt(0, 5)
		if what <= 1 then
			timer = g.RandomInt(5, 20)
			action = {keyRight}
		elseif what == 2 then
			return{0, keyRight | key.A | key.B}, {0}, 10
		elseif what == 3 then --jump
			return tryJump()
		elseif what == 4 then
			if distX < 200 then
				action = {key.DOWN, key.DOWN | keyRight, keyRight, key.A}
			elseif ey < 60 and distX > 80 then
				action = {key.DOWN, key.DOWN | keyRight, keyRight, key.B}
			else
				timer = g.RandomInt(5, 10)
				action = {keyLeft}
			end
		elseif what == 5 then
			if ey > 32 and g.RandomChance(80) then
				return tryJump()
			else
				action = {key.DOWN, key.DOWN | keyLeft, keyLeft, key.A}
			end
		end

		return action, action, timer
	else
		--Try throwing
		local canMove = (actor:GetFrameProperty().flags & fflags.canMove) ~= 0
		if canMove and g.RandomChance(10) and actor:ThrowCheck(g.GetTarget(), 50, 0 ,0) then
			if g.RandomChance(50) then
				return {0, keyRight | key.D}, {0}, 0
			else
				return {0, keyLeft | key.D}, {0}, 0
			end
		end

		if ey > 32 and g.RandomChance(90) then
			if distX < 80 and distY < 100 and g.RandomChance(70) then
				return {0, key.A}, {0}, 0
			elseif distX < 150 and distY >= 100 and distY < 200 and g.RandomChance(30) then
				return {0, key.DOWN | keyRight | key.C}, {0}, 0
			else
				return tryJump()
			end
		end

		if distX > 80 and g.RandomChance(80) then
			if g.RandomChance(80) then
				return {0, keyRight, 0, keyRight}, action, 6
			else
				local a, b = tryJump()
				return a, b, 0
			end
		end 
		
		local what = g.RandomInt(0, 7)
		if what == 0 then
			action = {0, key.DOWN | key.A}
		elseif what == 1 then
			action = {0, key.DOWN | key.B}
		elseif what == 2 then
			action = {0, key.B}
		elseif what == 3 then
			action = {0, key.DOWN | key.C}
		elseif what == 4 then
			action = {0, keyLeft | key.C}
		elseif what == 5 then
			action = {0, key.A}
		elseif what == 6 then
			action = {0, key.C}
		elseif what == 7 then
			if y == 32 and math.abs(x) > 230 and g.RandomChance(50) then
				local what2 = g.RandomInt(0,3)
				if what2 == 0 then
					return execSeq(actor, enemy,{
						{{key.UP| keyLeft}},
						{{0, keyRight, 0, keyRight}, 6, sflags.enterOnWhiff},
						{{0, key.C}, 0, sflags.enterOnWhiff},
					}), {0}, 0
				elseif what2 == 1 then
					return execSeq(actor, enemy,{
						{{key.UP| keyLeft}},
						{{0, keyRight, 0, keyRight}, 6, sflags.enterOnWhiff},
						{{0, keyRight, 0, keyRight}, 6, sflags.enterOnWhiff},
						{{0, key.C}, 0, sflags.enterOnWhiff},
					}), {0}, 0
				elseif what2 == 2 then
					return execSeq(actor, enemy,{
						{{key.UP| keyLeft}},
						{{0, keyRight, 0, keyRight}, 14, sflags.enterOnWhiff},
						{{0, key.C}, 0, sflags.enterOnWhiff},
					}), {0}, 0
				else
					return execSeq(actor, enemy,{
						{{key.UP| keyLeft}},
						{{0, keyRight, 0, keyRight}, 0, sflags.enterOnWhiff},
					}), {0}, 0
				end
			else
				return {keyRight}, {keyRight}, g.RandomInt(1,10)
			end
		end
	end

	return action, action, timer
end
