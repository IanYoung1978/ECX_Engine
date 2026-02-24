local function isParticipant(entity, event)
    local uid = entity:getID()
    local a = event:getCollisionEntityA()
    local b = event:getCollisionEntityB()
    return uid == a or uid == b
end

function onCollisionBegin(entity, event)
    if not isParticipant(entity, event) then
        return
    end
    
    -- Get current collision count
    local collisionCount = entity:getFloat("collision_count", 0.0)
    
    -- Only store original color on FIRST collision
    if collisionCount == 0.0 then
        local c = entity:getColour()
        entity:setFloat("orig_r", c.x)
        entity:setFloat("orig_g", c.y)
        entity:setFloat("orig_b", c.z)
        entity:setFloat("orig_a", c.w)
    end
    
    -- Increment collision count
    entity:setFloat("collision_count", collisionCount + 1.0)
    
    -- Turn red
    entity:setColour(1.0, 0.0, 0.0, 1.0)
end