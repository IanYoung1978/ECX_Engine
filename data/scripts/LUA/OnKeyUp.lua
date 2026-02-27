function onKeyUp(entity, event)
    local key = event:getKey()
    print("Key released: " .. key)
    if key == "P" then
        local sphereID = game:getEntityIDByUID(101)
        game:clearParent(sphereID)
        print("Sphere EntityID: " .. sphereID)
    end
end