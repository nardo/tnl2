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
	int __moduleReady;
	NPObjectRef socket_instance;
	NPIdentifier *_socket_identifiers;
	zone_allocator _allocator;
	net::socket_event_queue _event_queue;
	
	enum socket_method_id {
		socket_bind,
		socket_set_challenge_response,
		socket_send_to,
		socket_connect,
		socket_connect_introduced,
		socket_introduce,
		socket_accept_challenge,
		socket_accept_connection,
		socket_close_connection,
		socket_send_to_connection,
		socket_method_count,
	};
	static NPIdentifier *get_identifier_table()
	{
		static NPIdentifier socket_method_identifiers[socket_method_count];
		static const NPUTF8 *socket_method_names[] = {
			"bind",
			"set_challenge_response",
			"send_to",
			"connect",
			"connect_introduced",
			"introduce",
			"accept_challenge",
			"accept_connection",
			"close_connection",
			"send_to_connection",
		};
		static bool loaded = false;
		if(!loaded)
		{
			browser->getstringidentifiers(socket_method_names, socket_method_count, socket_method_identifiers);
			loaded = true;
		}
		return socket_method_identifiers;
	}
	
	static torque_socket_handle nacl_socket_create(bool background_thread, void (*socket_notify)(void *), void *socket_notify_data)
	{
		return socket_notify_data;
	}
	
	static void nacl_socket_destroy(torque_socket_handle the_socket)
	{
	}
	
	static bind_result nacl_socket_bind(torque_socket_handle the_socket, struct sockaddr *bound_interface_address)
	{
		tnl_test_instance *ti = static_cast<tnl_test_instance *>(the_socket);
		int result;
		string address_string = net::address(*bound_interface_address).to_string();
		ti->call_method(ti->socket_instance, ti->_socket_identifiers[socket_bind], &result, address_string);
	}
	
	static void nacl_socket_allow_incoming_connections(torque_socket_handle the_socket, int allowed)
	{
	}
	
	static void nacl_socket_set_key_pair(torque_socket_handle the_socket, unsigned key_data_size, unsigned char *the_key)
	{
	}
	
	static void nacl_socket_set_challenge_response(torque_socket_handle the_socket, unsigned challenge_response_size, unsigned char *challenge_response)
	{
		tnl_test_instance *ti = static_cast<tnl_test_instance *>(the_socket);
		empty_type result;
		string challenge_response_string(challenge_response, challenge_response_size);
		ti->call_method(ti->socket_instance, ti->_socket_identifiers[socket_set_challenge_response], &result, challenge_response_string);
	}
	
	static void nacl_socket_write_entropy(torque_socket_handle the_socket, unsigned char entropy[32])
	{
	}
	
	static void nacl_socket_read_entropy(torque_socket_handle the_socket, unsigned char entropy[32])
	{
	}
	
	static int nacl_socket_send_to(torque_socket_handle the_socket, struct sockaddr* remote_host, unsigned data_size, unsigned char *data)
	{
		tnl_test_instance *ti = static_cast<tnl_test_instance *>(the_socket);
		int result;
		string address_string = net::address(*remote_host).to_string();
		string data_string(data, data_size);
		ti->call_method(ti->socket_instance, ti->_socket_identifiers[socket_send_to], &result, address_string, data_string);
		return result;
	}
	
	static torque_connection_id nacl_socket_connect(torque_socket_handle the_socket, struct sockaddr* remote_host, unsigned connect_data_size, unsigned char *connect_data)
	{
		tnl_test_instance *ti = static_cast<tnl_test_instance *>(the_socket);
		int result;
		string address_string = net::address(*remote_host).to_string();
		string connect_data_string(connect_data, connect_data_size);
		string protocol_string;

		ti->call_method(ti->socket_instance, ti->_socket_identifiers[socket_connect], &result, address_string, connect_data_string, protocol_string);
		return result;
	}
	
	static torque_connection_id nacl_socket_connect_introduced(torque_socket_handle the_socket, torque_connection_id introducer, torque_connection_id remote_client_identity, int is_host, unsigned connect_data_size, unsigned char *connect_data)
	{
		tnl_test_instance *ti = static_cast<tnl_test_instance *>(the_socket);
		int result;
		string connect_data_string(connect_data, connect_data_size);
		int intro = introducer;
		int rcid = remote_client_identity;
		ti->call_method(ti->socket_instance, ti->_socket_identifiers[socket_connect_introduced], &result, intro, rcid, is_host,connect_data_string);
		return result;
	}
	
	static void nacl_socket_introduce(torque_socket_handle the_socket, torque_connection_id initiator, torque_connection_id host)
	{
		tnl_test_instance *ti = static_cast<tnl_test_instance *>(the_socket);
		empty_type result;
		int the_initiator = initiator;
		int the_host = host;
		ti->call_method(ti->socket_instance, ti->_socket_identifiers[socket_introduce], &result, the_initiator, the_host);
	}
	
	static void nacl_socket_accept_challenge(torque_socket_handle the_socket, torque_connection_id pending_connection)
	{
		tnl_test_instance *ti = static_cast<tnl_test_instance *>(the_socket);
		empty_type result;
		int pending = pending_connection;
		ti->call_method(ti->socket_instance, ti->_socket_identifiers[socket_accept_challenge], &result, pending);
	}
	
	static void nacl_socket_accept_connection(torque_socket_handle the_socket, torque_connection_id pending_connection)
	{
		tnl_test_instance *ti = static_cast<tnl_test_instance *>(the_socket);
		empty_type result;
		int pending = pending_connection;
		ti->call_method(ti->socket_instance, ti->_socket_identifiers[socket_accept_connection], &result, pending);
	}
	
	static void nacl_socket_close_connection(torque_socket_handle the_socket, torque_connection_id connection_id, unsigned disconnect_data_size, unsigned char *disconnect_data)
	{
		tnl_test_instance *ti = static_cast<tnl_test_instance *>(the_socket);
		empty_type result;
		int conn_id = connection_id;
		string disconnect_data_string(disconnect_data, disconnect_data_size);
		ti->call_method(ti->socket_instance, ti->_socket_identifiers[socket_close_connection], &result, conn_id, disconnect_data_string);
	}
	
	static int nacl_socket_send_to_connection(torque_socket_handle the_socket, torque_connection_id connection_id, unsigned datagram_size, unsigned char buffer[torque_sockets_max_datagram_size])
	{
		tnl_test_instance *ti = static_cast<tnl_test_instance *>(the_socket);
		int result;
		int conn_id = connection_id;
		string datagram(buffer, datagram_size);
		ti->call_method(ti->socket_instance, ti->_socket_identifiers[socket_send_to_connection], &result, conn_id, datagram);
		return result;
	}
	
	static struct torque_socket_event *nacl_socket_get_next_event(torque_socket_handle the_socket)
	{
		tnl_test_instance *ti = static_cast<tnl_test_instance *>(the_socket);
		return ti->get_next_event();
	}
	
	void on_challenge_response(int connection, string key, string message)
	{
		logprintf("tnl2_test got challenge_response(%d, %s, %s)", connection, key.c_str(), message.c_str());
		torque_socket_event *event = _event_queue.post_event(torque_connection_challenge_response_event_type, connection);
		_event_queue.set_event_key(event, key.data(), key.len());
		_event_queue.set_event_data(event, message.data(), message.len());
	}
	
	void on_connect_request(int connection, string key, string message)
	{
		logprintf("tnl2_test got connect_request(%d, %s, %s)", connection, key.c_str(), message.c_str());
		torque_socket_event *event = _event_queue.post_event(torque_connection_requested_event_type, connection);
		_event_queue.set_event_key(event, key.data(), key.len());
		_event_queue.set_event_data(event, message.data(), message.len());
	}
	
	void on_established(int connection)
	{
		logprintf("tnl2_test got established(%d)", connection);
		_event_queue.post_event(torque_connection_established_event_type, connection);
	}
	
	void on_close(int connection, string message)
	{
		logprintf("tnl2_test got close(%d,%s)", connection,message.c_str());
		torque_socket_event *event = _event_queue.post_event(torque_connection_disconnected_event_type, connection);
		_event_queue.set_event_data(event, message.data(), message.len());
	}
	
	void on_packet(int connection, int sequence, string packet)
	{
		logprintf("tnl2_test got packet(%d,%d,%s)", connection, sequence, packet.c_str());
		torque_socket_event *event = _event_queue.post_event(torque_connection_packet_event_type, connection);
		_event_queue.set_event_data(event, packet.data(), packet.len());
		event->packet_sequence = sequence;
	}
	
	void on_socket_packet(string source_address, string packet)
	{
		logprintf("tnl2_test got socket packet(%s,%s)", source_address.c_str(), packet.c_str());
		torque_socket_event *event = _event_queue.post_event(torque_socket_packet_event_type);
		_event_queue.set_event_data(event, packet.data(), packet.len());
		net::address(source_address.c_str()).to_sockaddr(&event->source_address);
	}
	
	void on_packet_delivery_notify(int connection, int sequence, bool status)
	{
		logprintf("tnl2_test got packet_delivery_notify(%d,%d,%d)", connection, sequence, status);
		torque_socket_event *event = _event_queue.post_event(torque_connection_packet_notify_event_type, connection);
		event->packet_sequence = sequence;
		event->delivered = status;
	}

	torque_socket_event *get_next_event()
	{
		if(!_event_queue.has_event())
		{
			_event_queue.clear();
			return 0;
		}
		return _event_queue.dequeue();
	}
	
	static torque_socket_interface *get_nacl_interface()
	{
		static torque_socket_interface _nacl_interface = 
		{
			nacl_socket_create,
			nacl_socket_destroy,
			nacl_socket_bind,
			nacl_socket_allow_incoming_connections,
			nacl_socket_set_key_pair,
			nacl_socket_set_challenge_response,
			nacl_socket_write_entropy,
			nacl_socket_read_entropy,
			nacl_socket_send_to,
			nacl_socket_connect,
			nacl_socket_connect_introduced,
			nacl_socket_introduce,
			nacl_socket_accept_challenge,
			nacl_socket_accept_connection,
			nacl_socket_close_connection,
			nacl_socket_send_to_connection,
			nacl_socket_get_next_event,
		};
		return &_nacl_interface;
	}
	tnl_test_instance() : _event_queue(&_allocator)
	{
		_socket_identifiers = get_identifier_table();
		__moduleReady = 1;
		_color = 0;
		net::address ping_address(net::address::any, 28000);
		
		SOCKADDR ping_sockaddr;
		ping_address.to_sockaddr(&ping_sockaddr);
		_game = new tnl_test::test_game(false, ping_sockaddr, get_nacl_interface(), this);
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
		
		tnl_test::test_game_render_frame_open_gl(_game);
		
		glFlush();
		
		pglSwapBuffers();
		pglMakeCurrent(NULL);
	}
	
	core::int16 handle_event(void *event)
	{
		NPPepperEvent* npevent = reinterpret_cast<NPPepperEvent*>(event);
		tnl_test::position p;
		switch (npevent->type) {
			case NPEventType_MouseDown:
				p.x = npevent->u.mouse.x / float(_width);
				p.y = npevent->u.mouse.y / float(_height);
				logprintf("Got mouse click at (%d, %d) -> (%g, %g)", npevent->u.mouse.x, npevent->u.mouse.y, float(p.x), float(p.y));
				_game->move_my_player_to(p);
				break;
			case NPEventType_MouseUp:
			case NPEventType_MouseMove:
			case NPEventType_MouseEnter:
			case NPEventType_MouseLeave:
				break;
			case NPEventType_MouseWheel:
				break;
			case NPEventType_RawKeyDown:
			case NPEventType_KeyDown:
			case NPEventType_KeyUp:
				break;
			case NPEventType_Char:
				break;
			case NPEventType_Minimize:
			case NPEventType_Focus:
			case NPEventType_Device:
				break;
			case NPEventType_Undefined:
			default:
				break;
		}
		return 1;
	}
	
	void activate_socket()
	{
		net::address bind_address;
		SOCKADDR bind_sockaddr;
		bind_address.to_sockaddr(&bind_sockaddr);
		_game->_net_interface->bind(bind_sockaddr);
	}
	
	void on_pong(string message)
	{
		logprintf("got pong %s", message.c_str());
	}
	
	static void register_class(type_database &db)
	{
		tnl_begin_class(db, tnl_test_instance, scriptable_object, true);
		tnl_slot(db, tnl_test_instance, socket_instance, 0);
		tnl_slot(db, tnl_test_instance, __moduleReady, 0);
		tnl_method(db, tnl_test_instance, activate_socket);
		tnl_method(db, tnl_test_instance, on_challenge_response);
		tnl_method(db, tnl_test_instance, on_connect_request);
		tnl_method(db, tnl_test_instance, on_established);
		tnl_method(db, tnl_test_instance, on_close);
		tnl_method(db, tnl_test_instance, on_packet);
		tnl_method(db, tnl_test_instance, on_socket_packet);
		tnl_method(db, tnl_test_instance, on_packet_delivery_notify);
		tnl_method(db, tnl_test_instance, on_pong);
		tnl_method(db, tnl_test_instance, tick);
		tnl_end_class(db);
	}
};

void plugin_initialize()
{
	pglInitialize();
	ltc_mp = ltm_desc;
	core::_log_prefix = "tnl_test";

	tnl_test_instance::register_class(global_type_database());
	global_plugin.add_class(get_global_type_record<tnl_test_instance>());
	global_plugin.set_plugin_class(get_global_type_record<tnl_test_instance>());
}

void plugin_shutdown()
{
	pglTerminate();
}