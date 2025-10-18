#define _USE_MATH_DEFINES
#include <GLEW/glew.h>
#include "GL/freeglut.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include "vecmath.h"
#include "main.h"

#include <map>

using namespace std;

void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::cerr << "OpenGL Debug Message: " << message << std::endl;
}

struct Vertex {
	GLfloat pos[3];
	GLfloat norm[3];
};

// Globals

// input data
vector<Vector3f> inputPositions;
vector<Vector3f> inputNormals;
vector<vector<unsigned>> inputFaces;

// converted input data to upload to GPU
vector<Vertex> uniqueVertices;
vector<unsigned int> indices;

// GPU data
GLuint vertexBufferObjectVerticesId;
GLuint elementBufferObject; //indices

//navigation
int mouseX = 0;
int mouseY = 0;
bool rotateByMouseMove = false;

const float DegToRad = M_PI / 180.0f;

Vector3f cameraDir(0, 0, -1);
Vector3f cameraUp(0, 1, 0);
Vector3f cameraPos(0, 0, 5);

//other
GLfloat diffColors[4][4] = { {0.5, 0.5, 0.9, 1.0}, {0.9, 0.5, 0.5, 1.0}, {0.5, 0.9, 0.3, 1.0}, {0.3, 0.8, 0.9, 1.0} };

// You will need more global variables to implement color and position changes
int colorIndex = 0;

// Light position
GLfloat Lt0pos[] = { 1.0f, 1.0f, 5.0f, 1.0f };

std::streamsize const MAX_BUFFER_SIZE = 256;

bool IS_ROTATE = false;


// These are convenience functions which allow us to call OpenGL 
// methods on Vec3d objects
inline void glVertex(const Vector3f& a)
{
	glVertex3fv(a);
}

inline void glNormal(const Vector3f& a)
{
	glNormal3fv(a);
}


// This function is called whenever a "Normal" key press is received.
void keyboardFunc(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27: // Escape key
		exit(0);
		break;
	case 'c':
		colorIndex = (colorIndex + 1) % 4;
		break;
	case 'r':
		IS_ROTATE = !IS_ROTATE;
		break;
	default:
		cout << "Unhandled key press " << key << "." << endl;
	}

	// this will refresh the screen so that the user sees the color change
	glutPostRedisplay();
}

// This function is called whenever a "Special" key press is received.
// Right now, it's handling the arrow keys.
void specialFunc(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		Lt0pos[1] += 0.5;
		break;
	case GLUT_KEY_DOWN:
		Lt0pos[1] -= 0.5;
		break;
	case GLUT_KEY_LEFT:
		Lt0pos[0] -= 0.5;
		break;
	case GLUT_KEY_RIGHT:
		Lt0pos[0] += 0.5;
		break;
	}

	// this will refresh the screen so that the user sees the light position
	glutPostRedisplay();
}

void SetUpCamera()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	Vector3f lookAtCenter = cameraPos + cameraDir;
	gluLookAt(cameraPos.x(), cameraPos.y(), cameraPos.z(),
		lookAtCenter.x(), lookAtCenter.y(), lookAtCenter.z(),
		cameraUp.x(), cameraUp.y(), cameraUp.z());
	glutPostRedisplay();
}

void mouseBtnCallback(int button, int state, int x, int y)
{
	if (button == GLUT_MIDDLE_BUTTON)
	{
		rotateByMouseMove = state == GLUT_DOWN;
		mouseX = x;
		mouseY = y;
	}
}

void mouseMoveCallback(int x, int y)
{
	int displacementX = x - mouseX;
	int displacementY = y - mouseY;
	mouseX = x;
	mouseY = y;
	if (rotateByMouseMove)
	{
		Vector3f verticalRotateAxis = Vector3f::cross(cameraDir, cameraUp).normalized();
		Matrix3f rotationMatrix =
			Matrix3f::rotation(Vector3f(0, 1, 0), -displacementX * 0.002f)
			* Matrix3f::rotation(verticalRotateAxis, -displacementY * 0.002f);
		cameraDir = rotationMatrix * cameraDir;
		cameraUp = rotationMatrix * cameraUp;
		cameraPos = rotationMatrix * cameraPos;

		SetUpCamera();
	}
}

// This function is responsible for displaying the object.
void drawScene(void)
{
	// Clear the rendering window
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set material properties of object

	// Here we use the first color entry as the diffuse color
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diffColors[colorIndex]);

	// Define specular color and shininess
	GLfloat specColor[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat shininess[] = { 100.0 };

	// Note that the specular color and shininess can stay constant
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

	// Set light properties

	// Light color (RGBA)
	GLfloat Lt0diff[] = { 1.0,1.0,1.0,1.0 };


	glLightfv(GL_LIGHT0, GL_DIFFUSE, Lt0diff);
	glLightfv(GL_LIGHT0, GL_POSITION, Lt0pos);


	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, pos));
	glNormalPointer(GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, norm));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferObject);

	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	// Dump the image to the screen.
	glutSwapBuffers();
}

// Initialize OpenGL's rendering modes
void initRendering()
{
	glEnable(GL_DEPTH_TEST);   // Depth testing must be turned on
	glEnable(GL_LIGHTING);     // Enable lighting calculations
	glEnable(GL_LIGHT0);       // Turn on light #0.
}

// Called when the window is resized
// w, h - width and height of the window in pixels.
void reshapeFunc(int w, int h)
{
	// Always use the largest square viewport possible
	if (w > h) {
		glViewport((w - h) / 2, 0, h, h);
	}
	else {
		glViewport(0, (h - w) / 2, w, w);
	}

	// Set up a perspective view, with square aspect ratio
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// 50 degree fov, uniform aspect ratio, near = 1, far = 100
	gluPerspective(50.0, 1.0, 1.0, 100.0);
}

void loadInput()
{
	char buffer[MAX_BUFFER_SIZE];

	while (true)
	{
		cin.getline(buffer, MAX_BUFFER_SIZE);
		stringstream ss(buffer);

		string s;
		ss >> s;
		if (s == "\0") {
			cin.getline(buffer, MAX_BUFFER_SIZE);
			stringstream ss(buffer);
			ss >> s;
			if (s == "\0")
				break;
		}

		if (s == "v") {
			Vector3f v;
			ss >> v[0] >> v[1] >> v[2];
			inputPositions.push_back(v);
		}

		if (s == "vn") {
			Vector3f vn;
			ss >> vn[0] >> vn[1] >> vn[2];
			inputNormals.push_back(vn);
		}

		if (s == "f") {
			vector<unsigned> face;

			string currIndexStr;
			std::getline(ss, currIndexStr, '/');
			face.push_back(std::stoul(currIndexStr, nullptr, 0));

			std::getline(ss, currIndexStr, '/');

			ss >> currIndexStr;
			face.push_back(std::stoul(currIndexStr, nullptr, 0));


			std::getline(ss, currIndexStr, '/');
			face.push_back(std::stoul(currIndexStr, nullptr, 0));

			std::getline(ss, currIndexStr, '/');

			ss >> currIndexStr;
			face.push_back(std::stoul(currIndexStr, nullptr, 0));


			std::getline(ss, currIndexStr, '/');
			face.push_back(std::stoul(currIndexStr, nullptr, 0));

			std::getline(ss, currIndexStr, '/');

			ss >> currIndexStr;
			face.push_back(std::stoul(currIndexStr, nullptr, 0));

			inputFaces.push_back(face);
		}
	}

	//In Wavefront .OBJ file we have two separate index streams: one for vertices and one for normals.
	//in order to use vertex buffers in our viewer we need to create a new flattened vertex buffer,
	//where each unique combination of (position, normal) gets its own vertex entry.
	map<std::pair<unsigned, unsigned>, unsigned> vertexMap;

	for (auto& face : inputFaces) {
		for (int j = 0; j < 3; ++j) {
			unsigned vIndex = face[j * 2 + 0]; // vertex index
			unsigned nIndex = face[j * 2 + 1]; // normal index
			auto key = std::make_pair(vIndex, nIndex);

			auto it = vertexMap.find(key);
			if (it != vertexMap.end()) {
				indices.push_back(it->second);
			}
			else {
				Vertex vert;
				vert.pos[0] = inputPositions[vIndex - 1][0];
				vert.pos[1] = inputPositions[vIndex - 1][1];
				vert.pos[2] = inputPositions[vIndex - 1][2];
				vert.norm[0] = inputNormals[nIndex - 1][0];
				vert.norm[1] = inputNormals[nIndex - 1][1];
				vert.norm[2] = inputNormals[nIndex - 1][2];

				unsigned newIndex = uniqueVertices.size();
				uniqueVertices.push_back(vert);
				vertexMap[key] = newIndex;
				indices.push_back(newIndex);
			}
		}
	}
}

void uploadInputToGpu()
{
	//you can use VBOs in your program, without writing shaders, as long as your OpenGL context supports the compatibility profile
	//The Compatibility Profile keeps all of the legacy (fixed-function) OpenGL functionality plus the modern programmable features.

	glGenBuffers(1, &vertexBufferObjectVerticesId);
	glGenBuffers(1, &elementBufferObject);

	// Upload vertices
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectVerticesId);
	glBufferData(GL_ARRAY_BUFFER,
		uniqueVertices.size() * sizeof(Vertex),
		uniqueVertices.data(),
		GL_STATIC_DRAW);

	// Upload indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		indices.size() * sizeof(GLuint),
		indices.data(),
		GL_STATIC_DRAW);
}

void time_rotate(int value) {
	const float timeRotateDeg = 0.5f;
	if (value == 1)
	{
		if (IS_ROTATE) {
			Matrix3f rotationMatrix = Matrix3f::rotation(Vector3f(0, 1, 0), timeRotateDeg * DegToRad);
			cameraDir = rotationMatrix * cameraDir;
			cameraUp = rotationMatrix * cameraUp;
			cameraPos = rotationMatrix * cameraPos;

			SetUpCamera();
		}
		glutTimerFunc(100, time_rotate, 1);
	}
}
// Main routine.
// Set up OpenGL, define the callbacks and start the main loop
int main(int argc, char** argv)
{
	loadInput();

	glutInit(&argc, argv);

	// We're going to animate it, so double buffer 
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	// Initial parameters for window position and size
	glutInitWindowPosition(60, 60);
	glutInitWindowSize(360, 360);
	glutCreateWindow("Assignment 0");

	//init glew
	GLenum initResult = glewInit();
	if (initResult != GLEW_OK)
		return -1;

	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

	// Enable debug output
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Ensure callbacks are synchronous for easier debugging
	glDebugMessageCallback(DebugCallback, nullptr);

	uploadInputToGpu();

	// Initialize OpenGL parameters.
	initRendering();

	// Set up callback functions for key presses
	glutKeyboardFunc(keyboardFunc); // Handles "normal" ascii symbols
	glutSpecialFunc(specialFunc);   // Handles "special" keyboard keys
	glutMouseFunc(mouseBtnCallback);
	glutMotionFunc(mouseMoveCallback);

	SetUpCamera();

	glutTimerFunc(1, time_rotate, 1);

	// Set up the callback function for resizing windows
	glutReshapeFunc(reshapeFunc);

	// Call this whenever window needs redrawing
	glutDisplayFunc(drawScene);

	// Start the main loop.  glutMainLoop never returns.
	glutMainLoop();

	return 0;	// This line is never reached.
}
