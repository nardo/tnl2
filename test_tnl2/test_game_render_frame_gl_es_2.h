///
// Create a shader object, load the shader source, and
// compile the shader.
//
static GLuint load_shader(const char *shaderSrc, GLenum type)
{
	GLuint shader;
	GLint compiled;
	// Create the shader object
    shader = glCreateShader(type);
	if(shader == 0)
		return 0;
	// Load the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);
    // Compile the shader
	glCompileShader(shader);
    // Check the compile status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
	{
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if(infoLen > 1)
		{
			char* infoLog = (char *) malloc(sizeof(char) * infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			logprintf("Error compiling shader:\n%s\n%s\n", shaderSrc, infoLog);
			free(infoLog);
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}
///
// Initialize the shader and program object
//

struct attribute
{
	int location;
	const char *attribute_name;
	
	attribute(int in_location, const char *in_attribute)
	{
		location = in_location;
		attribute_name = in_attribute;
	}
};

static GLuint init_program_object(const char *vShaderStr, const char *fShaderStr, array<attribute> &attributes)
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;
	// Load the vertex/fragment shaders
	vertexShader = load_shader(vShaderStr, GL_VERTEX_SHADER );
	fragmentShader = load_shader(fShaderStr, GL_FRAGMENT_SHADER);
	// Create the program object
	programObject = glCreateProgram();
	if(programObject == 0)
		return 0;
	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);
	// Bind vPosition to attribute 0
	for(uint32 i = 0; i < attributes.size(); i++)
		glBindAttribLocation(programObject, attributes[i].location, attributes[i].attribute_name);
	// Link the program
	glLinkProgram(programObject);
	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
	if(!linked)
	{
		GLint infoLen = 0;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
		if(infoLen > 1)
		{
			char* infoLog = (char *) malloc(sizeof(char) * infoLen);
			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			logprintf("Error linking program:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteProgram(programObject);
		return 0;
	}
	return programObject;
}

static void box(GLfloat render_vertices[12], float left, float top, float right, float bottom)
{
	/* [0, 1, 2],
	   [3, 4, 5],
	   [6, 7, 8],
	   [9, 10, 11], */
	
	render_vertices[0] = left * 2 - 1; render_vertices[1] = 1 - top * 2; render_vertices[2] = 0;
	render_vertices[3] = left * 2 - 1; render_vertices[4] = 1 - bottom * 2; render_vertices[5] = 0;
	render_vertices[6] = right * 2 - 1; render_vertices[7] = 1 - top * 2; render_vertices[8] = 0;
	render_vertices[9] = right * 2 - 1; render_vertices[10] = 1 - bottom * 2; render_vertices[11] = 0;
}

/// test_game_render_frame_open_gl is called by the platform windowing code to notify the game that it should render the current world using OpenGL.
static void test_game_render_frame_open_gl(test_game *the_game)
{	
	static GLuint program_object = 0;
	static GLuint color_loc = 0;
	if(!program_object)
	{
		char vShaderStr[] =
		"attribute vec4 vPosition;   \n"
		"void main()                 \n"
		"{                           \n"
		"   gl_Position = vPosition; \n"
		"}                           \n";
		char fShaderStr[] =
		"uniform vec4 color;  \n"
		//"precision mediump float;                   \n"
		"void main()                                \n"
		"{                                          \n"
		" gl_FragColor = color; \n"
		"}                                          \n";
		
		array<attribute> attributes;
		attributes.push_back(attribute(0, "vPosition"));
		program_object = init_program_object(vShaderStr, fShaderStr, attributes);
		if(!program_object)
			return;
		color_loc = glGetUniformLocation(program_object, "color");
	}
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(program_object);
	// Load the vertex data
	glClearColor(1, 1, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   	
	/*if(the_game->_client_player)
	{
		position p = the_game->_client_player->_render_pos;
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBegin(GL_POLYGON);
		glColor4f(0.5f, 0.5f, 0.5f, 0.65f);
		for(float32 r = 0; r < 3.1415 * 2; r += 0.1f)
		{
			glVertex2f(p.x + 0.25f * cos(r), p.y + 0.25f * sin(r));
		}
		
		glEnd();
		glDisable(GL_BLEND);
	}*/
	
	GLfloat render_vertices[12];
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, render_vertices);
	glEnableVertexAttribArray(0);
	
	// then draw all the _buildings.
	glUniform4f(color_loc, 1, 0, 0, 1);
	for(int32 i = 0; i < the_game->_buildings.size(); i++)
	{
		building *b = the_game->_buildings[i];
		
		box(render_vertices, b->upper_left.x, b->upper_left.y, b->lower_right.x, b->lower_right.y);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	
	// last, draw all the _players in the game.
	for(int32 i = 0; i < the_game->_players.size(); i++)
	{
		player *p = the_game->_players[i];
		glUniform4f(color_loc, 0, 0, 0, 1);
		
		box(render_vertices, p->_render_pos.x - 0.012f, p->_render_pos.y - 0.012f, p->_render_pos.x + 0.012f, p->_render_pos.y + 0.012f);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		switch(p->_player_type)
		{
			case player::player_type_ai:
			case player::player_type_ai_dummy:
				glUniform4f(color_loc, 0, 0, 1, 1);
				break;
			case player::player_type_client:
				glUniform4f(color_loc, 0.5, 0.5, 1, 1);
				break;
			case player::player_type_my_client:
				glUniform4f(color_loc, 1, 1, 1, 1);
				break;
		}
		box(render_vertices, p->_render_pos.x - 0.01f, p->_render_pos.y - 0.01f, p->_render_pos.x + 0.01f, p->_render_pos.y + 0.01f);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}
