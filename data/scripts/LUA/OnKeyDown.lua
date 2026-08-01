mouseCaptured = true
gamePaused = true

function onKeyDown(entity, event)
    local key = event:getKey()
	if key == "Escape" then
    print("Escape key pressed, shutting down the game.")
	    game:shutdown()
	end
	if key == "F3" then
	    debugOverlayVisible = not debugOverlayVisible
	end
	if key == "F4" then
	    mouseCaptured = not mouseCaptured
	    game:setMouseCaptured(mouseCaptured)
	end
	if key == "P" then
	    gamePaused = not gamePaused
	    if gamePaused then
	        game:pauseGame()
	    else
	        game:resumeGame()
	    end
	end
end
