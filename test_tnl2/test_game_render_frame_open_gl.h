
/// test_game_render_frame_open_gl is called by the platform windowing code to notify the game that it should render the current world using OpenGL.
static void test_game_render_frame_open_gl(test_game *the_game)
{
	glClearColor(1, 1, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// first, render the alpha blended circle around the player,
	// to show the scoping range.
	
	if(the_game->_client_player)
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
	}
	
	// then draw all the _buildings.
	for(int32 i = 0; i < the_game->_buildings.size(); i++)
	{
		building *b = the_game->_buildings[i];
		glBegin(GL_POLYGON);
		glColor3f(1, 0, 0);
		glVertex2f(b->upper_left.x, b->upper_left.y);
		glVertex2f(b->lower_right.x, b->upper_left.y);
		glVertex2f(b->lower_right.x, b->lower_right.y);
		glVertex2f(b->upper_left.x, b->lower_right.y);
		glEnd();
	}
	
	// last, draw all the _players in the game.
	for(int32 i = 0; i < the_game->_players.size(); i++)
	{
		player *p = the_game->_players[i];
		glBegin(GL_POLYGON);
		glColor3f(0,0,0);
		
		glVertex2f(p->_render_pos.x - 0.012f, p->_render_pos.y - 0.012f);
		glVertex2f(p->_render_pos.x + 0.012f, p->_render_pos.y - 0.012f);
		glVertex2f(p->_render_pos.x + 0.012f, p->_render_pos.y + 0.012f);
		glVertex2f(p->_render_pos.x - 0.012f, p->_render_pos.y + 0.012f);
		glEnd();
		
		glBegin(GL_POLYGON);
		switch(p->_player_type)
		{
			case player::player_type_ai:
			case player::player_type_ai_dummy:
				glColor3f(0, 0, 1);
				break;
			case player::player_type_client:
				glColor3f(0.5, 0.5, 1);
				break;
			case player::player_type_my_client:
				glColor3f(1, 1, 1);
				break;
		}
		glVertex2f(p->_render_pos.x - 0.01f, p->_render_pos.y - 0.01f);
		glVertex2f(p->_render_pos.x + 0.01f, p->_render_pos.y - 0.01f);
		glVertex2f(p->_render_pos.x + 0.01f, p->_render_pos.y + 0.01f);
		glVertex2f(p->_render_pos.x - 0.01f, p->_render_pos.y + 0.01f);
		
		glEnd();
	}
}
