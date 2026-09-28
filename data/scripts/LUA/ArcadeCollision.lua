-- Arcade ground contact for a Kinematic body (the impulse solver never touches it): landing on terrain
-- zeroes velocity and sets ignoreGravity, then pushes the entity out along the contact normal by the
-- penetration depth; losing all terrain contact resumes gravity. The engine re-fires onCollisionBegin
-- while a Kinematic body moves in contact, so each step corrects against fresh contact data. Contacts
-- are tracked as a set of terrain entity IDs, not a counter, because of those re-fires.
local groundContacts = {}

-- Matches EC_VoxelChunkSystem's chunk naming ("voxelchunk_x_z"); what else counts as ground is a game decision.
local function isTerrain(otherEntity)
    return otherEntity:getName():match("^voxelchunk_") ~= nil
end

function onCollisionBegin(entity, event)
    local otherId = event:getOtherEntityID()
    if otherId == 0 then
        return
    end
    local other = game:getEntityByID(otherId)

    if isTerrain(other) then
        local entityId = entity:getID()
        groundContacts[entityId] = groundContacts[entityId] or {}
        groundContacts[entityId][otherId] = true

        entity:setVelocity(0.0, 0.0, 0.0)
        entity:setIgnoreGravity(true)

        -- Push out along the contact normal by the penetration depth (correct on slopes, not just flat ground).
        local normal = event:getCollisionNormal()
        local depth = event:getCollisionPenetrationDepth()
        local pos = entity:getPosition()
        entity:setPosition(pos.x + normal.x * depth, pos.y + normal.y * depth, pos.z + normal.z * depth)
    end
end

function onCollisionEnd(entity, event)
    local otherId = event:getOtherEntityID()
    if otherId == 0 then
        return
    end
    local other = game:getEntityByID(otherId)

    if isTerrain(other) then
        local entityId = entity:getID()
        if groundContacts[entityId] then
            groundContacts[entityId][otherId] = nil
        end

        local stillGrounded = groundContacts[entityId] and next(groundContacts[entityId]) ~= nil
        if not stillGrounded then
            entity:setIgnoreGravity(false)
        end
    end
end
