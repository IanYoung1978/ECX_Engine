function onMouseMove(entity, event)
    if mouseCaptured == false then
        return
    end

    local mouseX = event:getMouseMotionX()
    local mouseY = event:getMouseMotionY()
    
    local sensitivity = 0.02
    
    local orient = entity:getOrientation()
    
    local newYaw = orient.y - mouseX * sensitivity
    local newPitch = orient.x - mouseY * sensitivity
    
    newPitch = math.max(-1.5, math.min(1.5, newPitch))
    
    entity:setOrientation(newPitch, newYaw, orient.z)
end