-- UniMod Framework Entry Script
unimod.log_info("Hello from Lua Runtime! UniMod Scripting is active.")

_G.UNIMOD_VERSION = "0.1.0"
_G.UNIMOD_INIT_LOADED = true

if mock_engine then
    unimod.log_info("Discovered mock_engine! Current character: " .. mock_engine.get_name())
    unimod.log_info("Initial Health: " .. tostring(mock_engine.get_health()))
    -- Boost speed via Lua mod
    mock_engine.set_speed(15.0)
    unimod.log_info("Updated speed to: " .. tostring(mock_engine.get_speed()) .. " m/s")
end
