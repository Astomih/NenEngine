local texture_clear = {}
local font_clear = {}
local drawer_clear = {}
local scene_switcher = require("scene_switcher")()

function Setup()
    texture_clear = texture()
    drawer_clear = drawui(texture_clear)
    font_clear = font()
    font_clear:load(DEFAULT_FONT_NAME, 64)
    font_clear:render_text(texture_clear, "STAGE CLEAR", color(1, 1, 1, 1))
    drawer_clear.scale = texture_clear:size()
    scene_switcher:setup()
    scene_switcher:start(true, "")
end

local function draw()
    drawer_clear:draw()
end

function Update()
    if scene_switcher.flag then
        scene_switcher:update(draw)
        return
    end
    draw()
    if mouse:is_button_pressed(mouseLEFT) then
        scene_switcher:start(false, "scene00_base")
    end
end
