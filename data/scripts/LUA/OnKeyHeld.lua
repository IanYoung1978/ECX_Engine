function onKeyHeld(entity, event)
    local key = event:getKey()
    local speed = 5.0
    local dt = event:getDeltaTime()

    if key == "W" then
        entity:moveForward(speed * dt)
    elseif key == "S" then
        entity:moveForward(-speed * dt)
    elseif key == "A" then
        entity:moveLeft(speed * dt)
    elseif key == "D" then
        entity:moveRight(speed * dt)
    elseif key == "Space" then
        entity:moveUp(speed * dt)
    elseif key == "C" then
        entity:moveUp(-speed * dt)
    end
end