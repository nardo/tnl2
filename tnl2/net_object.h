//-----------------------------------------------------------------------------
/// Superclass for ghostable networked objects.
///
/// @section NetObject_intro Introduction To net_object And Ghosting
///
/// One of the most powerful aspects of the Torque Network Library is its support for ghosting and prioritized, most-recent-state network updates. The way this works is a bit complex, but it is immensely efficient. Let's run through the steps that the server goes through for each client in this part of TNL's architecture:
///   - First, the server determines what objects are in-scope for the client.  This is done by calling perform_scope_query() on the ref_object which is considered the "scope" ref_object. This could be a simulation avatar of the character, a flyby camera, a vehicle the user is controlling, or something else.
///  - Second, it instructs the client to create "ghost" instances of those objects to represent the source objects on the server.  Finally, it sends updates to the ghosts of those objects whose state has been updated on the server, prioritizing the updates based on how relevant the ref_object is to that particular client.
///
/// There several significant advantages to using this networking system:
///  - Efficient network usage, since we only send data that has changed. In addition, since we only care about most-recent data, if a packet is dropped, we don't waste effort trying to deliver stale data.
///  - Cheating protection; since TNL doesn't deliver information about game objects which aren't "in scope", the ability for clients to learn about objects outside their immediate perceptions can be curtailed by an agressive scoping function.
///
/// @section NetObject_Implementation An Example Implementation
///
/// The basis of the ghost implementation in TNL is net_object.  Each net_object maintains an <b>update_mask</b>, a 32 bit word representing up to 32 independent states for the ref_object.  When a net_object's state changes it calls the set_mask_bits method to notify the network layer that the state has changed and needs to be updated on all clients that have that net_object in scope.
///
/// Using a net_object is very simple; let's go through a simple example implementation:
///
/// @code
/// class SimpleNetObject : public net_object
/// {
/// public:
///   typedef net_object parent;
///   TORQUE_DECLARE_CLASS(SimpleNetObject);
/// @endcode
///
/// Above is the standard boilerplate code for a net_object subclass.
///
/// @code
///    char message1[256];
///    char message2[256];
///    enum States {
///       Message1Mask = BIT(0),
///       Message2Mask = BIT(1),
///    };
/// @endcode
///
/// The example class has two ref_object "states" that each instance keeps track of, message1 and message2.  A real game ref_object might have states for health, velocity and position, or some other set of fields.  Each class has 32 bits to work with, so it's possible to be very specific when defining states.  In general, individual state bits should be assigned only to things that are updated independently - so if you update the position field and the velocity at the same time always, you could use a single bit to represent that state change.
///
/// @code
///    SimpleNetObject()
///    {
///       // in order for an ref_object to be considered by the network system,
///       // the ghostable net flag must be set.
///       // the ScopeAlways flag indicates that the ref_object is always scoped
///       // on all active connections.
///       _net_flags.set(ScopeAlways | ghostable);
///       strcpy(message1, "Hello World 1!");
///       strcpy(message2, "Hello World 2!");
///    }
/// @endcode
///
/// Here is the constructor. The code initializes the net flags, indicating that the SimpleNetObject should always be scoped, and that it can be ghosted to remote hosts
///
/// @code
///    uint32 pack_update(ghost_connection *, uint32 mask, bit_stream &stream)
///    {
///       // check which states need to be updated, and write updates
///       if(stream->writeFlag(mask & Message1Mask))
///          stream->writeString(message1);
///       if(stream->writeFlag(mask & Message2Mask))
///          stream->writeString(message2);
///
///       // the return value from pack_update can set which states still
///       // need to be updated for this ref_object.
///       return 0;
///    }
/// @endcode
///
/// Here's half of the meat of the networking code, the pack_update() function. (The other half, unpack_update(), is shown below.) The comments in the code pretty much explain everything, however, notice that the code follows a pattern of if(writeFlag(mask & StateMask)) { ... write data ... }. The pack_update()/unpack_update() functions are responsible for reading and writing the update flags to the BitStream.  This means the ghost_connection doesn't have to send the 32 bit update_mask with every packet.
///
/// @code
///    void unpack_update(ghost_connection *, bit_stream &stream)
///    {
///       // the unpack_update function must be symmetrical to pack_update
///       if(stream->readFlag())
///       {
///          stream->readString(message1);
///          logprintf("Got message1: %s", message1);
///       }
///       if(stream->readFlag())
///       {
///          stream->readString(message2);
///          logprintf("Got message2: %s", message2);
///       }
///    }
/// @endcode
///
/// The other half of the networking code in any net_object, unpack_update(). In SimpleNetObject, all the code does is print the new messages to the log; however, in a more advanced ref_object, the code might trigger animations, update complex ref_object properties, or even spawn new objects, based on what packet data is unpacked.
///
/// @code
///    void setMessage1(const char *msg)
///    {
///       set_mask_bits(Message1Mask);
///       strcpy(message1, msg);
///    }
///    void setMessage2(const char *msg)
///    {
///       set_mask_bits(Message2Mask);
///       strcpy(message2, msg);
///    }
/// @endcode
///
/// Here are the accessors for the two properties. It is good to encapsulate state variables, so that you don't have to remember to make a call to set_mask_bits every time you change anything; the accessors can do it for you. In a more complex ref_object, you might need to set multiple mask bits when you change something; this can be done using the | operator, for instance, set_mask_bits( Message1Mask | Message2Mask ); if you changed both messages.
///
/// @code
/// TNL_IMPLEMENT_NETOBJECT(SimpleNetObject);
/// @endcode
///
/// Finally, we use the net_object implementation macro, TNL_IMPLEMENT_NETOBJECT(), to implement our net_object. It is important that we use this, as it makes TNL perform certain initialization tasks that allow us to send the ref_object over the network. TORQUE_IMPLEMENT_CLASS() doesn't perform these tasks, see
/// the documentation on ClassRep for more details.
///
/// @nosubgrouping
struct ghost_info;
class ghost_connection;
class net_interface;

class net_object : public ref_object
{
	friend class ghost_connection;
	friend class net_interface;
	
	typedef ref_object parent;
	
	uint32 _dirty_mask_bits;
	uint32 _remote_index; ///< The index of this ghost on the other side of the connection.
	ghost_info *_first_object_ref; ///< Head of the linked list of GhostInfos for this ref_object.
	
	ghost_connection *_owning_connection; ///< The connection that owns this ghost, if it's a ghost
	net_interface *_interface; ///< The net_interface this object is visible to -- a limitation of the ghosting system is that any one net_object can only be ghosted over connections within a single interface.
	
protected:
	enum
	{
		is_ghost_flag = bit(1),  ///< Set if this is a ghost.
		ghostable_flag = bit(3),  ///< Set if this ref_object can ghost at all.
	};
	
	uint32 _net_flags;  ///< Flags field describing this ref_object, from NetFlag.
	
public:
	declare_dynamic_class()
	net_object()
	{
		// netFlags will clear itself to 0
		_remote_index = uint32(-1);
		_first_object_ref = NULL;
		_interface = NULL;
		_prev_dirty_list = NULL;
		_next_dirty_list = NULL;
		_dirty_mask_bits = 0;
		_type_rep = 0;
	}
	net_object *_prev_dirty_list;
	net_object *_next_dirty_list;
	type_database::type_rep *_type_rep; ///< set only for ghosts, this is the type descriptor of this ghost object
	
	~net_object()
	{
		while(_first_object_ref)
			_first_object_ref->connection->detach_object(_first_object_ref);
		
		if(_dirty_mask_bits)
		{
			_prev_dirty_list->_next_dirty_list = _next_dirty_list;
			_next_dirty_list->_prev_dirty_list = _prev_dirty_list;
		}
	}
		
	/// on_ghost_add is called on the client side of a connection after the constructor and after the first data update.
	virtual bool on_ghost_add(ghost_connection *the_connection)
	{
	}
	
	/// on_ghost_remove is called on the client side before the destructor when ghost has gone out of scope and is about to be deleted from the client.
	virtual void on_ghost_remove()
	{ }
	
	/// on_ghost_available is called on the server side after the server knows that the ghost is available and addressable via the get_ghost_index().
	virtual void on_ghost_available(ghost_connection * the_connection)
	{ }
	
	/// Notify the network system that one or more of this ref_object's states have
	/// been changed.
	///
	/// @note This is a server side call. It has no meaning for ghosts.
	void set_mask_bits(uint32 or_mask)
	{
		assert(or_mask != 0);
		//Assert(_dirty_mask_bits == 0 || (_prev_dirty_list != NULL || _next_dirty_list != NULL || _dirty_list == this), "Invalid dirty list state.");
		if(!_dirty_mask_bits)
			_interface->add_to_dirty_list(this);

		_dirty_mask_bits |= or_mask;
	}
	
	void set_dirty_state(uint32 state_index)
	{
		set_mask_bits(1 << state_index);
	}
	
	/// Called to determine the relative update priority of an ref_object.
	///
	/// All objects that are in scope and that have out of date states are queried and sorted by priority before being updated.  If there is not enough room in a single packet for all out of date objects, the skipped objects will have an incremented update_skips the next time that connection prepares to send a packet. Typically the update priority is scaled by update_skips so that as data becomes stale, it becomes more of a priority to  update.
	virtual float32 get_update_priority(net_object *scope_object, uint32 update_mask, uint32 update_skips)
	{
		return float32(update_skips) * 0.1f;
	}
	
	/// For a scope ref_object, determine what is in scope.
	///
	/// perform_scope_query is called on a NetConnection's scope ref_object
	/// to determine which objects in the world are in scope for that
	/// connection.
	virtual void perform_scope_query( ghost_connection * connection)
	{
	}
	
	/// get_net_index returns the index tag used to identify the server copy
	/// of a client ref_object.
	uint32 get_net_index() { return _remote_index; }
	
	/// is_ghost returns true if this ref_object is a ghost of a server ref_object.
	bool is_ghost() const
	{
		return (_net_flags & is_ghost_flag) != 0;
	}
	
	/// is_ghostable returns true if this ref_object can be ghosted to any clients.
	bool is_ghostable() const
	{
		return (_net_flags & ghostable_flag) != 0;
	}
	
	/// Return a hash for this ref_object.
	///
	/// @note This is based on its location in memory.
	uint32 get_hash_id() const
	{
		const net_object *ret = this;
		return *((uint32 *) &ret);
	}
};