function onMouseMove(event,entity,game)

	local orientation = entity.orientation
	
	local deltaYaw = event:x() / 50.0
	local deltaPitch = event:y() / 50.0
	orientation.x = orientation.x - deltaPitch
	orientation.y = orientation.y - deltaYaw
	entity.orientation = orientation
	return 0
end