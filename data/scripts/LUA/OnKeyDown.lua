function onKeyDown(event,entity,game)
		if event:key() == "Escape" then
			game:shutdown()
			return 0
		end
		
		return 0
end