-- "Arcade physics" ground contact: this entity's RigidBody is BodyType=Kinematic (see
-- <RigidBody> in its scene XML), so EC_PhysicsResolution never runs its impulse solver on
-- it - detection and this event still fire normally, but nothing stops the fall until this
-- handler says so. Landing on terrain sets RigidBody::ignoreGravity and zeroes velocity
-- directly; there is no per-frame ground query left to do in script - the engine's own
-- collision detection and the RigidBody flags are the whole mechanism. Jumping (see
-- DebugSceneKeyDown.lua's onKeyDown) is the mirror image: push velocity upward and clear
-- ignoreGravity so gravity resumes accumulating next tick.
--
-- Walking across uneven ground while already resting is handled the same way, not as a
-- special case: EC_NarrowPhase re-fires this same BEGIN event (see EC_CollisionPair's own
-- comment) whenever a Kinematic body moves while a pair is already colliding, carrying that
-- moment's fresh contact normal/depth - so every step's "moving should cause a re-test and
-- correction" just re-runs this exact handler again with up-to-date contact data, snapping
-- position back onto the true surface instead of the capsule silently sinking/clipping into
-- a slope as it moves.
--
-- Cresting a hill / walking off an edge is the mirror image of landing: OnCollisionEnd
-- fires when the capsule stops overlapping a terrain chunk, and gravity resumes so it falls
-- until the next OnCollisionBegin. A capsule straddling two terrain chunks at once (a chunk
-- boundary) can End one pair while still overlapping the other, so gravity only actually
-- resumes once no terrain entity remains in groundContacts[entity] - the SET of
-- currently-overlapping terrain entity IDs, not a plain counter: EC_NarrowPhase re-fires
-- BEGIN for the same still-touching chunk every time the capsule moves (see this file's own
-- comment above), so counting Begins/Ends 1:1 would never balance back to zero.
local groundContacts = {}

-- Matches EC_VoxelChunkSystem::init()'s own naming ("voxelchunk_" + x + "_" + z) - anything
-- else this entity touches (another entity, a projectile, etc.) is a different, game-
-- specific decision this base mechanic deliberately doesn't make.
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

        -- Push the capsule back out along the true contact normal by the penetration
        -- depth - resolves however much this step's move drove it into the surface,
        -- correctly even on a slope (not just a flat-ground, Y-only nudge).
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
