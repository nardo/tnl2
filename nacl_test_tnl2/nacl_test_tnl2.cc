//#include <nacl/nacl_av.h>
//#include <nacl/nacl_srpc.h>
#include <stdlib.h>
#include <stdio.h>
#include <nacl/nacl_npapi.h>
#include <nacl/npruntime.h>
#include <nacl/npapi_extensions.h>
#include <nacl/npupp.h>
#include <pgl/pgl.h>
#include <GLES2/gl2.h>

#include "test_tnl2.h"
#include "plugin_framework/plugin_framework.h"
#include "third_party/npapi/bindings/npapi_extensions_private.h"

const int32_t kCommandBufferSize = 1024 * 1024;
NPExtensions* extensions = NULL;

class tnl_test_instance : public scriptable_object
{
public:
	core::string bind_address;
	float _color;
	int _width, _height;
	NPDeviceContext3D _context3d;
	NPDevice* _device3d;
	PGLContext _pgl_context;
	tnl_test::test_game *_game;
	
	tnl_test_instance()
	{
		_color = 0;
		SOCKADDR addr;
		
		_game = new tnl_test::test_game(true, addr, addr);
	}
	
	~tnl_test_instance()
	{
		delete _game;
		destroy_context();
	}
	
	void init(NPMIMEType pluginType, core::int16 argc, char* argn[], char* argv[])
	{
		if (!extensions) {
			browser->getvalue(get_plugin_instance(), NPNVPepperExtensions,
							  reinterpret_cast<void*>(&extensions));
			// CHECK(extensions);
		}
		
		_device3d = extensions->acquireDevice(get_plugin_instance(), NPPepper3DDevice);
		if (_device3d == NULL) {
			printf("Failed to acquire 3DDevice\n");
			exit(1);
		}
	}
	
	void set_window(NPWindow *window)
	{
		_width = window->width;
		_height = window->height;
		
		if(!_pgl_context)
			init_context();
		browser->pluginthreadasynccall(get_plugin_instance(), tick_callback, this);
	}
	
	void init_context()
	{
		// Initialize a 3D context.
		NPDeviceContext3DConfig config;
		config.commandBufferSize = kCommandBufferSize;
		NPError err = _device3d->initializeContext(get_plugin_instance(), &config, &_context3d);
		if (err != NPERR_NO_ERROR) {
			printf("Failed to initialize 3D context\n");
			exit(1);
		}
		
		// Create a PGL context.
		_pgl_context = pglCreateContext(get_plugin_instance(), _device3d, &_context3d);
		
		// Initialize the demo GL state.
		pglMakeCurrent(_pgl_context);
		pglMakeCurrent(NULL);
	}
	
	void destroy_context()
	{
		// Destroy the PGL context.
		pglDestroyContext(_pgl_context);
		_pgl_context = NULL;
		
		// Destroy the Device3D context.
		_device3d->destroyContext(get_plugin_instance(), &_context3d);				
	}	
	
	static void tick_callback(void *data)
	{
		static_cast<tnl_test_instance *>(data)->tick();
	}

	void tick()
	{
		_game->tick();
		
		logprintf("hit draw (%d, %d)", _width, _height);
		if(!pglMakeCurrent(_pgl_context) && pglGetError() == PGL_CONTEXT_LOST)
		{
			destroy_context();
			init_context();
			pglMakeCurrent(_pgl_context);
		}
		
		glViewport(0, 0, _width, _height);
		_color += 0.05;
		if(_color > 1)
			_color = 0;
		glClearColor(_color, _color, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		
		tnl_test::test_game_render_frame_open_gl(0);
		
		glFlush();
		
		pglSwapBuffers();
		pglMakeCurrent(NULL);
		browser->pluginthreadasynccall(get_plugin_instance(), tick_callback, this);
	}
	
	core::int16 handle_event(void *event)
	{
		return 0;
	}
	
	bool module_ready()
	{
		return true;
	}
	
	static void register_class(type_database &db)
	{
		tnl_begin_class(db, tnl_test_instance, scriptable_object, true);
		tnl_method(db, tnl_test_instance, module_ready);
		tnl_end_class(db);		
	}
};

void plugin_initialize()
{
	pglInitialize();
	ltc_mp = ltm_desc;
	tnl_test_instance::register_class(global_type_database());
	global_plugin.add_class(get_global_type_record<tnl_test_instance>());
	global_plugin.set_plugin_class(get_global_type_record<tnl_test_instance>());
}

void plugin_shutdown()
{
	pglTerminate();
}