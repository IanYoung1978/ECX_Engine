function onKeyUp(entity, event)
    local key = event:getKey()
    if key == "P" then
        local sphereID = game:getEntityIDByUID(101)
        game:clearParent(sphereID)
        print("Sphere EntityID: " .. sphereID)
    end
    if key == "F1" then
        game:toggleDebug()
    end
end