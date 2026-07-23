-- Feeds this entity's position into the scene streaming system every frame.
-- Attach via <ScriptComponent><OnUpdate filename="..."/></ScriptComponent> on
-- whichever entity the game considers "the player" (a camera, a character, etc).
function update(entity, deltaTime)
    game:setStreamingReferencePosition(entity:getPosition())
end
