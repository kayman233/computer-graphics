// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <iostream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/texture.hpp>

# define M_PI 3.14159265358979323846  /* pi */

vec4 random_quaternion()
{
	// The rand() function is not very portable and may not be available on your system.
	// Add the appropriate include or replace by an other random function in case of problem.
	double seed = rand() / (double)RAND_MAX;
	double r1 = std::sqrt(1.0 - seed);
	double r2 = std::sqrt(seed);
	double t1 = 2.0 * M_PI * (rand() / (double)RAND_MAX);
	double t2 = 2.0 * M_PI * (rand() / (double)RAND_MAX);
	return vec4(std::sin(t1) * r1, std::cos(t1) * r1, std::sin(t2) * r2, std::cos(t2) * r2);
}

struct Object {
	vec3 pos;
	float size;
	vec4 quat;
	float cameradistance;
	bool is_alive;

	bool operator<(const Object& that) const {
		return this->cameradistance > that.cameradistance;
	}

	Object(vec3 _pos, vec4 _quat) : pos(_pos), quat(_quat) {
		size = 2.0f;
		is_alive = true;
		vec3 cam_pos = getCameraPosition();
		cameradistance = distance(pos, cam_pos);
	}
};

const int MaxObjects = 100;
const int MaxDistance = 30;
const int MinDistance = -30;
std::vector<Object> ObjectsContainer;

void InstantiateObject() {
	float x_p = rand() % (MaxDistance - MinDistance + 1) + MinDistance;
	float y_p = rand() % (MaxDistance - MinDistance + 1) + MinDistance;
	float z_p = rand() % (MaxDistance - MinDistance + 1) + MinDistance;

	vec3 pos(x_p, 0, z_p);
	pos += getCameraPosition();
	vec4 quat = random_quaternion();
	ObjectsContainer.emplace_back(Object(pos, quat));
}


void SortObjects() {
	std::sort(ObjectsContainer.begin(), ObjectsContainer.end());
}

struct Fireball {
	vec3 pos;
	vec3 dir;
	float speed;
	float size;
	bool is_alive;

	Fireball(vec3 _pos, vec3 _dir) : pos(_pos), dir(_dir) {
		is_alive = true;
		speed = 10.0f;
		size = 1.0f;
	}
};

const int MaxFireballs = 100;
std::vector<Fireball> FireballsContainer;

void InstantiateFireball() {
	vec3 dir = normalize(getCameraDirection());
	vec3 pos = getCameraPosition() + dir;
	FireballsContainer.emplace_back(Fireball(pos, dir));
}

void RemoveFarFireballs() {
	for (int i = FireballsContainer.size() - 1; i >= 0; --i) {
		vec3 camera_pos = getCameraPosition();
		vec3 fireball_pos = FireballsContainer[i].pos;
		float dist = distance(fireball_pos, camera_pos);
		if (dist >= MaxDistance + 10) {
			FireballsContainer.erase(FireballsContainer.begin() + i);
		}
	}
}

void CheckCollision() {
	for (Fireball& fireball : FireballsContainer) {
		for (Object& object : ObjectsContainer) {
			float dist = distance(object.pos, fireball.pos);
			if (dist <= object.size + fireball.size) {
				fireball.is_alive = false;
				object.is_alive = false;
			}
		}
	}

	for (int i = FireballsContainer.size() - 1; i >= 0; --i) {
		if (!FireballsContainer[i].is_alive) {
			FireballsContainer.erase(FireballsContainer.begin() + i);
		}
	}

	for (int i = ObjectsContainer.size() - 1; i >= 0; --i) {
		if (!ObjectsContainer[i].is_alive) {
			ObjectsContainer.erase(ObjectsContainer.begin() + i);
		}
	}
}

int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Tutorial 04 - Colored Cube", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programObject = LoadShaders("Object.vertexshader", "Object.fragmentshader");
	GLuint programFire = LoadShaders("Fireball.vertexshader", "Fireball.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint MatrixObject = glGetUniformLocation(programObject, "MVP");
	GLuint MatrixFire = glGetUniformLocation(programFire, "MVP");

	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID = glGetUniformLocation(programFire, "myTextureSampler");

	GLuint Texture = loadDDS("fire.DDS");

	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals; // Won't be used at the moment.
	bool res = loadOBJ("sphere.obj", vertices, uvs, normals);

	// Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
	// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
	static const GLfloat g_vertex_buffer_data[] = {
		-1.0f,-1.0f,-1.0f,
		-1.5f,-1.0f, 1.5f,
		-1.0f, 1.0f, 1.0f,

		 1.0f,-1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,

		 1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		 1.5f,-1.0f,-1.5f,

		 1.0f, 1.0f,-1.0f,
		 1.5f,-1.0f,-1.5f,
		-1.0f,-1.0f,-1.0f,

		 1.0f,-1.0f, 1.0f,
		-1.5f,-1.0f, 1.5f,
		-1.0f,-1.0f,-1.0f,

		-1.0f, 1.0f, 1.0f,
		-1.5f,-1.0f, 1.5f,
		 1.0f,-1.0f, 1.0f,

		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,

		 1.5f,-1.0f,-1.5f,
		 1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f, 1.0f,
	};

	static GLfloat g_color_buffer_data[8 * 3 * 3];
	for (int v = 0; v < 8 * 3; v++) {
		g_color_buffer_data[3 * v + 0] = 0.7f;
		g_color_buffer_data[3 * v + 1] = (double)rand() / (RAND_MAX);
		g_color_buffer_data[3 * v + 2] = 1.0f;
	}

	static std::vector<vec3> g_obj_position_data(MaxObjects);
	static std::vector<vec4> g_obj_quat_data(MaxObjects);

	GLuint object_vertexbuffer;
	glGenBuffers(1, &object_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, object_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	GLuint objects_position_buffer;
	glGenBuffers(1, &objects_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, objects_position_buffer);
	glBufferData(GL_ARRAY_BUFFER, MaxObjects * sizeof(vec3), nullptr, GL_STREAM_DRAW);

	GLuint object_colorbuffer;
	glGenBuffers(1, &object_colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, object_colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

	GLuint object_quat_buffer;
	glGenBuffers(1, &object_quat_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, object_quat_buffer);
	glBufferData(GL_ARRAY_BUFFER, MaxObjects * sizeof(vec4), nullptr, GL_STREAM_DRAW);




	static std::vector<vec3> g_fireball_position_data(MaxFireballs);

	GLuint fireball_vertex_buffer;
	glGenBuffers(1, &fireball_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, fireball_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), &vertices[0], GL_STREAM_DRAW);

	GLuint fireball_position_buffer;
	glGenBuffers(1, &fireball_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, fireball_position_buffer);
	glBufferData(GL_ARRAY_BUFFER, MaxFireballs * sizeof(vec3), nullptr, GL_STREAM_DRAW);

	GLuint fireball_uvbuffer;
	glGenBuffers(1, &fireball_uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, fireball_uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);

	double lastTime = glfwGetTime();
	double createTime = 2.0f;
	double shootTime = 0.0f;
	do {
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RemoveFarFireballs();
		CheckCollision();

		double currentTime = glfwGetTime();
		double delta = currentTime - lastTime;
		lastTime = currentTime;
		createTime += delta;
		shootTime += delta;

		if (createTime >= 3.0f && ObjectsContainer.size() <= MaxObjects) {
			InstantiateObject();
			SortObjects();
			createTime = 0.0f;
		}

		if (shootTime >= 2.0f && FireballsContainer.size() <= MaxFireballs) {
			std::cout << "shoot\n";
			InstantiateFireball();
			shootTime = 0.0f;
		}

		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		for (int i = 0; i < ObjectsContainer.size(); ++i) {
			Object& object = ObjectsContainer[i];
			g_obj_position_data[i] = object.pos;
			g_obj_quat_data[i] = object.quat;
		}

		glBindBuffer(GL_ARRAY_BUFFER, objects_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, MaxObjects * sizeof(vec3), nullptr, GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, ObjectsContainer.size() * sizeof(vec3), &g_obj_position_data[0]);

		glBindBuffer(GL_ARRAY_BUFFER, object_quat_buffer);
		glBufferData(GL_ARRAY_BUFFER, MaxObjects * sizeof(vec4), nullptr, GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, ObjectsContainer.size() * sizeof(vec4), &g_obj_quat_data[0]);

		glUseProgram(programObject);

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixObject, 1, GL_FALSE, &MVP[0][0]);

		// 1 attribute buffer : object_vertexbuffer
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, object_vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2 attribute buffer : objects_position_buffer
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, objects_position_buffer);
		glVertexAttribPointer(
			1,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 3 attribute buffer : object_colorbuffer
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, object_colorbuffer);
		glVertexAttribPointer(
			2,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 4 attribute buffer : object_quat_buffer
		glEnableVertexAttribArray(3);
		glBindBuffer(GL_ARRAY_BUFFER, object_quat_buffer);
		glVertexAttribPointer(
			3,                  // attribute
			4,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		glVertexAttribDivisor(0, 0);
		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 0);
		glVertexAttribDivisor(3, 1);

		glDrawArraysInstanced(GL_TRIANGLES, 0, 8 * 3, ObjectsContainer.size());

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);

		for (int i = 0; i < FireballsContainer.size(); ++i) {
			Fireball& fireball = FireballsContainer[i];
			fireball.pos += fireball.dir * fireball.speed * (float)delta;
			g_fireball_position_data[i] = fireball.pos;
		}

		glBindBuffer(GL_ARRAY_BUFFER, fireball_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, MaxFireballs * sizeof(vec3), nullptr, GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, FireballsContainer.size() * sizeof(vec3), &g_fireball_position_data[0]);


		glUseProgram(programFire);
		glUniformMatrix4fv(MatrixFire, 1, GL_FALSE, &MVP[0][0]);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		// 1 attribute buffer : fireball_vertex_buffer
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, fireball_vertex_buffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2 attribute buffer : fireball_uvbuffer
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, fireball_uvbuffer);
		glVertexAttribPointer(
			1,                  // attribute
			2,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 3 attribute buffer : fireball_position_buffer
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, fireball_position_buffer);
		glVertexAttribPointer(
			2,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		glVertexAttribDivisor(0, 0);
		glVertexAttribDivisor(1, 0);
		glVertexAttribDivisor(2, 1);

		glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size(), FireballsContainer.size());

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteBuffers(1, &object_vertexbuffer);
	glDeleteBuffers(1, &object_quat_buffer);
	glDeleteBuffers(1, &objects_position_buffer);
	glDeleteBuffers(1, &object_colorbuffer);
	glDeleteBuffers(1, &fireball_vertex_buffer);
	glDeleteBuffers(1, &fireball_uvbuffer);
	glDeleteBuffers(1, &fireball_position_buffer);
	glDeleteProgram(programObject);
	glDeleteProgram(programFire);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

