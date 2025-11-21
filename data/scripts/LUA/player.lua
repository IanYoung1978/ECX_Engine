-- Player controller script

-- Called every frame
--[[
function update(entity, deltaTime)
    -- Simple movement logic
    local vel = entity:getVelocity()
    
    -- Apply gravity
    vel.y = vel.y - 9.8 * deltaTime
    
    entity:setVelocity(vel.x, vel.y, vel.z)
end
--]]
-- Called when key is held down
function onKeyHeld(entity, event)
    local key = event:getKey()
    local speed = 5.0
    
    if key == "W" then
        entity:moveForward(speed * 0.016)
    elseif key == "S" then
        entity:moveForward(-speed * 0.016)
    elseif key == "A" then
        entity:moveLeft(speed * 0.016)  -- Use the built-in function!
    elseif key == "D" then
        entity:moveRight(speed * 0.016)  -- Use the built-in function!
    end
end

-- Called when mouse moves
function onMouseMove(entity, event)
    local mouseX = event:getMouseMotionX()
    local mouseY = event:getMouseMotionY()
    
    local sensitivity = 0.01
    
    local orient = entity:getOrientation()
    
    local newYaw = orient.y - mouseX * sensitivity
    local newPitch = orient.x - mouseY * sensitivity
    
    newPitch = math.max(-1.5, math.min(1.5, newPitch))
    
    entity:setOrientation(newPitch, newYaw, orient.z)
end
--[[
-- Called when key is first pressed
function onKeyDown(entity, event)
    local key = event:getKey()
    
    if key == "Space" then
        -- Jump
        local vel = entity:getVelocity()
        vel.y = 10.0
        entity:setVelocity(vel.x, vel.y, vel.z)
        print("Player jumped!")
    end
end

-- Optional: initialization (called when entity is created)
function onInit(entity)
    print("Player script initialized for entity")
end

-- Optional: cleanup
function onDestroy(entity)
    print("Player script destroyed")
end
--]]