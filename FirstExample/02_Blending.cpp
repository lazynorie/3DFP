
///////////////////////////////////////////////////////////////////////
//
// 01_JustAmbient.cpp
//
///////////////////////////////////////////////////////////////////////

using namespace std;

#include <cstdlib>
#include <ctime>
#include "vgl.h"
#include "LoadShaders.h"
#include "Light.h"
#include "Shape.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FPS 60
#define MOVESPEED 0.1f
#define TURNSPEED 0.05f
#define X_AXIS glm::vec3(1,0,0)
#define Y_AXIS glm::vec3(0,1,0)
#define Z_AXIS glm::vec3(0,0,1)
#define XY_AXIS glm::vec3(1,1,0)
#define YZ_AXIS glm::vec3(0,1,1)
#define XZ_AXIS glm::vec3(1,0,1)


enum keyMasks {
	KEY_FORWARD =  0b00000001,		// 0x01 or 1 or 01
	KEY_BACKWARD = 0b00000010,		// 0x02 or 2 or 02
	KEY_LEFT = 0b00000100,		
	KEY_RIGHT = 0b00001000,
	KEY_UP = 0b00010000,
	KEY_DOWN = 0b00100000,
	KEY_MOUSECLICKED = 0b01000000
	// Any other keys you want to add.
};

// IDs.
GLuint vao, ibo, points_vbo, colors_vbo, uv_vbo, normals_vbo, modelID, viewID, projID;
GLuint program;

// Matrices.
glm::mat4 View, Projection;

// Our bitflags. 1 byte for up to 8 keys.
unsigned char keys = 0; // Initialized to 0 or 0b00000000.

// Camera and transform variables.
float scale = 1.0f, angle = 0.0f;
glm::vec3 position, frontVec, worldUp, upVec, rightVec; // Set by function
GLfloat pitch, yaw;
int lastX, lastY;

// Texture variables.
GLuint firstTx, secondTx, blankTx, brickTx ,hedgeTx,roomTx;
GLint width, height, bitDepth;

// Light variables.
AmbientLight aLight(glm::vec3(1.0f, 1.0f, 1.0f),	// Ambient colour.
	0.5f);							// Ambient strength.
DirectionalLight dLight(glm::vec3(-1.0f, 0.0f, -0.5f), // Direction.
	glm::vec3(1.0f, 1.0f, 0.25f),  // Diffuse colour.
	0.5f);						  // Diffuse strength.

PointLight pLights[5] = { { glm::vec3(-1.0f, 6.0f, -1.0f), 10.0f, glm::vec3(1.0f, 1.0f, 1.0f), 10.0f },
						  { glm::vec3(24.0f, 6.0f, -26.0f), 10.0f, glm::vec3(1.0f, 1.0f, 1.0f), 10.0f },
						  { glm::vec3(-1.0f, 6.0f, -26.0f), 10.0f, glm::vec3(1.0f, 1.0f, 1.0f), 10.0f} ,
						  { glm::vec3(24.0f, 6.0f, -1.0f), 10.0f, glm::vec3(1.0f, 1.0f, 1.0f), 10.0f },
						  { glm::vec3(2.5f, 0.5f, -5.0f), 10.0f, glm::vec3(1.0f, 1.0f, 1.0f), 10.0f } };



void timer(int);

void resetView()
{
	position = glm::vec3(10.0f, 10.0f, 10.0f);
	frontVec = glm::vec3(0.0f, 0.0f, -1.0f);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	pitch = 0.0f;
	yaw = -90.0f;
	// View will now get set only in transformObject
}

// Shapes. Recommend putting in a map
Cube g_cube(15);
Prism g_tower(24);
Grid g_grid(25,1); // New UV scale parameter. Works with texture now.
Cube g_wall(15);
Cube g_gatehouse(10);
Cube m_wall(5);
Cone t_top(5);

void init(void)
{
	srand((unsigned)time(NULL));
	//Specifying the name of vertex and fragment shaders.
	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};

	//Loading and compiling shaders
	program = LoadShaders(shaders);
	glUseProgram(program);	//My Pipeline is set up

	modelID = glGetUniformLocation(program, "model");
	projID = glGetUniformLocation(program, "projection");
	viewID = glGetUniformLocation(program, "view");

	// Projection matrix : 45∞ Field of View, aspect ratio, display range : 0.1 unit <-> 100 units
	Projection = glm::perspective(glm::radians(45.0f), 1.0f / 1.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	// Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	resetView();

	// Image loading.
	stbi_set_flip_vertically_on_load(true);

	unsigned char* image = stbi_load("dirt.png", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &firstTx);
	glBindTexture(GL_TEXTURE_2D, firstTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);

	// Second texture. 
	unsigned char* image2 = stbi_load("chainmail.png", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &secondTx);
	glBindTexture(GL_TEXTURE_2D, secondTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image2);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image2);

	

	// Third texture. Blank one.
	unsigned char* image3 = stbi_load("blank.jpg", &width, &height, &bitDepth, 0);
	if (!image3) cout << "Unable to load file!" << endl;
	
	glGenTextures(1, &blankTx);
	glBindTexture(GL_TEXTURE_2D, blankTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image3);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image3);

	// Brick texure
	unsigned char* image4 = stbi_load("brick.jpg", &width, &height, &bitDepth, 0);
	if (!image4) cout << "Unable to load file!" << endl;

	glGenTextures(1, &brickTx);
	glBindTexture(GL_TEXTURE_2D, brickTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image4);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image4);

	//5th image  hedge
	unsigned char* image5 = stbi_load("hedge.png", &width, &height, &bitDepth, 0);
	if (!image5) cout << "Unable to load file!" << endl;

	glGenTextures(1, &hedgeTx);
	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image5);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image5);

	unsigned char* image6 = stbi_load("Treasure room.png", &width, &height, &bitDepth, 0);
	if (!image6) cout << "Unable to load file!" << endl;

	glGenTextures(1, &roomTx);
	glBindTexture(GL_TEXTURE_2D, roomTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image6);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image6);

	glUniform1i(glGetUniformLocation(program, "texture0"), 0);

	// Setting ambient Light.
	glUniform3f(glGetUniformLocation(program, "aLight.ambientColour"), aLight.ambientColour.x, aLight.ambientColour.y, aLight.ambientColour.z);
	glUniform1f(glGetUniformLocation(program, "aLight.ambientStrength"), aLight.ambientStrength);
	// Setting directional light.
	glUniform3f(glGetUniformLocation(program, "dLight.base.diffuseColour"), dLight.diffuseColour.x, dLight.diffuseColour.y, dLight.diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "dLight.base.diffuseStrength"), dLight.diffuseStrength);

	glUniform3f(glGetUniformLocation(program, "dLight.direction"), dLight.direction.x, dLight.direction.y, dLight.direction.z);

	// Setting point lights.
	glUniform3f(glGetUniformLocation(program, "pLights[0].base.diffuseColour"), pLights[0].diffuseColour.x, pLights[0].diffuseColour.y, pLights[0].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[0].base.diffuseStrength"), pLights[0].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[0].position"), pLights[0].position.x, pLights[0].position.y, pLights[0].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[0].constant"), pLights[0].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[0].linear"), pLights[0].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[0].exponent"), pLights[0].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[1].base.diffuseColour"), pLights[1].diffuseColour.x, pLights[1].diffuseColour.y, pLights[1].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[1].base.diffuseStrength"), pLights[1].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[1].position"), pLights[1].position.x, pLights[1].position.y, pLights[1].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[1].constant"), pLights[1].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[1].linear"), pLights[1].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[1].exponent"), pLights[1].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[2].base.diffuseColour"), pLights[2].diffuseColour.x, pLights[2].diffuseColour.y, pLights[2].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[2].base.diffuseStrength"), pLights[2].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[2].position"), pLights[2].position.x, pLights[2].position.y, pLights[2].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[2].constant"), pLights[2].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[2].linear"), pLights[2].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[2].exponent"), pLights[2].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[3].base.diffuseColour"), pLights[3].diffuseColour.x, pLights[3].diffuseColour.y, pLights[3].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[3].base.diffuseStrength"), pLights[3].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[3].position"), pLights[3].position.x, pLights[3].position.y, pLights[3].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[3].constant"), pLights[3].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[3].linear"), pLights[3].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[3].exponent"), pLights[3].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[4].base.diffuseColour"), pLights[4].diffuseColour.x, pLights[4].diffuseColour.y, pLights[4].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[4].base.diffuseStrength"), pLights[4].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[4].position"), pLights[4].position.x, pLights[4].position.y, pLights[4].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[4].constant"), pLights[4].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[4].linear"), pLights[4].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[4].exponent"), pLights[4].exponent);




	vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

		ibo = 0;
		glGenBuffers(1, &ibo);
	
		points_vbo = 0;
		glGenBuffers(1, &points_vbo);

		colors_vbo = 0;
		glGenBuffers(1, &colors_vbo);

		uv_vbo = 0;
		glGenBuffers(1, &uv_vbo);

		normals_vbo = 0;
		glGenBuffers(1, &normals_vbo);

	glBindVertexArray(0); // Can optionally unbind the vertex array to avoid modification.

	// Change shape data.
	g_tower.SetMat(0.1, 16);
	g_grid.SetMat(0.0, 16);

	// Enable depth test and blend.
	glEnable(GL_DEPTH_TEST);
	//glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glBlendEquation(GL_FUNC_ADD);
	// Enable smoothing.
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	// Enable face culling.
	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CCW);
	//glCullFace(GL_BACK);

	timer(0); 
}

//---------------------------------------------------------------------
//
// calculateView
//
void calculateView()
{
	frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec.y = sin(glm::radians(pitch));
	frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec = glm::normalize(frontVec);
	rightVec = glm::normalize(glm::cross(frontVec, worldUp));
	upVec = glm::normalize(glm::cross(rightVec, frontVec));

	View = glm::lookAt(
		position, // Camera position
		position + frontVec, // Look target
		upVec); // Up vector
	glUniform3f(glGetUniformLocation(program, "eyePosition"), position.x, position.y, position.z);
}

//---------------------------------------------------------------------
//
// transformModel
//
void transformObject(glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle, glm::vec3 translation) {
	glm::mat4 Model;
	Model = glm::mat4(1.0f);
	Model = glm::translate(Model, translation);
	Model = glm::rotate(Model, glm::radians(rotationAngle), rotationAxis);
	Model = glm::scale(Model, scale);
	
	calculateView();
	glUniformMatrix4fv(modelID, 1, GL_FALSE, &Model[0][0]);
	glUniformMatrix4fv(viewID, 1, GL_FALSE, &View[0][0]);
	glUniformMatrix4fv(projID, 1, GL_FALSE, &Projection[0][0]);
}

//---------------------------------------------------------------------
//
// display
//
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(vao);
	// Draw all shapes.

	/*glBindTexture(GL_TEXTURE_2D, alexTx);
	g_plane.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 10.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_plane.NumIndices(), GL_UNSIGNED_SHORT, 0);*/

	//glEnable(GL_DEPTH_TEST);


	//ground(grid)
	glBindTexture(GL_TEXTURE_2D, firstTx);
	g_grid.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_grid.NumIndices(), GL_UNSIGNED_SHORT, 0);


	//towers
	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_tower.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(-1.0f, 0.0f, -1.0f));
	glDrawElements(GL_TRIANGLES, g_tower.NumIndices(), GL_UNSIGNED_SHORT, 0);
	
	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_tower.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(24.0f, 0.0f, -26.0f));
	glDrawElements(GL_TRIANGLES, g_tower.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_tower.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(-1.0f, 0.0f, -26.0f));
	glDrawElements(GL_TRIANGLES, g_tower.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_tower.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(24.0f, 0.0f, -1.0f));
	glDrawElements(GL_TRIANGLES, g_tower.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//tower tops
	/*glBindTexture(GL_TEXTURE_2D, brickTx);
	t_top.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), Y_AXIS, 0.0f, glm::vec3(24.0f, 3.0f, -1.0f));
	glDrawElements(GL_TRIANGLES, t_top.NumIndices(), GL_UNSIGNED_SHORT, 0);*/

	//walls
	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 2.0f, 0.5f), X_AXIS, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 2.0f, 0.5f), X_AXIS, 0.0f, glm::vec3(15.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(25.0f, 2.0f, 0.5f), Y_AXIS, 90.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(25.0f, 2.0f, 0.5f), Y_AXIS, 0.0f, glm::vec3(0.0f, 0.0f, -25.0f));
	glDrawElements(GL_TRIANGLES, g_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(25.0f, 2.0f, 0.5f), Y_AXIS, -90.0f, glm::vec3(25.0f, 0.0f, -25.0f));
	glDrawElements(GL_TRIANGLES, g_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);


	//gatehouse
	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 4.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(9.0f, 0.0f, -1.0f));
	glDrawElements(GL_TRIANGLES, g_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	/*glBindTexture(GL_TEXTURE_2D, brickTx);
	g_tower.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 4.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(9.0f, 0.0f, -1.0f));
	glDrawElements(GL_TRIANGLES, g_tower.NumIndices(), GL_UNSIGNED_SHORT, 0);*/

	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 4.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -1.0f));
	glDrawElements(GL_TRIANGLES, g_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	/*glBindTexture(GL_TEXTURE_2D, brickTx);
	g_tower.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 4.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -1.0f));
	glDrawElements(GL_TRIANGLES, g_tower.NumIndices(), GL_UNSIGNED_SHORT, 0);*/

	glBindTexture(GL_TEXTURE_2D, brickTx);
	g_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 0.5f, 2.0f), X_AXIS, 0.0f, glm::vec3(9.0f, 2.5f, -1.0f));
	glDrawElements(GL_TRIANGLES, g_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//mazewall
	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(2.5f, 0.0f, -2.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(14.5f, 0.0f, -2.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(20.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(2.5f, 0.0f, -2.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	g_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(20.0f, 1.0f, 0.3f), Y_AXIS, 0.0f, glm::vec3(2.5f, 0.0f, -22.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);


	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(20.0f, 1.0f, 0.3f), Y_AXIS, -90.0f, glm::vec3(22.5f, 0.0f, -22.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);
	
	//X-axis wall
	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(16.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(4.5f, 0.0f, -4.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(4.5f, 0.0f, -6.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(12.5f, 0.0f, -6.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);


	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(2.5f, 0.0f, -8.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(8.5f, 0.0f, -8.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);


	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(14.5f, 0.0f, -8.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(18.5f, 0.0f, -8.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//*Brick wall
	glBindTexture(GL_TEXTURE_2D, roomTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(10.5f, 0.0f, -10.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(16.5f, 0.0f, -10.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);


	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(2.5f, 0.0f, -12.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(14.5f, 0.0f, -12.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(4.5f, 0.0f, -14.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//*Brick wall
	glBindTexture(GL_TEXTURE_2D, roomTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(12.5f, 0.0f, -14.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(18.5f, 0.0f, -14.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(16.5f, 0.0f, -16.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(12.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(8.5f, 0.0f, -18.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 1.0f, 0.3f), X_AXIS, 0.0f, glm::vec3(6.5f, 0.0f, -20.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//*z_aixs wall

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(4.5f, 0.0f, -10.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(4.5f, 0.0f, -16.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(16.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(6.5f, 0.0f, -4.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(8.5f, 0.0f, -6.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(8.5f, 0.0f, -14.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(10.5f, 0.0f, -4.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);
	//*Brick wall
	glBindTexture(GL_TEXTURE_2D, roomTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(10.5f, 0.0f, -10.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(10.5f, 0.0f, -14.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(12.5f, 0.0f, -6.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(12.5f, 0.0f, -14.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(12.5f, 0.0f, -18.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//*Brick wall
	glBindTexture(GL_TEXTURE_2D, roomTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(14.5f, 0.0f, -10.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);


	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(14.5f, 0.0f, -16.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(14.5f, 0.0f, -20.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(16.5f, 0.0f, -8.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(16.5f, 0.0f, -12.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(16.5f, 0.0f, -18.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(18.5f, 0.0f, -6.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(18.5f, 0.0f, -10.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);


	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(18.5f, 0.0f, -20.5f));
	glDrawElements(GL_TRIANGLES, m_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);


	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(20.5f, 0.0f, -8.5f));



	glBindTexture(GL_TEXTURE_2D, hedgeTx);
	m_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 1.0f, 0.3f), Y_AXIS, 90.0f, glm::vec3(20.5f, 0.0f, -14.5f));
	
	glBindVertexArray(0); // Done writing.
	glutSwapBuffers(); // Now for a potentially smoother render.
}

void parseKeys()
{
	if (keys & KEY_FORWARD)
		position += frontVec * MOVESPEED;
	else if (keys & KEY_BACKWARD)
		position -= frontVec * MOVESPEED;
	if (keys & KEY_LEFT)
		position -= rightVec * MOVESPEED;
	else if (keys & KEY_RIGHT)
		position += rightVec * MOVESPEED;
	if (keys & KEY_UP)
		position.y += MOVESPEED;
	else if (keys & KEY_DOWN)
		position.y -= MOVESPEED;
}

void timer(int) { // essentially our update()
	parseKeys();
	glutPostRedisplay();
	glutTimerFunc(1000/FPS, timer, 0); // 60 FPS or 16.67ms.
}

//---------------------------------------------------------------------
//
// keyDown
//
void keyDown(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD; break;
	case 's':
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD; break;
	case 'a':
		if (!(keys & KEY_LEFT))
			keys |= KEY_LEFT; break;
	case 'd':
		if (!(keys & KEY_RIGHT))
			keys |= KEY_RIGHT; break;
	case 'r':
		if (!(keys & KEY_UP))
			keys |= KEY_UP; break;
	case 'f':
		if (!(keys & KEY_DOWN))
			keys |= KEY_DOWN; break;
	}
}

void keyDownSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD;
	}
}

void keyUp(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		keys &= ~KEY_FORWARD; break;
	case 's':
		keys &= ~KEY_BACKWARD; break;
	case 'a':
		keys &= ~KEY_LEFT; break;
	case 'd':
		keys &= ~KEY_RIGHT; break;
	case 'r':
		keys &= ~KEY_UP; break;
	case 'f':
		keys &= ~KEY_DOWN; break;
	case ' ':
		resetView();
	}
}

void keyUpSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		keys &= ~KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		keys &= ~KEY_BACKWARD;
	}
}

void mouseMove(int x, int y)
{
	if (keys & KEY_MOUSECLICKED)
	{
		pitch += (GLfloat)((y - lastY) * TURNSPEED);
		yaw -= (GLfloat)((x - lastX) * TURNSPEED);
		lastY = y;
		lastX = x;
	}
}

void mouseClick(int btn, int state, int x, int y)
{
	if (state == 0)
	{
		lastX = x;
		lastY = y;
		keys |= KEY_MOUSECLICKED; // Flip flag to true
		glutSetCursor(GLUT_CURSOR_NONE);
		//cout << "Mouse clicked." << endl;
	}
	else
	{
		keys &= ~KEY_MOUSECLICKED; // Reset flag to false
		glutSetCursor(GLUT_CURSOR_INHERIT);
		//cout << "Mouse released." << endl;
	}
}

void clean()
{
	cout << "Cleaning up!" << endl;
	glDeleteTextures(1, &firstTx);
	glDeleteTextures(1, &secondTx);
	glDeleteTextures(1, &blankTx);
	glDeleteTextures(1, &brickTx);
	glDeleteTextures(1, &hedgeTx);
	glDeleteTextures(1, &roomTx);
}

//---------------------------------------------------------------------
//
// main
//
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutSetOption(GLUT_MULTISAMPLE, 8);
	glutInitWindowSize(1024, 1024);
	glutCreateWindow("Final Project");

	glewInit();	//Initializes the glew and prepares the drawing pipeline.
	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyDown);
	glutSpecialFunc(keyDownSpec);
	glutKeyboardUpFunc(keyUp); // New function for third example.
	glutSpecialUpFunc(keyUpSpec);

	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMove); // Requires click to register.
	
	atexit(clean); // This GLUT function calls specified function before terminating program. Useful!

	glutMainLoop();

	return 0;
}
