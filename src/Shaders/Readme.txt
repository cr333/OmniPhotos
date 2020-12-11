This folder contains basic shaders used in the framework.

General/Quad:

- RenderQuad.vertex.glsl
Descr.: Used for quad rendering used for two-pass rendering.
The framebuffer used in apps always draws in its 0th color attachment aka its renderedTexture.
After all drawings are done, the default framebuffer is selected (glBindFramebuffer(0)) and the renderedTexture is drawn on the screen
Used: Framebuffer

- RenderQuad.fragment.glsl
Descr.: Draws each pixel of the renderedTexture in the framebuffer (default framebuffer at this time).
Used: Framebuffer


General:

- ProjectIntoCamera.vertex.glsl 
Descr.: Applies projection matrix of the camera to input vertices (buffer at location 0) and
sets gl_Position.
Used: Camera Rays, what else?

- ProjectIntoCameraAndPassColour.vertex.glsl
Descr.: Same as "ProjectIntoCamera" but passes a vertexColor (buffer at location 1) into the fragment shader.
Vertex stage for colouring input geometry (Vertexbuffers).
Used: Drawing coordinate axes, drawing camera orientations

- UniformColour.fragment.glsl
Descr.: Renders a constant colour set in the file.
Used: Drawing lines.

- PassedColour.fragment.glsl
Descr.: Renders the colour passed by the vertex shader
Used: Render axes in different colours (coordinate Axes, camera orientations)
