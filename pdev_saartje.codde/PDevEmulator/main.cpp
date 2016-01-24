#include "Chip8.h"

Chip8 emulator;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	emulator.Keypress(key, action);
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
	std::string path;

	for (int i = 0; i < count; i++)
	{
		path.append(paths[i]);
	}

	emulator.Initialize();
	emulator.LoadFile(*paths);

	if (emulator.file == NULL)
	{
		glfwSetWindowShouldClose(window, 1);
	}
}

// Shader sources
const GLchar* vertexSource =
"#version 150 core\n"
"in vec2 position;"
"in vec2 texcoord;"
"out vec2 Texcoord;"
"void main() {"
"   Texcoord = texcoord;"
"   gl_Position = vec4(position, 0.0, 1.0);"
"}";

const GLchar* fragmentSource =
"#version 150 core\n"
"in vec2 Texcoord;"
"out vec4 outColor;"
"uniform sampler2D texGraphics;"
"void main() {"
"   outColor=texture(texGraphics, Texcoord);"
"}";

int main()
{
	if (!emulator.Initialize()) return 0;
	
	if (!glfwInit())
		exit(EXIT_FAILURE);

	GLFWwindow* window = glfwCreateWindow(640, 320, "Chip-8 Emulator", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	emulator.LoadFile();

	glfwMakeContextCurrent(window);

	// Load all OpenGL functions using the glfw loader function
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	// Create Vertex Array Object
	GLuint vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	// Create a Vertex Buffer Object and copy the vertex data to it
	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);

	//xyuv
	GLfloat vertices[] =
	{
		//	X		Y		U		V
			-1.0f,	1.0f,	0.0f,	0.0f,	// Top-left
			1.0f,	1.0f,	1.0,	0.0f,	// Top-right
			1.0f,	-1.0f,	1.0f,	1.0f,	// Bottom-right
			-1.0f,	-1.0f,	0.0f,	1.0f	// Bottom-left
	};

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Create an element array
	GLuint elementArray;
	glGenBuffers(1, &elementArray);

	GLuint indices[] = {
		0, 1, 2,
		2, 3, 0
	};

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArray);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Create and compile the vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	// Create and compile the fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	glGetError();

	// Link the vertex and fragment shader into a shader program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// Specify the layout of the vertex data
	GLint positionAttribute = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(positionAttribute);
	glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	GLint textureAttribute = glGetAttribLocation(shaderProgram, "texcoord");
	glEnableVertexAttribArray(textureAttribute);
	glVertexAttribPointer(textureAttribute, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<void*>(2 * sizeof(GLfloat)));

	// Load texture
	GLuint texture;
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glfwSwapInterval(1);
	glfwSetKeyCallback(window, key_callback);
	glfwSetDropCallback(window, drop_callback);

	glBindTexture(GL_TEXTURE_2D, texture);

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);

		for (size_t i = 0; i < 8; i++) // 1 tick is 60 hz, default == 0.5khz, 500/60 = 8,xx
		{
			emulator.Tick();
		}

		if (emulator.delayTimer > 0)
		{
			emulator.delayTimer--;
		}

		if (emulator.soundTimer > 0)
		{
			if (emulator.soundTimer > 1)
			{
				Beep(523, 50 * emulator.soundTimer);
				emulator.soundTimer = 0;
			}
			else
			{
				Beep(523, 50);
				--emulator.soundTimer;
			}
		}

		emulator.Draw();

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, &emulator.textureVector[0]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}