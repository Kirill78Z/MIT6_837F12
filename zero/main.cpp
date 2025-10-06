#include "GL/freeglut.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include "vecmath.h"
#include "main.h"
using namespace std;

// Globals

// This is the list of points (3D vectors)
vector<Vector3f> vecv;

// This is the list of normals (also 3D vectors)
vector<Vector3f> vecn;

// This is the list of faces (indices into vecv and vecn)
vector<vector<unsigned>> vecf;

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

	// This GLUT method draws a teapot.  You should replace
	// it with code which draws the object you loaded.
	//glutSolidTeapot(1.0);
	for (vector<unsigned> face : vecf) {
		unsigned a = face[0];
		unsigned c = face[1];
		unsigned d = face[2];
		unsigned f = face[3];
		unsigned g = face[4];
		unsigned i = face[5];


		glBegin(GL_TRIANGLES);
		glNormal3d(vecn[c - 1][0], vecn[c - 1][1], vecn[c - 1][2]);
		glVertex3d(vecv[a - 1][0], vecv[a - 1][1], vecv[a - 1][2]);
		glNormal3d(vecn[f - 1][0], vecn[f - 1][1], vecn[f - 1][2]);
		glVertex3d(vecv[d - 1][0], vecv[d - 1][1], vecv[d - 1][2]);
		glNormal3d(vecn[i - 1][0], vecn[i - 1][1], vecn[i - 1][2]);
		glVertex3d(vecv[g - 1][0], vecv[g - 1][1], vecv[g - 1][2]);
		glEnd();
	}


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
			vecv.push_back(v);
		}

		if (s == "vn") {
			Vector3f vn;
			ss >> vn[0] >> vn[1] >> vn[2];
			vecn.push_back(vn);
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

			vecf.push_back(face);
		}
	}
}

void time_rotate(int value) {
	if (value == 1) {
		if (IS_ROTATE) {
			glMatrixMode(GL_MODELVIEW);
			glRotatef(5, 0, 1, 0);
			glutPostRedisplay();
		}
		glutTimerFunc(100, time_rotate, 1);
	}
}

void initCamera()
{
	// Rotate the image
	glMatrixMode(GL_MODELVIEW);  // Current matrix affects objects positions
	glLoadIdentity();              // Initialize to the identity

	// Position the camera at [0,0,5], looking at [0,0,0],
	// with [0,1,0] as the up direction.
	gluLookAt(0.0, 0.0, 5.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0);
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

	// Initialize OpenGL parameters.
	initRendering();

	// Set up callback functions for key presses
	glutKeyboardFunc(keyboardFunc); // Handles "normal" ascii symbols
	glutSpecialFunc(specialFunc);   // Handles "special" keyboard keys

	initCamera();

	glutTimerFunc(100, time_rotate, 1);

	// Set up the callback function for resizing windows
	glutReshapeFunc(reshapeFunc);

	// Call this whenever window needs redrawing
	glutDisplayFunc(drawScene);

	// Start the main loop.  glutMainLoop never returns.
	glutMainLoop();

	return 0;	// This line is never reached.
}
