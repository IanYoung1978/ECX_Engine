-- OnKeyHeld for the arcade-physics camera (VoxelChunkDemo.xml) - horizontal movement only.
-- The shared OnKeyHeld.lua (every other scene's free-fly debug camera) binds Space/C to
-- continuous fly up/down, which conflicts with this scene's jump-on-release mechanic (see
-- OnKeyUp.lua's own comment) - holding Space would fly the capsule upward every frame on
-- top of whatever the jump/gravity mechanic is doing. Vertical movement here comes only
-- from RigidBody velocity (gravity/jump), never a direct moveUp() from held input.
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
    end
end
