-- Change mesh colour when this entity starts/ends a collision.

local function isParticipant(entity, event)
    local uid = entity:getID()
    local a = event:getCollisionEntityA()
    local b = event:getCollisionEntityB()
    return uid == a or uid == b
end

function onCollisionEnd(entity, event)
    if not isParticipant(entity, event) then
        return
    end
    
    -- Decrement collision count
    local collisionCount = entity:getFloat("collision_count", 0.0)
    collisionCount = collisionCount - 1.0
    entity:setFloat("collision_count", collisionCount)
    
    -- Only restore color when ALL collisions have ended
    if collisionCount <= 0.0 then
        local r = entity:getFloat("orig_r", 1.0)
        local g = entity:getFloat("orig_g", 1.0)
        local b = entity:getFloat("orig_b", 1.0)
        local a = entity:getFloat("orig_a", 1.0)
        entity:setColour(r, g, b, a)
        entity:setFloat("collision_count", 0.0)  -- Clamp to zero
    end
end