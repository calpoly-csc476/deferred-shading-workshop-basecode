/* Base code for shadow mapping lab */
/* This code is incomplete - follow tasks listed in handout */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <random>
#include <cassert>
#include <cmath>
#include <stdio.h>
#include <time.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tiny_obj_loader/tiny_obj_loader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "Texture.h"
#include "WindowManager.h"
#include "Util.h"


using namespace std;
using namespace glm;



class Application : public EventCallbacks
{

public:

	const int NumLights = 100;
	const int NumTrees = 25;

	WindowManager * windowManager = nullptr;

	// Shaders
	shared_ptr<Program> SceneProg;
	shared_ptr<Program> SceneTexProg;
	shared_ptr<Program> LightProg;
	shared_ptr<Program> DebugProg;

	// Shapes
	shared_ptr<Shape> sphere;
	shared_ptr<Shape> dog;
	shared_ptr<Shape> dragon;
	shared_ptr<Shape> stairs;
	shared_ptr<Shape> tree;

	shared_ptr<Texture> treeTexture;

	// Debug Settings
	bool ShowSceneColor = false;
	bool ShowSceneNormals = false;
	bool ShowSceneDepth = false;
	bool ShowScenePosition = false;

	GLuint SceneFBO;
	GLuint SceneColorTexture;
	GLuint SceneNormalsTexture;
	GLuint SceneDepthTexture;

	int g_width = -1;
	int g_height = -1;

	// Ground Plane vertex data
	GLuint GroundVertexArray;
	int GroundIndexCount;

	// Screen Quad vertex data (for debugging)
	GLuint QuadVertexArray;
	GLuint QuadVertexBuffer;

	struct Light
	{
		vec3 Position;
		vec3 Color;
		float T;
	};

	vector<Light> Lights;


	/////////////////
	// Camera Data //
	/////////////////

	// Previous frame start time (for time-based movement)
	float t0 = 0;

	vec3 cameraLookAt;

	float cTheta = - 3.14159f / 2.f;
	float cPhi = 0;
	bool mouseDown = false;

	double lastX = 0;
	double lastY = 0;
	float cameraRotateSpeed = 0.005f;

	bool moveForward = false;
	bool moveBack = false;
	bool moveLeft = false;
	bool moveRight = false;
	vec3 cameraPos = vec3(0, 1.5f, 8);
	float cameraMoveSpeed = 12.0f;


	/////////////////////
	// Event Callbacks //
	/////////////////////

	void mouseCallback(GLFWwindow* window, int but, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			mouseDown = true;
		}

		if (action == GLFW_RELEASE)
		{
			mouseDown = false;
		}
	}

	void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		if (mouseDown)
		{
			cTheta += (float) (xpos - lastX) * cameraRotateSpeed;
			cPhi -= (float) (ypos - lastY) * cameraRotateSpeed;
			cPhi = glm::max(cPhi, -3.1415f / 2.f);
			cPhi = glm::min(cPhi, 3.1415f / 2.f);
		}

		lastX = xpos;
		lastY = ypos;
	}

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		switch (key)
		{
		case GLFW_KEY_W:
			moveForward = (action != GLFW_RELEASE);
			break;
		case GLFW_KEY_S:
			moveBack = (action != GLFW_RELEASE);
			break;
		case GLFW_KEY_A:
			moveLeft = (action != GLFW_RELEASE);
			break;
		case GLFW_KEY_D:
			moveRight = (action != GLFW_RELEASE);
			break;

		case GLFW_KEY_H:
			ShowSceneColor = (action != GLFW_RELEASE);
			break;
		case GLFW_KEY_J:
			ShowSceneNormals = (action != GLFW_RELEASE);
			break;
		case GLFW_KEY_K:
			ShowSceneDepth = (action != GLFW_RELEASE);
			break;
		case GLFW_KEY_L:
			ShowScenePosition = (action != GLFW_RELEASE);
			break;
		};

		if (action == GLFW_PRESS)
		{
			switch (key)
			{
			case GLFW_KEY_1:
				cameraMoveSpeed = 1.f;
				break;
			case GLFW_KEY_2:
				cameraMoveSpeed = 3.f;
				break;
			case GLFW_KEY_3:
				cameraMoveSpeed = 6.f;
				break;
			case GLFW_KEY_4:
				cameraMoveSpeed = 12.f;
				break;
			case GLFW_KEY_5:
				cameraMoveSpeed = 24.f;
				break;
			}
		}
	}

	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY)
	{
	}

	void resizeCallback(GLFWwindow* window, int w, int h)
	{
		CHECKED_GL_CALL(glViewport(0, 0, g_width = w, g_height = h));
		setGBufferTextureSize(g_width, g_height);
	}



	///////////////////////
	// Geometry Creation //
	///////////////////////

	// Create Geometry
	void initGeom()
	{
		initGround();
		initQuad();
	}

	// Create the ground plane
	void initGround()
	{
		const float groundSize = 200;
		const float groundY = 0;

		// A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
		const float GrndPos[] =
		{
			-groundSize, groundY, -groundSize,
			-groundSize, groundY,  groundSize,
			 groundSize, groundY,  groundSize,
			 groundSize, groundY, -groundSize,
		};

		const float GrndNorm[] =
		{
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
		};

		unsigned short idx[] = { 0, 1, 2, 0, 2, 3 };

		CHECKED_GL_CALL(glGenVertexArrays(1, &GroundVertexArray));
		CHECKED_GL_CALL(glBindVertexArray(GroundVertexArray));

		GLuint GroundPositionBuffer, GroundNormalBuffer, GroundIndexBuffer;

		CHECKED_GL_CALL(glGenBuffers(1, &GroundPositionBuffer));
		CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, GroundPositionBuffer));
		CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW));
		CHECKED_GL_CALL(glEnableVertexAttribArray(0));
		CHECKED_GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0));

		CHECKED_GL_CALL(glGenBuffers(1, &GroundNormalBuffer));
		CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, GroundNormalBuffer));
		CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW));
		CHECKED_GL_CALL(glEnableVertexAttribArray(1));
		CHECKED_GL_CALL(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0));

		GroundIndexCount = 6;
		CHECKED_GL_CALL(glGenBuffers(1, &GroundIndexBuffer));
		CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GroundIndexBuffer));
		CHECKED_GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW));

		CHECKED_GL_CALL(glBindVertexArray(0));
	}

	// geometry set up for a quad
	void initQuad()
	{
		// now set up a simple quad for rendering FBO
		glGenVertexArrays(1, &QuadVertexArray);
		glBindVertexArray(QuadVertexArray);

		const GLfloat g_quad_vertex_buffer_data[] =
		{
			-1.0f, -1.0f,  0.0f,
			 1.0f, -1.0f,  0.0f,
			-1.0f,  1.0f,  0.0f,
			-1.0f,  1.0f,  0.0f,
			 1.0f, -1.0f,  0.0f,
			 1.0f,  1.0f,  0.0f,
		};

		glGenBuffers(1, &QuadVertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, QuadVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);

		glBindVertexArray(0);
	}



	///////////
	// Setup //
	///////////

	void setGBufferTextureSize(const int width, const int height)
	{
		glBindTexture(GL_TEXTURE_2D, SceneColorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

		glBindTexture(GL_TEXTURE_2D, SceneNormalsTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

		glBindTexture(GL_TEXTURE_2D, SceneDepthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	}

	// set up the FBO for GBuffer (scene colors, normals, and a depth texture)
	void initGBuffer()
	{
		// generate the FBO for the shadow depth
		glGenFramebuffers(1, &SceneFBO);

		// generate the color texture
		glGenTextures(1, &SceneColorTexture);
		glBindTexture(GL_TEXTURE_2D, SceneColorTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// generate the normals texture
		glGenTextures(1, &SceneNormalsTexture);
		glBindTexture(GL_TEXTURE_2D, SceneNormalsTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// generate the depth texture
		glGenTextures(1, &SceneDepthTexture);
		glBindTexture(GL_TEXTURE_2D, SceneDepthTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		setGBufferTextureSize(g_width, g_height);

		// bind with framebuffer's depth buffer
		glBindFramebuffer(GL_FRAMEBUFFER, SceneFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SceneColorTexture, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, SceneNormalsTexture, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, SceneDepthTexture, 0);

		const GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, drawBuffers);

		uint Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (Status != GL_FRAMEBUFFER_COMPLETE)
		{
			cerr << "Failed to properly create framebuffer!" << endl;

			string Problem;
			switch (Status)
			{
			case GL_FRAMEBUFFER_UNDEFINED:
				Problem = "Undefined - the specified framebuffer is the default read or draw framebuffer, but the default framebuffer does not exist.";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				Problem = "Incomplete attachment - one or more of the framebuffer attachment points are framebuffer incomplete.";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				Problem = "Incomplete missing attachment - the framebuffer does not have at least one image attached to it.";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
				Problem = "Incomplete draw buffer - the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for one or more color attachment point(s) named by GL_DRAW_BUFFERi.";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
				Problem = "Incomplete read buffer - GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.";
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED:
				Problem = "Unsupported - the combination of internal formats of the attached images violates an implementation-dependent set of restrictions.";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
				Problem = "Incomplete multisample.";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
				Problem = "Incomplete layer targets - one or more framebuffer attachments is layered, and one or more populated attachment is not layered, or all populated color attachments are not from textures of the same target.";
				break;
			default:
				Problem = "Unknown.";
				break;
			}

			cerr << "Framebuffer problem: " << Problem << endl;
			exit(1);
		}

		CHECKED_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}

	void init(string const & RESOURCE_DIR)
	{
		GLSL::checkVersion();

		glfwGetFramebufferSize(windowManager->getHandle(), &g_width, &g_height);

		// Set background color
		glClearColor(0.f, 0.f, 0.f, 1.0f);
		// Enable z-buffer test
		glEnable(GL_DEPTH_TEST);

		// Initialize mesh
		dog = make_shared<Shape>();
		dog->loadMesh(RESOURCE_DIR + "dog.obj");
		dog->resize();
		dog->init();

		sphere = make_shared<Shape>();
		sphere->loadMesh(RESOURCE_DIR + "sphere.obj");
		sphere->resize();
		sphere->init();

		dragon = make_shared<Shape>();
		dragon->loadMesh(RESOURCE_DIR + "dragon10k.obj");
		dragon->resize();
		dragon->init();

		stairs = make_shared<Shape>();
		stairs->loadMesh(RESOURCE_DIR + "staircase.obj");
		stairs->resize();
		stairs->init();

		tree = make_shared<Shape>();
		tree->loadMesh(RESOURCE_DIR + "PineTree3.obj");
		tree->resize();
		tree->init();

		treeTexture = make_shared<Texture>();
		treeTexture->setFilename(RESOURCE_DIR + "PineTexture.png");
		treeTexture->init();
		treeTexture->setUnit(0);

		// Initialize the GLSL programs

		SceneProg = make_shared<Program>();
		SceneProg->setVerbose(true);
		SceneProg->setShaderNames(RESOURCE_DIR + "scene_vert.glsl", RESOURCE_DIR + "scene_frag.glsl");
		if (! SceneProg->init())
		{
			exit(1);
		}

		SceneTexProg = make_shared<Program>();
		SceneTexProg->setVerbose(true);
		SceneTexProg->setShaderNames(RESOURCE_DIR + "sceneTex_vert.glsl", RESOURCE_DIR + "sceneTex_frag.glsl");
		if (! SceneTexProg->init())
		{
			exit(1);
		}

		LightProg = make_shared<Program>();
		LightProg->setVerbose(true);
		LightProg->setShaderNames(RESOURCE_DIR + "light_vert.glsl", RESOURCE_DIR + "light_frag.glsl");
		if (! LightProg->init())
		{
			exit(1);
		}

		DebugProg = make_shared<Program>();
		DebugProg->setVerbose(true);
		DebugProg->setShaderNames(RESOURCE_DIR + "debug_vert.glsl", RESOURCE_DIR + "debug_frag.glsl");
		if (! DebugProg->init())
		{
			exit(1);
		}

		////////////////////////
		// Intialize textures //
		////////////////////////

		SceneProg->addUniform("P");
		SceneProg->addUniform("M");
		SceneProg->addUniform("V");
		SceneProg->addAttribute("vertPos");
		SceneProg->addAttribute("vertNor");
		SceneProg->addUniform("materialColor");

		SceneTexProg->addUniform("P");
		SceneTexProg->addUniform("M");
		SceneTexProg->addUniform("V");
		SceneTexProg->addAttribute("vertPos");
		SceneTexProg->addAttribute("vertNor");
		SceneTexProg->addAttribute("vertTex");
		SceneTexProg->addUniform("materialTex");

		LightProg->addUniform("sceneColorTex");
		LightProg->addUniform("sceneNormalsTex");
		LightProg->addUniform("sceneDepthTex");
		LightProg->addUniform("P");
		LightProg->addUniform("V");
		LightProg->addUniform("M");
		LightProg->addUniform("lightColor");
		LightProg->addUniform("lightPos");
		LightProg->addUniform("invP");
		LightProg->addUniform("invV");
		LightProg->addUniform("cameraPos");
		LightProg->addAttribute("vertPos");
		LightProg->addAttribute("vertNor");

		DebugProg->addUniform("sceneColorTex");
		DebugProg->addUniform("sceneNormalsTex");
		DebugProg->addUniform("sceneDepthTex");
		DebugProg->addUniform("uMode");
		DebugProg->addUniform("invP");
		DebugProg->addUniform("invV");
		DebugProg->addAttribute("vertPos");

		initGBuffer();

		///////////////////////
		// Initialize lights //
		///////////////////////


		srand(100);
		for (int i = 0; i < NumLights; ++ i)
		{
			Light l;
			l.Position = vec3(nrand() * 30.f, 1.5f + frand() * 0.5f, nrand() * 30.f);
			l.Color = HSV(frand(), 1.f, 1.f);
			l.T = frand() * 6.28f;
			Lights.push_back(l);
		}
	}



	////////////////
	// Transforms //
	////////////////

	mat4 SetProjectionMatrix(shared_ptr<Program> curShade)
	{
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float) height;

		mat4 Projection = perspective(radians(50.0f), aspect, 0.1f, 200.0f);
		
		if (curShade)
		{
			CHECKED_GL_CALL(glUniformMatrix4fv(curShade->getUniform("P"), 1, GL_FALSE, value_ptr(Projection)));
		}
		return Projection;
	}

	mat4 SetView(shared_ptr<Program> curShade)
	{
		mat4 Cam = glm::lookAt(cameraPos, cameraLookAt, vec3(0, 1, 0));
		if (curShade)
		{
			CHECKED_GL_CALL(glUniformMatrix4fv(curShade->getUniform("V"), 1, GL_FALSE, value_ptr(Cam)));
		}
		return Cam;
	}

	void SetModel(vec3 trans, float rotY, float sc, shared_ptr<Program> curS)
	{
		mat4 Trans = glm::translate(glm::mat4(1.0f), trans);
		mat4 Rot = glm::rotate(glm::mat4(1.0f), rotY, vec3(0, 1, 0));
		mat4 Scale = glm::scale(glm::mat4(1.0f), vec3(sc));
		mat4 ctm = Trans * Rot * Scale;
		CHECKED_GL_CALL(glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm)));
	}



	////////////
	// Render //
	////////////

	// Draw the dog, sphere, dragon, and stairs and ground plane
	void DrawScene()
	{
		SceneProg->bind();

		SetProjectionMatrix(SceneProg);
		SetView(SceneProg);

		// draw the dog mesh
		SetModel(vec3(-1, 0.75f, 0), 0, 1, SceneProg);
		CHECKED_GL_CALL(glUniform3f(SceneProg->getUniform("materialColor"), 0.8f, 0.2f, 0.2f));
		dog->draw(SceneProg);

		// draw the sphere mesh
		SetModel(vec3(1, 0.5f, 0), 0, 1, SceneProg);
		CHECKED_GL_CALL(glUniform3f(SceneProg->getUniform("materialColor"), 0.2f, 0.2f, 0.8f));
		sphere->draw(SceneProg);

		// draw the dragon mesh
		SetModel(vec3(-1, 2.f, -4), radians(90.f), 3, SceneProg);
		CHECKED_GL_CALL(glUniform3f(SceneProg->getUniform("materialColor"), 0.2f, 0.8f, 0.8f));
		dragon->draw(SceneProg);

		// draw the stairs mesh
		SetModel(vec3(1, 2.5f, -9), radians(180.f), 3, SceneProg);
		CHECKED_GL_CALL(glUniform3f(SceneProg->getUniform("materialColor"), 0.8f, 0.6f, 0.2f));
		stairs->draw(SceneProg);

		// draw the ground plane
		SetModel(vec3(0, 0, 0), 0, 1, SceneProg);
		CHECKED_GL_CALL(glUniform3f(SceneProg->getUniform("materialColor"), 0.8f, 0.8f, 0.8f));
		CHECKED_GL_CALL(glBindVertexArray(GroundVertexArray));
		CHECKED_GL_CALL(glDrawElements(GL_TRIANGLES, GroundIndexCount, GL_UNSIGNED_SHORT, 0));
		CHECKED_GL_CALL(glBindVertexArray(0));

		SceneProg->unbind();


		SceneTexProg->bind();

		SetProjectionMatrix(SceneTexProg);
		SetView(SceneTexProg);

		// draw trees
		treeTexture->bind(SceneTexProg->getUniform("materialTex"));
		srand(200);
		for (int i = 0; i < NumTrees; ++ i)
		{
			const float angle = 3.14159f * 2 * frand();
			const float theta = 3.14159f * 2 * (float) i / (float) NumTrees  + frand() * 0.5f;
			const float radius = 13.f + 22.f * frand();
			const float scale = 7.f + 6.f * frand();

			const vec3 Pos = vec3(cos(theta) * radius, scale, sin(theta) * radius);

			SetModel(Pos, angle, scale, SceneProg);
			tree->draw(SceneTexProg);
		}

		SceneTexProg->unbind();
	}

	void UpdateCamera(float const dT)
	{

		glm::vec3 up = glm::vec3(0, 1, 0);
		glm::vec3 forward = glm::vec3(cos(cTheta) * cos(cPhi), sin(cPhi), sin(cTheta) * cos(cPhi));
		glm::vec3 right = glm::normalize(glm::cross(forward, up));

		if (moveForward)
			cameraPos += forward * cameraMoveSpeed * dT;
		if (moveBack)
			cameraPos -= forward * cameraMoveSpeed * dT;
		if (moveLeft)
			cameraPos -= right * cameraMoveSpeed * dT;
		if (moveRight)
			cameraPos += right * cameraMoveSpeed * dT;

		cameraLookAt = cameraPos + forward;
	}
	
	/* let's draw */
	void render()
	{
		float t1 = (float) glfwGetTime();

		float const dT = (t1 - t0);
		t0 = t1;

		UpdateCamera(dT);


		// First render the scene into the gbuffer

		CHECKED_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, SceneFBO));
		CHECKED_GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		DrawScene();


		// Now do the light pass - read gbuffer values to do illumination (or just copy values to screen for debug)

		CHECKED_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		CHECKED_GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));


		int PassMode = 0;

		if (ShowSceneColor)
		{
			PassMode = 1;
		}
		else if (ShowSceneNormals)
		{
			PassMode = 2;
		}
		else if (ShowSceneDepth)
		{
			PassMode = 3;
		}
		else if (ShowScenePosition)
		{
			PassMode = 4;
		}

		if (PassMode)
		{
			// Debug mode - just copy

			DebugProg->bind();
			CHECKED_GL_CALL(glActiveTexture(GL_TEXTURE0));
			CHECKED_GL_CALL(glBindTexture(GL_TEXTURE_2D, SceneColorTexture));
			CHECKED_GL_CALL(glUniform1i(DebugProg->getUniform("sceneColorTex"), 0));

			CHECKED_GL_CALL(glActiveTexture(GL_TEXTURE1));
			CHECKED_GL_CALL(glBindTexture(GL_TEXTURE_2D, SceneNormalsTexture));
			CHECKED_GL_CALL(glUniform1i(DebugProg->getUniform("sceneNormalsTex"), 1));

			CHECKED_GL_CALL(glActiveTexture(GL_TEXTURE2));
			CHECKED_GL_CALL(glBindTexture(GL_TEXTURE_2D, SceneDepthTexture));
			CHECKED_GL_CALL(glUniform1i(DebugProg->getUniform("sceneDepthTex"), 2));

			CHECKED_GL_CALL(glUniform1i(DebugProg->getUniform("uMode"), PassMode));

			mat4 P = SetProjectionMatrix(nullptr);
			mat4 V = SetView(nullptr);
			mat4 invP = inverse(P);
			mat4 invV = inverse(V);
			CHECKED_GL_CALL(glUniformMatrix4fv(DebugProg->getUniform("invP"), 1, GL_FALSE, value_ptr(invP)));
			CHECKED_GL_CALL(glUniformMatrix4fv(DebugProg->getUniform("invV"), 1, GL_FALSE, value_ptr(invV)));

			CHECKED_GL_CALL(glBindVertexArray(QuadVertexArray));
			CHECKED_GL_CALL(glDrawArrays(GL_TRIANGLES, 0, 6));
			DebugProg->unbind();
		}
		else
		{
			// Lighting mode

			LightProg->bind();

			mat4 P = SetProjectionMatrix(LightProg);
			mat4 V = SetView(LightProg);

			mat4 invP = inverse(P);
			mat4 invV = inverse(V);
			CHECKED_GL_CALL(glUniformMatrix4fv(LightProg->getUniform("invP"), 1, GL_FALSE, value_ptr(invP)));
			CHECKED_GL_CALL(glUniformMatrix4fv(LightProg->getUniform("invV"), 1, GL_FALSE, value_ptr(invV)));
			CHECKED_GL_CALL(glUniform3f(LightProg->getUniform("cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z));

			CHECKED_GL_CALL(glActiveTexture(GL_TEXTURE0));
			CHECKED_GL_CALL(glBindTexture(GL_TEXTURE_2D, SceneColorTexture));
			CHECKED_GL_CALL(glUniform1i(LightProg->getUniform("sceneColorTex"), 0));

			CHECKED_GL_CALL(glActiveTexture(GL_TEXTURE1));
			CHECKED_GL_CALL(glBindTexture(GL_TEXTURE_2D, SceneNormalsTexture));
			CHECKED_GL_CALL(glUniform1i(LightProg->getUniform("sceneNormalsTex"), 1));

			CHECKED_GL_CALL(glActiveTexture(GL_TEXTURE2));
			CHECKED_GL_CALL(glBindTexture(GL_TEXTURE_2D, SceneDepthTexture));
			CHECKED_GL_CALL(glUniform1i(LightProg->getUniform("sceneDepthTex"), 2));

			// Blend mode for additive
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_ONE, GL_ONE);

			// Culling for 3D spheres - want 2D coverage
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);

			// No depth - light volumes shouldn't block other lights
			glDisable(GL_DEPTH_TEST);
			
			for (int i = 0; i < Lights.size(); ++ i)
			{
				vec3 Pos = Lights[i].Position + vec3(
					cos(0.2f * t1 + Lights[i].T) * 5.f,
					sin(t1 + Lights[i].T),
					sin(0.2f * t1 + Lights[i].T) * 5.f);

				SetModel(Pos, 0, 0.5f, LightProg);
				CHECKED_GL_CALL(glUniform3f(LightProg->getUniform("lightPos"), Pos.x, Pos.y, Pos.z));
				CHECKED_GL_CALL(glUniform3f(LightProg->getUniform("lightColor"), Lights[i].Color.x, Lights[i].Color.y, Lights[i].Color.z));
				sphere->draw(LightProg);
			}

			// Reset state
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);

			LightProg->unbind();
		}
	}

};

int main(int argc, char **argv)
{
	std::string resourceDir = "../resources/";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	WindowManager *windowManager = new WindowManager();
	windowManager->init(1024, 768);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	application->init(resourceDir);
	application->initGeom();

	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		application->render();
		glfwSwapBuffers(windowManager->getHandle());
		glfwPollEvents();
	}

	windowManager->shutdown();
	return 0;
}
