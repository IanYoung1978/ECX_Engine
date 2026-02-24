-- Change mesh colour when this entity starts/ends a collision.

local function isParticipant(entity, event)
    local uid = entity:getID()
    local a = event:getCollisionEntityA()
    local b = event:getCollisionEntityB()
    print("Checking if entity " .. uid .. " is part of the collision between " .. a .. " and " .. b)
    return uid == a or uid == b
end

function onCollisionEnd(entity, event)
    print("Collision ended for entity " .. entity:getID())
    if not isParticipant(entity, event) then
        print("Entity " .. entity:getID() .. " was not part of the collision, ignoring.")
        return
    end
    

    local r = entity:getFloat("orig_r", 1.0)
    local g = entity:getFloat("orig_g", 1.0)
    local b = entity:getFloat("orig_b", 1.0)
    local a = entity:getFloat("orig_a", 1.0)

    entity:setColour(r, g, b, a)
end