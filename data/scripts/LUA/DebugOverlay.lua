debugOverlayVisible = false

local uiReady = false
local fpsTextID, logTextID, panelID

function update(entity, deltaTimeS)
    if not uiReady then
        panelID = game:getEntityByName("debug_panel"):getID()
        fpsTextID = game:getEntityByName("debug_fps_text"):getID()
        logTextID = game:getEntityByName("debug_log_text"):getID()

        -- Entity name lookup isn't populated until the scene finishes loading
        -- (EC_SceneManager::buildEntityMaps), which happens well after this
        -- entity's first update() call - keep retrying until the names resolve.
        if panelID == 0 or fpsTextID == 0 or logTextID == 0 then
            return
        end
        uiReady = true
    end

    game:setUIVisible(panelID, debugOverlayVisible)
    game:setUIVisible(fpsTextID, debugOverlayVisible)
    game:setUIVisible(logTextID, debugOverlayVisible)

    if not debugOverlayVisible then
        return
    end

    game:setUIText(fpsTextID, string.format("FPS: %.1f  (%.2f ms)", game:getFPS(), game:getMSPF()))

    local count = game:getRecentLogCount()
    local startIndex = math.max(0, count - 8)
    local lines = {}
    for i = startIndex, count - 1 do
        local line = game:getRecentLog(i)
        if #line > 100 then
            line = string.sub(line, 1, 100) .. "..."
        end
        table.insert(lines, line)
    end
    game:setUIText(logTextID, table.concat(lines, "\n"))
end
