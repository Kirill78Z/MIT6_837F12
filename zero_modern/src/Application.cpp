//Here I tried to rework the zero assignment in the modern OpenGL way.
//Doing this I realized that reproducing the zero assignment in pure modern OpenGL is too difficult for me now because it requires complex shaders.
//So I dropped it. It just renders one quad with some constant color and that's it.
//I did some further research with great help of ChatGPT and figured out that I don't have to write shaders on my own to use VBOs.
//I can use VBOs in the zero assignment, without writing shaders, as long as OpenGL context supports the compatibility profile. That's enough for me.
#include <assert.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::cerr << "OpenGL Debug Message: " << message << std::endl;
	__debugbreak();
}

struct ShaderProgramSource {
	std::string VertexSource;
	std::string FragmentSource;
};

static ShaderProgramSource ParseShader(const std::string& filePath) {
	std::ifstream stream(filePath);

	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1,
	};

	std::string line;
	std::stringstream ss[2];
	ShaderType type = ShaderType::NONE;
	while (getline(stream, line)) {
		if (line.find("#shader") != std::string::npos) {
			if (line.find("vertex") != std::string::npos) {
				type = ShaderType::VERTEX;
			}
			else if (line.find("fragment") != std::string::npos) {
				type = ShaderType::FRAGMENT;
			}
		}
		else {
			ss[(int)type] << line << "\n";
		}
	}

	return { ss[0].str(), ss[1].str() };
}

static unsigned int CompileShader(unsigned int type, const std::string& source) {
	unsigned int id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE) {
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)alloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		std::cout << "FAILED TO COMPILE " << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") << " SHADER" << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id);
		return 0;
	}

	return id;
}

static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader) {
	unsigned int program = glCreateProgram();
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "Zero assignment modern", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//init glew
	GLenum initResult = glewInit();
	if (initResult != GLEW_OK)
		return -1;

	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

	// Enable debug output
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Ensure callbacks are synchronous for easier debugging
	glDebugMessageCallback(DebugCallback, nullptr);

	const int vertexNumber = 4;
	const int coordinateNumber = vertexNumber * 2;
	float positions[coordinateNumber] = {
		-0.5f, -0.5f,
		0.5f, -0.5f,
		0.5f, 0.5f,
		-0.5f, 0.5f,
	};

	const int triangleNumber = 2;
	const int indicesNumber = triangleNumber * 3;
	unsigned int indices[triangleNumber * 3] = {
		0, 1, 2,
		2, 3, 0
	};

	//create a buffer for vertices
	unsigned int vertexBufferId;
	glGenBuffers(1, &vertexBufferId);

	//select buffer for vertices
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);

	//put data into buffer: vertices
	glBufferData(GL_ARRAY_BUFFER, coordinateNumber * sizeof(float), positions, GL_STATIC_DRAW);//we can put null as data and populate it later

	//enable the position attribute of vertex to tell ogl to use it in rendering
	glEnableVertexAttribArray(0);

	//describe the position attribute of vertex
	glVertexAttribPointer(
		0, //number of attributes of vertex (position, normal, texture coordinate...) that we describe. We have only position.
		2, //number of data elements in attribute: number of coordinates inside position
		GL_FLOAT, //type of data elements
		GL_FALSE, //no need to normalize the passed data
		sizeof(float) * 2, //stride: how many bytes between vertices
		0 //offset to the next attribute in vertex. Since we have only one attribute per vertex we can just pass zero
	);

	//create a buffer for indices
	unsigned int indexBufferId;
	glGenBuffers(1, &indexBufferId);

	//select buffer for indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);

	//put data into buffer: indices
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesNumber * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	//compile shaders
	ShaderProgramSource shaders = ParseShader("res/shaders/Basic.shader");
	unsigned int shader = CreateShader(shaders.VertexSource, shaders.FragmentSource);
	glUseProgram(shader);

	//set color as uniform
	int location = glGetUniformLocation(shader, "u_Color");
	assert(location != -1);
	glUniform4f(location, 0.2f, 0.3f, 1.0f, 1.0f);//must be used when the corresponding shader is selected

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);

		//glDrawArrays(GL_TRIANGLES, 0, 3);
		glDrawElements(GL_TRIANGLES, indicesNumber, GL_UNSIGNED_INT, nullptr);//nullptr because GL_ELEMENT_ARRAY_BUFFER is bound now (after glBindBuffer call)

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glDeleteProgram(shader);

	glfwTerminate();
	return 0;
}