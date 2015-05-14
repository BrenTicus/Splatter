#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <Windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "model.h"
#include "tinyfiledialogs.h"

using namespace std;
using namespace glm;

struct UniformsBlock
{
	mat4 mv_matrix;
	mat4 view_matrix;
	mat4 proj_matrix;
};

struct ShaderInfo
{
	GLuint vao;
	GLuint vbo;
	GLuint uniforms;
	GLuint shaderProgram;
};

Model* model;

// Constants for the shader files.
const char* vertexFile = "vertex.glsl";
const char* fragmentFile = "fragment.glsl";

// Variables to hold values for manipulating the model.
const float rotationSpeed = 1.0f;
const float scaleStep = 0.02f;

// Matrices for M-V-P information.
mat4 modelMatrix;
mat4 viewMatrix;
mat4 projMatrix;

/*
Loads the shader from the given file.
*/
bool loadShaderFile(const char *filename, GLuint shader) 
{
	GLint shaderLength = 0;
	GLubyte shaderText[8192];
	FILE *fp;

	// Open the shader file
	fp = fopen(filename, "r");
	if (fp != NULL) {
		// See how long the file is
		while (fgetc(fp) != EOF)
			shaderLength++;

		// Go back to beginning of file
		rewind(fp);

		// Read the whole file in
		fread(shaderText, 1, shaderLength, fp);

		// Make sure it is null terminated and close the file
		shaderText[shaderLength] = '\0';
		fclose(fp);
	}
	else 
	{
		return false;
	}

	// Load the string into the shader object
	GLchar* fsStringPtr[1];
	fsStringPtr[0] = (GLchar *)((const char*)shaderText);
	glShaderSource(shader, 1, (const GLchar **)fsStringPtr, NULL);

	return true;
}

/*
Render the currently loaded model.
*/
void renderScene(ShaderInfo* shader, UniformsBlock* mvpMats) 
{
	// Clear the window and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shader->shaderProgram);

	// Send required uniforms in
	GLuint location = glGetUniformLocation(shader->shaderProgram, "modelLength");
	glUniform1f(location, model->getLength());
	location = glGetUniformLocation(shader->shaderProgram, "modelScale");
	glUniform1f(location, model->getScale());

	// Shove the MVP matrices into the shader
	GLuint bindingPoint = 1, blockIndex;
	blockIndex = glGetUniformBlockIndex(shader->shaderProgram, "constants");
	glUniformBlockBinding(shader->shaderProgram, blockIndex, bindingPoint);
	glBindBuffer(GL_UNIFORM_BUFFER, shader->uniforms);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformsBlock), mvpMats , GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, shader->uniforms);

	glBindVertexArray(shader->vao);

	glDrawArrays(GL_POINTS, 0, model->getVertices()->size());
}


/*
Set up the shaders and buffers for rendering.
*/
void setupRenderingContext(ShaderInfo* shader) 
{
	// Background and openGL settings.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPointParameterf(GL_POINT_SIZE_MAX, 100.0f);
	glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, 100.0f);

	// Set up and compile the shaders.
	GLuint hVertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint hFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	GLint testVal;

	if (!loadShaderFile(vertexFile, hVertexShader)) {
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		cout << "The shader " << vertexFile << " could not be found." << endl;
	}

	if (!loadShaderFile(fragmentFile, hFragmentShader)) {
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		cout << "The shader " << fragmentFile << " could not be found." << endl;
	}

	glCompileShader(hVertexShader);
	glCompileShader(hFragmentShader);

	// Check for any error generated during shader compilation
	glGetShaderiv(hVertexShader, GL_COMPILE_STATUS, &testVal);
	if (testVal == GL_FALSE) {
		char source[8192];
		char infoLog[8192];
		glGetShaderSource(hVertexShader, 8192, NULL, source);
		glGetShaderInfoLog(hVertexShader, 8192, NULL, infoLog);
		cout << "The shader: " << endl << (const char*)source << endl << " failed to compile:" << endl;
		fprintf(stderr, "%s\n", infoLog);
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
	}
	glGetShaderiv(hFragmentShader, GL_COMPILE_STATUS, &testVal);
	if (testVal == GL_FALSE) {
		char source[8192];
		char infoLog[8192];
		glGetShaderSource(hFragmentShader, 8192, NULL, source);
		glGetShaderInfoLog(hFragmentShader, 8192, NULL, infoLog);
		cout << "The shader: " << endl << (const char*)source << endl << " failed to compile:" << endl;
		fprintf(stderr, "%s\n", infoLog);
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
	}

	// Create and link the shader program.
	shader->shaderProgram = glCreateProgram();
	glAttachShader(shader->shaderProgram, hVertexShader);
	glAttachShader(shader->shaderProgram, hFragmentShader);

	glLinkProgram(shader->shaderProgram);
	glDeleteShader(hVertexShader);
	glDeleteShader(hFragmentShader);
	glGetProgramiv(shader->shaderProgram, GL_LINK_STATUS, &testVal);
	if (testVal == GL_FALSE) {
		char infoLog[1024];
		glGetProgramInfoLog(shader->shaderProgram, 1024, NULL, infoLog);
		cout << "The shader program" << "(" << vertexFile << " + " << fragmentFile << ") failed to link:" << endl << (const char*)infoLog << endl;
		glDeleteProgram(shader->shaderProgram);
		shader->shaderProgram = (GLuint)NULL;
	}

	// Setup buffer objects for when the model is to be loaded.
	glGenVertexArrays(1, &shader->vao);
	glBindVertexArray(shader->vao);

	glGenBuffers(1, &shader->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, shader->vbo);

	glGenBuffers(1, &shader->uniforms);

	cout << "Shader set up.\n";
}

/*
Load in a new model to display.
*/
void loadModel(string filename)
{
	if (model) delete model;
	model = new Model(filename);

	vec3 midpoint = model->getMidpoint();

	modelMatrix = mat4(1.0f);
	viewMatrix = lookAt(vec3(midpoint.x, midpoint.y, midpoint.z + 2 * model->getLength()), midpoint, vec3(0, 1, 0));
	projMatrix = perspective(60.0f, 4.0f / 3.0f, 0.1f, 5 * model->getLength());

	// Buffering vertex data, normals, and the size of each point.
	glBufferData(GL_ARRAY_BUFFER, model->getVertices()->size() * sizeof(model->getVertices()->at(0)) + model->getNormals()->size() * sizeof(model->getNormals()->at(0)) + model->getSizes()->size() * sizeof(model->getSizes()->at(0)), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, model->getVertices()->size() * sizeof(model->getVertices()->at(0)), &model->getVertices()->at(0));
	glBufferSubData(GL_ARRAY_BUFFER, model->getVertices()->size() * sizeof(model->getVertices()->at(0)), model->getNormals()->size() * sizeof(model->getNormals()->at(0)), &model->getNormals()->at(0));
	glBufferSubData(GL_ARRAY_BUFFER, model->getVertices()->size() * sizeof(model->getVertices()->at(0)) + model->getNormals()->size() * sizeof(model->getNormals()->at(0)), model->getSizes()->size() * sizeof(model->getSizes()->at(0)), &model->getSizes()->at(0));

	// Constants to help with location bindings
	#define VERTEX_DATA 0
	#define VERTEX_NORMAL 1
	#define POINT_SIZE 2

	glEnableVertexAttribArray(VERTEX_DATA);
	glVertexAttribPointer(VERTEX_DATA, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(VERTEX_NORMAL);
	glVertexAttribPointer(VERTEX_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(model->getVertices()->size() * sizeof(model->getVertices()->at(0))));
	glEnableVertexAttribArray(POINT_SIZE);
	glVertexAttribPointer(POINT_SIZE, 1, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(model->getVertices()->size() * sizeof(model->getVertices()->at(0)) + model->getNormals()->size() * sizeof(model->getNormals()->at(0))));
}

/*
Keyboard callback function.
*/
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Close window
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (model)
	{
		modelMatrix *= translate(model->getMidpoint());
		// Model controls
		if (key == GLFW_KEY_UP && (action || 3))
			modelMatrix *= rotate(rotationSpeed, vec3(1, 0, 0));
		if (key == GLFW_KEY_DOWN && (action || 3))
			modelMatrix *= rotate(-rotationSpeed, vec3(1, 0, 0));
		if (key == GLFW_KEY_LEFT && (action || 3))
			modelMatrix *= rotate(-rotationSpeed, vec3(0, 1, 0));
		if (key == GLFW_KEY_RIGHT && (action || 3))
			modelMatrix *= rotate(rotationSpeed, vec3(0, 1, 0));
		if (key == GLFW_KEY_A && (action || 3))
			modelMatrix *= rotate(rotationSpeed, vec3(0, 0, 1));
		if (key == GLFW_KEY_D && (action || 3))
			modelMatrix *= rotate(-rotationSpeed, vec3(0, 0, 1));
		modelMatrix *= translate(-model->getMidpoint());
		if (key == GLFW_KEY_W && (action || 3))
			model->setScale(model->getScale() + scaleStep);
		if (key == GLFW_KEY_S && (action || 3))
			model->setScale(model->getScale() - scaleStep);
		viewMatrix = translate(viewMatrix, model->getMidpoint());
		if (key == GLFW_KEY_Z && (action || 3))
			viewMatrix *= rotate(rotationSpeed, vec3(0, 1, 0));
		if (key == GLFW_KEY_X && (action || 3))
			viewMatrix *= rotate(-rotationSpeed, vec3(0, 1, 0));
		viewMatrix = translate(viewMatrix, -model->getMidpoint());
	}
	if (key == GLFW_KEY_O && action == GLFW_PRESS)
	{
		const char* const filter = "*.obj";
		const char* filename = tinyfd_openFileDialog("Load OBJ", NULL, 1, &filter, 0);
		if (filename)
		{
			loadModel(filename);
			glfwSetWindowTitle(window, ("Splatter - " + (string)filename).c_str());
		}
	}
}

/*
Error callback function.
*/
static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}


/*
Program entry point.
*/
int main(int argc, char** argv)
{
	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) 
	{
		cout << "GLFW initialization failed. Aborting" << endl;
		cin.ignore();
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_SAMPLES, 4);	//Anti-aliasing - This will have smooth polygon edges

	window = glfwCreateWindow(800, 600, "Splatter", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		cout << "Window creation failed. Aborting." << endl;
		cin.ignore();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		//Problem: glewInit failed, something is seriously wrong.
		cout << "glewInit failed. Aborting." << endl;
		cin.ignore();
		exit(EXIT_FAILURE);
	}

	ShaderInfo shader = { 0, 0, 0 };
	UniformsBlock mvpMats = { mat4(1.0f), mat4(1.0f), mat4(1.0f) };

	setupRenderingContext(&shader);

	if (shader.shaderProgram) {
		while (!glfwWindowShouldClose(window))
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			glViewport(0, 0, width, height);


			if (model)
			{
				mvpMats.mv_matrix = viewMatrix * scale(modelMatrix, vec3(model->getScale()));
				mvpMats.proj_matrix = projMatrix;
				mvpMats.view_matrix = viewMatrix;

				renderScene(&shader, &mvpMats);
			}

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}

	delete model;

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
