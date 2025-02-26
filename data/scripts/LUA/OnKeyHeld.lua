function onKeyHeld(event,entity,game)

	local speed = 0.1
	local velocity = vec3(0,0,0)
	if event:key() == "W" then
		entity:moveForward(speed)
	elseif event:key() == "S" then
		entity:moveForward(-speed)
	elseif event:key() == "A" then
		entity:moveRight(-speed)
	elseif event:key() == "D" then
		entity:moveRight(speed)
	elseif event:key() == "Space" then
		entity:moveUp(speed)
	elseif event:key() == "C" then
		entity:moveUp(-speed)
	end
	return 0
end