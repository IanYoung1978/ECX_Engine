function onKeyDown(event,entity,game)
		if event:key() == "Escape" then
			game:shutdown()
			return 0
		end
		
		--local velocity = entity.velocity
		--velocity = vec3.normalize(velocity)
		--if event:key() == "W" then
		--	velocity = velocity + entity:forward()
		--elseif event:key() == "S" then
		--	velocity = velocity - entity:forward()
		--elseif event:key() == "A" then
		--	velocity = velocity - entity:right()
		--elseif event:key() == "D" then
		--	velocity = velocity + entity:right()
		--elseif event:key() == "SPACE" then
		--	velocity = velocity + entity:up()
		--elseif event:key() == "C" then
		--	velocity = velocity - entity:up()
		--end		
		--velocity = vec3.normalize(velocity * 5.0)
		--entity.velocity = velocity
		return 0
end