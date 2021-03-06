#include <iostream>
#include <vector>
#include <typeinfo>
#include <algorithm>
#include <string>
#include <map>
#include <cmath>
#include <ctime>

#include "../include/GL/glew.h"
#include "../include/GL/freeglut.h"
#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include "../include/glm/gtc/type_ptr.hpp"

#include "../include/shader.h"


#include "data.hpp"
#include "Config.hpp"
#include "Input.hpp"
#include "Camera.hpp"
#include "tutorial16.hpp"

void Init(void);
void display(void);
void draw(void);
void updateFrameCount();
void reshape(int width, int height);
void idle(void);

std::time_t parameters_time;
tutorial16 t16;

int main(int argc, char *argv[])
{
	using namespace Configs;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("2017CG final project 0656604_0556652 - LTC");
	glutReshapeWindow(512, 512);
	glewInit();

	Init();

	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);
	glutMouseWheelFunc(mouseWheel);
	glutPassiveMotionFunc(mouse);
	glutSetCursor(GLUT_CURSOR_NONE);
	glutSpecialFunc(SpecialInput);

	timebase = glutGet(GLUT_ELAPSED_TIME);
	glutMainLoop();

	return 0;
}

void InitShaders()
{
	using namespace Configs;

	GLuint vertex_shader = createShader("shaders/ltc/ltc.vs", "vertex");
	GLuint fragment_shader = createShader("shaders/ltc/ltc.fs", "fragment");
	ltcProgram = createProgram(vertex_shader, fragment_shader);

	GLuint blit_vs = createShader("shaders/ltc/ltc_blit.vs", "vertex");
	GLuint blit_fs = createShader("shaders/ltc/ltc_blit.fs", "fragment");
	blitProgram = createProgram(blit_vs, blit_fs);

	GLuint genshadowMap_vs = createShader("shaders/GenShadowMap.vs", "vertex");
	GLuint genshadowMap_fs = createShader("shaders/GenShadowMap.fs", "fragment");
	depthProgram = createProgram(genshadowMap_vs, genshadowMap_fs);

	GLuint debug_vs = createShader("shaders/Debug.vs", "vertex");
	GLuint debug_fs = createShader("shaders/Debug.fs", "fragment");
	debugProgram = createProgram(debug_vs, debug_fs);

	GLuint shadowMapping_vs = createShader("shaders/shadowMapping.vs", "vertex");
	GLuint shadowMapping_fs = createShader("shaders/shadowMapping.fs", "fragment");
	shadowProgram = createProgram(shadowMapping_vs, shadowMapping_fs);

	GLuint bgfx_vs = createShader("shaders/ltc_bgfx.vs", "vertex");
	GLuint bgfx_fs = createShader("shaders/ltc_bgfx.fs", "fragment");
	bgfxProgram = createProgram(bgfx_vs, bgfx_fs);

	GLuint shadowMask_vs = createShader("shaders/shadowMask.vs", "vertex");
	GLuint shadowMask_fs = createShader("shaders/shadowMask.fs", "fragment");
	shadowMaskProgram = createProgram(shadowMask_vs, shadowMask_fs);

	GLuint bgfx_s_vs = createShader("shaders/ltc_bgfx_Shadow.vs", "vertex");
	GLuint bgfx_s_fs = createShader("shaders/ltc_bgfx_Shadow.fs", "fragment");
	bgfxShadowProgram = createProgram(bgfx_s_vs, bgfx_s_fs);
	
}

void InitBuffers()
{
	using namespace Configs;

	// Create Vertex buffer (2 triangles)
	glGenBuffers(1, &screenBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, screenBuffer);
	float screen[] = {
		-1.0, -1.0,
		1.0, -1.0,
		-1.0,  1.0,
		1.0, -1.0,
		1.0,  1.0,
		-1.0,  1.0
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, screen, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);


	float lightRect[] = {
		-1.0, -1.0,  0.0,
		-1.0,  1.0,  0.0,
		1.0, -1.0,  0.0,
		1.0, -1.0,  0.0,
		-1.0,  1.0,  0.0,
		1.0,  1.0,  0.0
	};
	// Create Light Rect buffer
	glGenBuffers(1, &lightRectBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, lightRectBuffer);

	// Static for Now
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 6, lightRect, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);

}

void InitScreenFrameBuffer()
{
	using namespace Configs;

	/*
	*	Generate RTT Texture
	*/

	//glCreateFramebuffers(1,&rttFramebuffer); // just for OpenGL 4.5
	glGenFramebuffers(1, &rttFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, rttFramebuffer);

	rttFramebuffer_width = 512; // FIXME
	rttFramebuffer_height = 512;

	//glCreateTextures(GL_TEXTURE_2D,1,&rttTexture); // just for OpenGL 4.5
	glGenTextures(1, &rttTexture);
	glBindTexture(GL_TEXTURE_2D, rttTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, rttFramebuffer_width, rttFramebuffer_height, 0, GL_RGBA, GL_FLOAT, NULL); // Note: use GL_RGBA32F instead of GL_RGBA
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rttTexture, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	glBindTexture(GL_TEXTURE_2D, NULL);


	/*
	*	Generate Depth Map
	*/

	glGenFramebuffers(1, &depthBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, depthBuffer);

	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);
	glDrawBuffer(GL_NONE);

	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "FrameBuffer Not Ready" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	glBindTexture(GL_TEXTURE_2D, NULL);


	/*
	*	Create ShadowMaskBuffer
	*/
	glGenFramebuffers(1, &shadowMaskBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMaskBuffer);

	glGenTextures(1, &shadowMaskTexture);
	glBindTexture(GL_TEXTURE_2D, shadowMaskTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, rttFramebuffer_width, rttFramebuffer_height, 0, GL_RGBA, GL_FLOAT, NULL); // Note: use GL_RGBA32F instead of GL_RGBA
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMaskTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	glBindTexture(GL_TEXTURE_2D, NULL);

	/* line 275 - */
	glGenTextures(1, &ltc_mat_texture);
	glBindTexture(GL_TEXTURE_2D, ltc_mat_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 64, 64, 0, GL_RGBA, GL_FLOAT, g_ltc_mat); // Note: use GL_RGBA32F instead of GL_RGBA
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &ltc_mag_texture);
	glBindTexture(GL_TEXTURE_2D, ltc_mag_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 64, 64, 0, GL_ALPHA, GL_FLOAT, g_ltc_mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	/* line - 283 */

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, NULL);
}

void Init(void)
{
	using namespace Configs;

	InitShaders();
	InitBuffers();
	InitScreenFrameBuffer();

	// For Tutorial16
	t16 = tutorial16();
	t16.loadOBJ("models/newObj.obj", t16.vertices, t16.uvs, t16.normals);
	t16.Init();

}

// Display Function
void display(void) {

	using namespace Configs;

	if (spin_mirror) {
		roty += 0.005 / demo_speed.speed;
		rotz += 0.008 / demo_speed.speed;
		if (spin_hw_toggle) {
			height += spin_height;
			width += spin_width;
		}
		if (height > 14 || height < 1)
			spin_height *= -1;
		if (width > 14 || width < 1)
			spin_width *= -1;
	}

	draw();

	if (mode == 1)
		printf("\r 8 edges: %.lfFPS (%.2lf ms/frame)", fps, execTime);
	else if (mode == 2)
		printf("\r12 edges: %.lfFPS (%.2lf ms/frame)", fps, execTime);
	else
		printf("\r 4 edges: %.lfFPS (%.2lf ms/frame)", fps, execTime);
	glFinish();
}

// Main Render Calls
void draw()
{
	using namespace Configs;
	/* line 555 - */
	parameters_time = std::clock();
	glEnable(GL_DEPTH_TEST);
	glClearColor(0, 0, 0, 0);

	// Note: the viewport is automatically set up to cover the entire Canvas.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	computeMatricesFromInputs();

	if (ymode == 0) {
		// Draw ShadowMap Result
		t16.RenderShadowMap16();
		//t16.RenderWithShadowMap16(shadowMaskBuffer);

		// Draw LTC Shading Result
		//t16.RenderSceneGeometryAlter(rttFramebuffer);
		
		// Draw LTC Shading Result With ShadowMap
		t16.RenderSceneGeometryAlterAlter(NULL);

		// Blend two results
		//t16.BlendShadowMask(rttTexture, shadowMaskTexture);
	}
	else if (ymode == 1)
	{
		// Draw LTC Shading Result without Gamma correction
		t16.RenderSceneGeometryAlter(NULL);

		//t16.BiltRender();

		// Draw Light
		//t16.RenderLightPosition(NULL);
	}
	else if (ymode == 2)
	{
		// Draw ShadowMap Result
		t16.RenderShadowMap16();
		t16.RenderWithShadowMap16(NULL);

		// Draw Light
		t16.RenderLightPosition(NULL, 0.01);
	}
	//else if (ymode == 3) {
	//	// Original One
	//	t16.RenderScene();
	//	t16.BiltRender();
	//}

	// Clean up
	glBindTexture(GL_TEXTURE_2D, NULL);
	glBindVertexArray(NULL);
	glUseProgram(NULL);

	updateFrameCount();
}

// Counting FPS
void updateFrameCount()
{
	using namespace Configs;
	frame++;
	time_of_glut = glutGet(GLUT_ELAPSED_TIME);

	if (time_of_glut - timebase > 1000)
	{
		fps = frame / ((double)(time_of_glut - timebase) / 1000.0);
		execTime = 1.0 / fps * 1000;
		timebase = time_of_glut;
		frame = 0;

		if (spin_mirror)
		{
			if (spin_mode_toggle)
			{
				++demo_speed.mode_counter;
			}

			if (demo_speed.mode_counter > 1)
			{
				mode = (mode + 1) % 3;
				demo_speed.mode_counter = 0;
			}
		}
	}
}

void reshape(int width, int height)
{
	using namespace Configs;
	parameters.screenWidth = width;
	parameters.screenHeight = height;
	glViewport(0, 0, width, height);
}

void idle(void)
{
	glutPostRedisplay();
}