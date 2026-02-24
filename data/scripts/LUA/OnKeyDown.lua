function onKeyDown(entity, event)
    local key = event:getKey()
	if key == "Escape" then
    print("Escape key pressed, shutting down the game.")
	    game:shutdown()
	end
end