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
    elseif key == "Space" then
        entity:moveUp(speed * 0.016)
    elseif key == "C" then
        entity:moveUp(-speed * 0.016)
    end
end