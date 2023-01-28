local texture_dead = {}
local font_dead = {}
local drawer_dead = {}
local scene_switcher = require("scene_switcher")()

function Setup()
    texture_dead = texture()
    drawer_dead = drawui(texture_dead)
    font_dead = font()
    font_dead:load(DEFAULT_FONT_NAME, 64)
    font_dead:render_text(texture_dead, "YOU DEAD", color(1, 0.25, 0.25, 1))
    drawer_dead.scale = texture_dead:size()
    scene_switcher:setup()
    scene_switcher:start(true, "")
end

local function draw()
    drawer_dead:draw()
end

function Update()
    if scene_switcher.flag then
        scene_switcher:update(draw)
        return
    end
    draw()
    if keyboard:is_key_pressed(keyENTER) then
        scene_switcher:start(false, "main")
    end
end
