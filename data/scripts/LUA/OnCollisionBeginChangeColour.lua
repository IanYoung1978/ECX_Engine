local function isParticipant(entity, event)
    local uid = entity:getID()
    local a = event:getCollisionEntityA()
    local b = event:getCollisionEntityB()
    print("Checking if entity " .. uid .. " is part of the collision between " .. a .. " and " .. b)
    return uid == a or uid == b
end

function onCollisionBegin(entity, event)
    if not isParticipant(entity, event) then
    print("Entity " .. entity:getID() .. " was not part of the collision, ignoring.")
        return
    end

    local c = entity:getColour()
    entity:setFloat("orig_r", c.x)
    entity:setFloat("orig_g", c.y)
    entity:setFloat("orig_b", c.z)
    entity:setFloat("orig_a", c.w)

    -- Highlight on collision
    entity:setColour(1.0, 0.0, 0.0, 1.0)
end