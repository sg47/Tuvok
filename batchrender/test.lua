require "util"

hostname = util.hostname()
print("Hostname is: " .. hostname)
if hostname == "takeo" then
  print("Loading takeo.lua...")
  require "takeo"
elseif hostname == "MBPRO-Infi.local" or hostname == "Rainer-Schlonvoigts-Tower-PC.local" or hostname == "rschloenvoigt-B85-HD3" then
  print("Loading rakeo.lua...")
  require "rakeo"
else
  error("Can't load proper host file for host " .. hostname .. "...")
end

width = 320
height = 240

print("Initializing renderer")
ren = tuvok.renderer.new(tuvok.renderer.types.OpenGL_GridLeaper, true, false,
                         false, false)
ren.addShaderPath("Shaders")
bsize = {35, 35, 35} -- size of the bricks to use/rebrick into.
netSuccess = ren.loadNetDS(machine.dataset.engine, bsize, MM_PRECOMPUTE, width, height)
if not netSuccess then
  error("Could not load net dataset!")
end

-- Parameters are: Framebuffer width and height, color bits, depth bits,
--                 stencil bits, double buffer, and if visible.
context = tuvok.createContext(width, height, 32,24,8, true, false)
ren.initialize(context)
ren.resize({width, height})
ren.setRendererTarget(tuvok.renderer.types.RT_Headless)
mat = matrix.rotateY(80) * matrix.rotateZ(-20) * matrix.rotateX(30)
-- ren.setRotation(mat)
ren.setSampleRateModifier(8.0)
ren.paint()

ren.setRendererTarget(tuvok.renderer.types.RT_Capture)
ren.captureSingleFrame(outputDir .. '/render.png', true)
ren.cleanup()
deleteClass(ren)
