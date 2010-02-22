/// ghost_connection is a subclass of event_connection that manages the transmission (ghosting) and updating of NetObjects over a connection.  The ghost_connection is responsible for doing scoping calculations (on the server side) and transmitting most-recent ghost information to the client.
/// ghosting is the most complex, and most powerful, part of TNL's capabilities. It allows the information sent to clients to be very precisely matched to what they need, so that no excess bandwidth is wasted.  Each ghost_connection has a <b>scope object</b> that is responsible for determining what other net_object instances are relevant to that connection's client.  Each time ghost_connection sends a packet, net_object::perform_scope_query() is called on the scope object, which calls ghost_connection::object_in_scope() for each relevant object.
/// Each object that is in scope, and in need of update (based on its maskbits) is given a priority ranking by calling that object's getUpdatePriority() method.  The packet is then filled with updates, ordered by priority. This way the most important updates get through first, with less important updates being sent as space is available.
/// There is a cap on the maximum number of ghosts that can be active through a ghost_connection at once.  The enum ghost_id_bit_size (defaults to 10) determines how many bits will be used to transmit the ID for each ghost, so the maximum number is 2^ghost_id_bit_size or 1024.  This can be easily raised; see the  ghost_constants enum.
/// Each object ghosted is assigned a ghost ID; the client is <b>only</b> aware of the ghost ID. This acts to enhance simulation security, as it becomes difficult to map objects from one connection to another, or to reliably identify objects from ID alone. IDs are also reassigned based on need, making it hard to track objects that have fallen out of scope (as any object which the player shouldn't see would).
/// resolve_ghost() is used on the client side, and resolveObjectFromGhostIndex() on the server side, to convert ghost IDs to object references.
/// @see net_object for more information on network object functionality.
class ghost_connection : public event_connection
{
	typedef event_connection parent;
public:
	/// ghost_ref tracks an update sent in one packet for the ghost of one net_object.
	///
	/// When we are notified that a pack is sent/lost, this is used to determine what
	/// updates need to be resent and so forth.
	struct ghost_ref
	{
		uint32 mask;              ///< The mask of bits that were updated in this packet
		uint32 ghost_info_flags;    ///< ghost_info::Flags bitset, determes if the ghost is in a
		///  special processing mode (created/deleted)
		ghost_info *ghost;      ///< The ghost information for the object on the connection that sent
		///  the packet this ghost_ref is attached to
		ghost_ref *next_ref;     ///< The next ghost updated in this packet
		ghost_ref *update_chain; ///< A pointer to the ghost_ref on the least previous packet that
		///  updated this ghost, or NULL, if no prior packet updated this ghost
	};
	
	/// Notify structure attached to each packet with information about the ghost updates in the packet
	struct ghost_packet_notify : public event_connection::event_packet_notify
	{
		ghost_ref *ghost_list; ///< list of ghosts updated in this packet
		ghost_packet_notify() { ghost_list = NULL; }
	};
	
	type_database *_type_database;
	
protected:
	
	/// Override of event_connection's alloc_notify, to use the ghost_packet_notify structure.
	packet_notify *alloc_notify() { return new ghost_packet_notify; }
	
	/// Override to properly update the ghost_info's for all ghosts that had upates in the dropped packet.
	void packet_dropped(packet_notify *pnotify)
	{
		parent::packet_dropped(pnotify);
		ghost_packet_notify *notify = static_cast<ghost_packet_notify *>(pnotify);
		
		ghost_ref *packet_ref = notify->ghost_list;
		// loop through all the packRefs in the packet
		
		while(packet_ref)
		{
			ghost_ref *temp = packet_ref->next_ref;
			
			uint32 update_flags = packet_ref->mask;
			
			// figure out which flags need to be updated on the object
			for(ghost_ref *walk = packet_ref->update_chain; walk && update_flags; walk = walk->update_chain)
				update_flags &= ~walk->mask;
			
			// for any flags we haven't updated since this (dropped) packet
			// or them into the mask so they'll get updated soon
			
			if(update_flags)
			{
				if(!packet_ref->ghost->update_mask)
				{
					packet_ref->ghost->update_mask = update_flags;
					ghost_push_non_zero(packet_ref->ghost);
				}
				else
					packet_ref->ghost->update_mask |= update_flags;
			}
			
			// make sure this packet_ref isn't the last one on the ghost_info
			if(packet_ref->ghost->last_update_chain == packet_ref)
				packet_ref->ghost->last_update_chain = NULL;
			
			// if this packet was ghosting an object, set it
			// to re ghost at it's earliest convenience
			
			if(packet_ref->ghost_info_flags & ghost_info::ghosting)
			{
				packet_ref->ghost->flags |= ghost_info::not_yet_ghosted;
				packet_ref->ghost->flags &= ~ghost_info::ghosting;
			}
			
			// otherwise, if it was being deleted,
			// set it to re-delete
			
			else if(packet_ref->ghost_info_flags & ghost_info::killing_ghost)
			{
				packet_ref->ghost->flags |= ghost_info::kill_ghost;
				packet_ref->ghost->flags &= ~ghost_info::killing_ghost;
			}
			
			delete packet_ref;
			packet_ref = temp;
		}
	}
	
	/// Override to update flags associated with the ghosts updated in this packet.
	void packet_received(packet_notify *pnotify)
	{
		parent::packet_received(pnotify);
		ghost_packet_notify *notify = static_cast<ghost_packet_notify *>(pnotify);
		
		ghost_ref *packet_ref = notify->ghost_list;
		
		// loop through all the notifies in this packet
		
		while(packet_ref)
		{
			// make sure this packet_ref isn't the last one on the ghost_info
			if(packet_ref->ghost->last_update_chain == packet_ref)
				packet_ref->ghost->last_update_chain = NULL;
			
			ghost_ref *temp = packet_ref->next_ref;      
			// if this object was ghosting , it is now ghosted
			
			if(packet_ref->ghost_info_flags & ghost_info::ghosting)
			{
				packet_ref->ghost->flags &= ~ghost_info::ghosting;
				if(packet_ref->ghost->obj)
					packet_ref->ghost->obj->on_ghost_available(this);
			}
			// otherwise, if it was dieing, free the ghost
			
			else if(packet_ref->ghost_info_flags & ghost_info::killing_ghost)
				free_ghost_info(packet_ref->ghost);
			
			delete packet_ref;
			packet_ref = temp;
		}
	}
	
	static int compare_priority(const void *a,const void *b)
	{
		ghost_info *ga = *((ghost_info **) a);
		ghost_info *gb = *((ghost_info **) b);

		float32 ret = ga->priority - gb->priority;
		return (ret < 0) ? -1 : ((ret > 0) ? 1 : 0);
	}
	
	/// Performs the scoping query in order to determine if there is data to send from this ghost_connection.
	void prepare_write_packet()
	{
		parent::prepare_write_packet();
		
		if(!does_ghost_from() && !_ghosting)
			return;
		// first step is to check all our polled ghosts:
		
		// 1. Scope query - find if any new objects have come into
		//    scope and if any have gone out.
		
		// Each packet we loop through all the objects with non-zero masks and
		// mark them as "out of scope" before the scope query runs.
		// if the object has a zero update mask, we wait to remove it until it requests
		// an update
		
		for(int32 i = 0; i < _ghost_zero_update_index; i++)
		{
			// increment the updateSkip for everyone... it's all good
			ghost_info *walk = _ghost_array[i];
			walk->update_skip_count++;
			if(!(walk->flags & (ghost_info::scope_local_always)))
				walk->flags &= ~ghost_info::in_scope;
		}
		
		if(_scope_object)
			_scope_object->perform_scope_query(this);
	}
	
	/// Override to write ghost updates into each packet.
	void write_packet(bit_stream &bstream, packet_notify *pnotify)
	{
		parent::write_packet(bstream, pnotify);
		ghost_packet_notify *notify = static_cast<ghost_packet_notify *>(pnotify);
		
		//if(mConnectionParameters.mDebugObjectSizes)
		//  bstream.write_integer(DebugChecksum, 32);
		
		notify->ghost_list = NULL;
		
		if(!does_ghost_from())
			return;
		
		if(!bstream.write_bool(_ghosting && _scope_object.is_valid()))
			return;
		
		// fill a packet (or two) with ghosting data
		
		// 2. call scoped objects' priority functions if the flag set is nonzero
		//    A removed ghost is assumed to have a high priority
		// 3. call updates based on sorted priority until the packet is
		//    full.  set flags to zero for all updated objects
		
		ghost_info *walk;
		
		for(int32 i = _ghost_zero_update_index - 1; i >= 0; i--)
		{
			if(!(_ghost_array[i]->flags & ghost_info::in_scope))
				detach_object(_ghost_array[i]);
		}
		
		uint32 max_index = 0;
		for(int32 i = _ghost_zero_update_index - 1; i >= 0; i--)
		{
			walk = _ghost_array[i];
			if(walk->index > max_index)
				max_index = walk->index;
			
			// clear out any kill objects that haven't been ghosted yet
			if((walk->flags & ghost_info::kill_ghost) && (walk->flags & ghost_info::not_yet_ghosted))
			{
				free_ghost_info(walk);
				continue;
			}
			// don't do any ghost processing on objects that are being killed
			// or in the process of ghosting
			else if(!(walk->flags & (ghost_info::killing_ghost | ghost_info::ghosting)))
			{
				if(walk->flags & ghost_info::kill_ghost)
					walk->priority = 10000;
				else
					walk->priority = walk->obj->get_update_priority(_scope_object, walk->update_mask, walk->update_skip_count);
			}
			else
				walk->priority = 0;
		}
		ghost_ref *update_list = NULL;
		qsort(_ghost_array, _ghost_zero_update_index, sizeof(ghost_info *), compare_priority);
		//quickSort(_ghost_array, _ghost_array + _ghost_zero_update_index, compare_priority);
		// reset the array indices...
		for(int32 i = _ghost_zero_update_index - 1; i >= 0; i--)
			_ghost_array[i]->array_index = i;
		
		int32 send_size = 1;
		while(max_index >>= 1)
			send_size++;
		
		if(send_size < 3)
			send_size = 3;
		
		bstream.write_integer(send_size - 3, 3); // 0-7 3 bit number
		
		uint32 count = 0;
		// 
		for(int32 i = _ghost_zero_update_index - 1; i >= 0 && !bstream.is_full(); i--)
		{
			ghost_info *walk = _ghost_array[i];
			if(walk->flags & (ghost_info::killing_ghost | ghost_info::ghosting))
				continue;
			
			uint32 update_start = bstream.get_bit_position();
			uint32 update_mask = walk->update_mask;
			uint32 returned_mask = 0;
			
			bstream.write_bool(true);
			bstream.write_integer(walk->index, send_size);
			if(!bstream.write_bool(walk->flags & ghost_info::kill_ghost))
			{
				// this is an update of some kind:
				//if(mConnectionParameters.mDebugObjectSizes)
				//	bstream.advanceBitPosition(BitStreamPosBitSize);
				
				int32 start_position = bstream.get_bit_position();
				bool is_initial_update = false;
				type_database::type_rep *type_rep = walk->type_rep;
				
				if(walk->flags & ghost_info::not_yet_ghosted)
				{
					uint32 class_index = type_rep->class_index;
					bstream.write_ranged_uint32(class_index, 0, _type_database->get_indexed_class_count());
					is_initial_update = true;
				}
				
				// write out the mask unless this is the first update (in which case the mask will be all dirty):
				if(!is_initial_update)
					bstream.write_integer(update_mask, type_rep->max_state_index);
				// now loop through all the fields, and if it's in the update mask, blast it into the bit stream:

				void *object_pointer = (void *) walk->obj;
				while(type_rep)
				{
					for(dictionary<type_database::field_rep>::pointer i = type_rep->fields.first(); i; ++i)
					{
						type_database::field_rep *field_rep = i.value();
						uint32 state_mask = 1 << field_rep->state_index;
						
						if(update_mask & state_mask)
							if(!field_rep->write_function(bstream, (void *) ((uint8 *)object_pointer + field_rep->offset)))
								returned_mask |= state_mask;
					}
					type_rep = type_rep->parent_class;
				}
				
				if(is_initial_update)
				{
					walk->type_rep->initial_update_count++;
					walk->type_rep->initial_update_bit_total += bstream.get_bit_position() - start_position;
				}
				else
				{
					walk->type_rep->total_update_count++;
					walk->type_rep->total_update_bit_total += bstream.get_bit_position() - start_position;
				}
				TorqueLogMessageFormatted(LogGhostConnection, ("ghost_connection %s GHOST %d", walk->type_rep->name.c_str(), bstream.get_bit_position() - 16 - start_position));
				
				assert((returned_mask & (~update_mask)) == 0); // Cannot set new bits in packUpdate return
			}
			
			// check for packet overrun, and rewind this update if there
			// was one:
			if(bstream.get_bit_space_available() < minimum_padding_bits)
			{
				bstream.set_bit_position(update_start);
				break;
			}
			
			// otherwise, create a record of this ghost update and
			// attach it to the packet.
			ghost_ref *upd = new ghost_ref;
			
			upd->next_ref = update_list;
			update_list = upd;
			
			if(walk->last_update_chain)
				walk->last_update_chain->update_chain = upd;
			walk->last_update_chain = upd;
			
			upd->ghost = walk;
			upd->ghost_info_flags = 0;
			upd->update_chain = NULL;
			
			if(walk->flags & ghost_info::kill_ghost)
			{
				walk->flags &= ~ghost_info::kill_ghost;
				walk->flags |= ghost_info::killing_ghost;
				walk->update_mask = 0;
				upd->mask = update_mask;
				ghost_push_to_zero(walk);
				upd->ghost_info_flags = ghost_info::killing_ghost;
			}
			else
			{
				if(walk->flags & ghost_info::not_yet_ghosted)
				{
					walk->flags &= ~ghost_info::not_yet_ghosted;
					walk->flags |= ghost_info::ghosting;
					upd->ghost_info_flags = ghost_info::ghosting;
				}
				walk->update_mask = returned_mask;
				if(!returned_mask)
					ghost_push_to_zero(walk);
				upd->mask = update_mask & ~returned_mask;
				walk->update_skip_count = 0;
				count++;
			}
		}
		// count # of ghosts have been updated,
		// _ghost_zero_update_index # of ghosts remain to be updated.
		// no more objects...
		bstream.write_bool(false);
		notify->ghost_list = update_list;
	}
	
	/// Override to read updated ghost information from the packet stream.
	void read_packet(bit_stream &bstream)
	{
		parent::read_packet(bstream);
		
		if(!does_ghost_to())
			return;
		if(!bstream.read_bool())
			return;
		
		int32 id_size;
		id_size = bstream.read_integer( 3 );
		id_size += 3;
		
		// while there's an object waiting...
		
		while(bstream.read_bool())
		{
			uint32 index;
			bool is_initial_update = false;
			//int32 start_position = bstream.getCurPos();
			index = (uint32) bstream.read_integer(id_size);
			if(bstream.read_bool()) // is this ghost being deleted?
			{
				assert(_local_ghosts[index] != NULL); // Error, NULL ghost encountered.
				if(_local_ghosts[index])
				{
					_local_ghosts[index]->on_ghost_remove();
					delete _local_ghosts[index];
					_local_ghosts[index] = NULL;
				}
			}
			else
			{
				uint32 end_position = 0;
				void *object_pointer;
				
				if(!_local_ghosts[index]) // it's a new ghost... cool
				{
					uint32 class_index = bstream.read_ranged_uint32(0, _type_database->get_indexed_class_count() -  1);
					
					type_database::type_rep *type_rep = _type_database->get_indexed_class(class_index);
					
					uint32 instance_size = type_rep->type->size;
					object_pointer = operator new(instance_size);
					type_rep->type->construct_object(object_pointer);
					
					net_object *obj = (net_object *) object_pointer;
					
					obj->_type_rep = type_rep;
					obj->_owning_connection = this;
					obj->_net_flags = net_object::is_ghost_flag;
					
					// object gets initial update before adding to the manager
					
					obj->_remote_index = index;
					_local_ghosts[index] = obj;
					
					is_initial_update = true;
					while(type_rep)
					{
						for(dictionary<type_database::field_rep>::pointer i = type_rep->fields.first(); i; ++i)
						{
							// on initial update, read all the fields
							type_database::field_rep *field_rep = i.value();
							field_rep->read_function(bstream, (void *) ((uint8 *)object_pointer + field_rep->offset));
						}
						type_rep = type_rep->parent_class;
					}
					
					if(!obj->on_ghost_add(this))
						throw tnl_exception_ghost_add_failed;
				}
				else
				{
					object_pointer = (void *) _local_ghosts[index];
					type_database::type_rep *type_rep = _local_ghosts[index]->_type_rep;

					uint32 update_mask = bstream.read_integer(type_rep->max_state_index);
					while(type_rep)
					{
						for(dictionary<type_database::field_rep>::pointer i = type_rep->fields.first(); i; ++i)
						{
							type_database::field_rep *field_rep = i.value();
							uint32 state_mask = 1 << field_rep->state_index;
							
							if(update_mask & state_mask)
								field_rep->read_function(bstream, (void *) ((uint8 *)object_pointer + field_rep->offset));
						}
						type_rep = type_rep->parent_class;
					}
				}
			}
		}
	}
	
	/// Override to check if there is data pending on this ghost_connection.
	bool is_data_to_transmit()
	{
		// once we've run the scope query - if there are no objects that need to be updated,
		// we return false
		return parent::is_data_to_transmit() || _ghost_zero_update_index != 0;
	}
	
	//----------------------------------------------------------------
	// ghost manager functions/code:
	//----------------------------------------------------------------
	
protected:
	/// Array of ghost_info structures used to track all the objects ghosted by this side of the connection. For efficiency, ghosts are stored in three segments - the first segment contains GhostInfos that have pending updates, the second ghostrefs that need no updating, and last, free GhostInfos that may be reused.
	ghost_info **_ghost_array;
	
	
	int32 _ghost_zero_update_index; ///< Index in _ghost_array of first ghost with 0 update mask (ie, with no updates).
	
	int32 _ghost_free_index; ///< index in _ghost_array of first free ghost.
	
	bool _ghosting; ///< Am I currently ghosting objects over?
	bool _scoping; ///< Am I currently allowing objects to be scoped?
	uint32  _ghosting_sequence; ///< Sequence number describing this ghosting session.
	
	net_object **_local_ghosts; ///< Local ghost array for remote objects, or NULL if mGhostTo is false.
	
	ghost_info *_ghost_refs; ///< Allocated array of ghostInfos, or NULL if mGhostFrom is false.
	ghost_info **_ghost_lookup_table; ///< Table indexed by object id->ghost_info, or NULL if mGhostFrom is false.
	
	safe_ptr<net_object> _scope_object;///< The local net_object that performs scoping queries to determine what objects to ghost to the client.
	
	void clear_ghost_info()
	{
		// gotta clear out the ghosts...
		for(packet_notify *walk = _notify_queue_head; walk; walk = walk->next_packet)
		{
			ghost_packet_notify *note = static_cast<ghost_packet_notify *>(walk);
			ghost_ref *del_walk = note->ghost_list;
			note->ghost_list = NULL;
			while(del_walk)
			{
				ghost_ref *next = del_walk->next_ref;
				delete del_walk;
				del_walk = next;
			}
		}
		for(int32 i = 0; i < max_ghost_count; i++)
		{
			if(_ghost_refs[i].array_index < _ghost_free_index)
			{
				detach_object(&_ghost_refs[i]);
				_ghost_refs[i].last_update_chain = NULL;
				free_ghost_info(&_ghost_refs[i]);
			}
		}
		assert((_ghost_free_index == 0) && (_ghost_zero_update_index == 0));
	}
	
	void delete_local_ghosts()
	{
		if(!_local_ghosts)
			return;
		// just delete all the local ghosts,
		// and delete all the ghosts in the current save list
		for(int32 i = 0; i < max_ghost_count; i++)
		{
			if(_local_ghosts[i])
			{
				_local_ghosts[i]->on_ghost_remove();
				delete _local_ghosts[i];
				_local_ghosts[i] = NULL;
			}
		}
	}
	
	
	
	bool validate_ghost_array()
	{
		assert(_ghost_zero_update_index >= 0 && _ghost_zero_update_index <= _ghost_free_index);
		assert(_ghost_free_index <= max_ghost_count);
		int32 i;
		for(i = 0; i < _ghost_zero_update_index; i ++)
		{
			assert(_ghost_array[i]->array_index == i);
			assert(_ghost_array[i]->update_mask != 0);
		}
		for(; i < _ghost_free_index; i ++)
		{
			assert(_ghost_array[i]->array_index == i);
			assert(_ghost_array[i]->update_mask == 0);
		}
		for(; i < max_ghost_count; i++)
		{
			assert(_ghost_array[i]->array_index == i);
		}
		return true;
	}
	
	void free_ghost_info(ghost_info *ghost)
	{
		assert(ghost->array_index < _ghost_free_index);
		if(ghost->array_index < _ghost_zero_update_index)
		{
			assert(ghost->update_mask != 0);
			ghost->update_mask = 0;
			ghost_push_to_zero(ghost);
		}
		ghost_push_zero_to_free(ghost);
		assert(ghost->last_update_chain == NULL);
	}
	
	/// Notifies subclasses that the remote host is about to start ghosting objects.
	virtual void on_start_ghosting() {}                              
	
	/// Notifies subclasses that the server has stopped ghosting objects on this connection.
	virtual void on_end_ghosting() {}
	
public:
	ghost_connection()
	{
		// ghost management data:
		_scope_object = NULL;
		_ghosting_sequence = 0;
		_ghosting = false;
		_scoping = false;
		_ghost_array = NULL;
		_ghost_refs = NULL;
		_ghost_lookup_table = NULL;
		_local_ghosts = NULL;
		_ghost_zero_update_index = 0;
	}
	
	~ghost_connection()
	{
		_clear_all_packet_notifies();
		
		// delete any ghosts that may exist for this connection, but aren't added
		if(_ghost_array)
			clear_ghost_info();
		delete_local_ghosts();
		delete[] _local_ghosts;
		delete[] _ghost_lookup_table;
		delete[] _ghost_refs;
		delete[] _ghost_array;
	}
	
	/// Sets whether ghosts transmit from this side of the connection.
	void set_ghost_from(bool ghost_from)
	{
		if(_ghost_array)
			return;
		
		if(ghost_from)
		{
			_ghost_free_index = _ghost_zero_update_index = 0;
			_ghost_array = new ghost_info *[max_ghost_count];
			_ghost_refs = new ghost_info[max_ghost_count];
			int32 i;
			for(i = 0; i < max_ghost_count; i++)
			{
				_ghost_refs[i].obj = NULL;
				_ghost_refs[i].index = i;
				_ghost_refs[i].update_mask = 0;
			}
			_ghost_lookup_table = new ghost_info *[ghost_lookup_table_size];
			for(i = 0; i < ghost_lookup_table_size; i++)
				_ghost_lookup_table[i] = 0;
		}
	}
	
	/// Sets whether ghosts are allowed from the other side of the connection.
	void set_ghost_to(bool ghost_to)
	{
		if(_local_ghosts) // if ghosting to this is already enabled, silently return
			return;
		
		if(ghost_to)
		{
			_local_ghosts = new net_object *[max_ghost_count];
			for(int32 i = 0; i < max_ghost_count; i++)
				_local_ghosts[i] = NULL;
		}
	}
	
	
	/// Does this ghost_connection ghost NetObjects to the remote host?
	bool does_ghost_from() { return _ghost_array != NULL; } 
	
	/// Does this ghost_connection receive ghosts from the remote host?
	bool does_ghost_to() { return _local_ghosts != NULL; }
	
	/// Returns the sequence number of this ghosting session.
	uint32 get_ghosting_sequence() { return _ghosting_sequence; }
	
	enum ghost_constants {
		ghost_id_bit_size = 10, ///< Size, in bits, of the integer used to transmit ghost IDs
		ghost_lookup_table_size_shift = 10, ///< The size of the hash table used to lookup source NetObjects by remote ghost ID is 1 << ghost_lookup_table_size_shift.
		
		max_ghost_count = (1 << ghost_id_bit_size), ///< Maximum number of ghosts that can be active at any one time.
		ghost_count_bit_size = ghost_id_bit_size + 1, ///< Size of the field needed to transmit the total number of ghosts.
		
		ghost_lookup_table_size = (1 << ghost_lookup_table_size_shift), ///< Size of the hash table used to lookup source NetObjects by remote ghost ID.
		ghost_lookup_table_mask = (ghost_lookup_table_size - 1), ///< Hashing mask for table lookups.
	};
	
	/// Sets the object that is queried at each packet to determine what NetObjects should be ghosted on this connection.
	void set_scope_object(net_object *object)
	{
		if(((net_object *) _scope_object) == object)
			return;
		_scope_object = object;
	}
	
	
	/// Returns the current scope object.
	net_object *get_scope_object() { return (net_object*)_scope_object; }; 
	
	/// Indicate that the specified object is currently in scope.  Method called by the scope object to indicate that the specified object is in scope.
	void object_in_scope(net_object *object)
	{
		if (!_scoping || !does_ghost_from())
			return;
		if (!object->is_ghostable())
			return;
		
		type_database::type_rep *type_rep = _type_database->find_type(object->get_type_record());
						
		assert(type_rep != 0);
						
		int32 index = object->get_hash_id() & ghost_lookup_table_mask;
		
		// check if it's already in scope
		// the object may have been cleared out without the lookupTable being cleared
		// so validate that the object pointers are the same.
		
		for(ghost_info *walk = _ghost_lookup_table[index ]; walk; walk = walk->next_lookup_info)
		{
			if(walk->obj != object)
				continue;
			walk->flags |= ghost_info::in_scope;
			return;
		}
		
		if (_ghost_free_index == max_ghost_count)
			return;
		
		ghost_info *giptr = _ghost_array[_ghost_free_index];
		ghost_push_free_to_zero(giptr);
		giptr->update_mask = 0xFFFFFFFF;
		ghost_push_non_zero(giptr);
		
		giptr->flags = ghost_info::not_yet_ghosted | ghost_info::in_scope;
		
		giptr->obj = object;
		giptr->type_rep = type_rep;
		
		giptr->last_update_chain = NULL;
		giptr->update_skip_count = 0;
		
		giptr->connection = this;
		
		giptr->next_object_ref = object->_first_object_ref;
		if(object->_first_object_ref)
			object->_first_object_ref->prev_object_ref = giptr;
		giptr->prev_object_ref = NULL;
		object->_first_object_ref = giptr;
		
		giptr->next_lookup_info = _ghost_lookup_table[index];
		_ghost_lookup_table[index] = giptr;
		//assert(validate_ghost_array(), "Invalid ghost array!");
	}
	
	/// The specified object should be always in scope for this connection.
	void object_local_scope_always(net_object *obj)
	{
		if(!does_ghost_from())
			return;
		object_in_scope(obj);
		for(ghost_info *walk = _ghost_lookup_table[obj->get_hash_id() & ghost_lookup_table_mask]; walk; walk = walk->next_lookup_info)
		{
			if(walk->obj != obj)
				continue;
			walk->flags |= ghost_info::scope_local_always;
			return;
		}
	}
	
	/// The specified object should not be always in scope for this connection.
	void object_local_clear_always(net_object *object)
	{
		if(!does_ghost_from())
			return;
		for(ghost_info *walk = _ghost_lookup_table[object->get_hash_id() & ghost_lookup_table_mask]; walk; walk = walk->next_lookup_info)
		{
			if(walk->obj != object)
				continue;
			walk->flags &= ~ghost_info::scope_local_always;
			return;
		}
	}
	
	
	/// Given an object's ghost id, returns the ghost of the object (on the client side).
	net_object *resolve_ghost(int32 id)
	{
		if(id == -1)
			return NULL;
		
		return _local_ghosts[id];
	}
	
	
	/// Given an object's ghost id, returns the source object (on the server side).
	net_object *resolve_ghost_parent(int32 id)
	{
		return _ghost_refs[id].obj;
	}
	
	/// Moves the specified ghost_info into the range of the ghost array for non-zero updateMasks.
	void ghost_push_non_zero(ghost_info *info)
	{
		assert(info->array_index >= _ghost_zero_update_index && info->array_index < _ghost_free_index); // Out of range array_index
		assert(_ghost_array[info->array_index] == info); // Invalid array object
		if(info->array_index != _ghost_zero_update_index)
		{
			_ghost_array[_ghost_zero_update_index]->array_index = info->array_index;
			_ghost_array[info->array_index] = _ghost_array[_ghost_zero_update_index];
			_ghost_array[_ghost_zero_update_index] = info;
			info->array_index = _ghost_zero_update_index;
		}
		_ghost_zero_update_index++;
		//assert(validate_ghost_array(), "Invalid ghost array!");
	}
	
	/// Moves the specified ghost_info into the range of the ghost array for zero updateMasks.
	void ghost_push_to_zero(ghost_info *info)
	{
		assert(info->array_index < _ghost_zero_update_index);
		assert(_ghost_array[info->array_index] == info);
		_ghost_zero_update_index--;
		if(info->array_index != _ghost_zero_update_index)
		{
			_ghost_array[_ghost_zero_update_index]->array_index = info->array_index;
			_ghost_array[info->array_index] = _ghost_array[_ghost_zero_update_index];
			_ghost_array[_ghost_zero_update_index] = info;
			info->array_index = _ghost_zero_update_index;
		}
		//assert(validate_ghost_array(), "Invalid ghost array!");
	}
	
	/// Moves the specified ghost_info into the range of the ghost array for free (unused) GhostInfos.
	void ghost_push_zero_to_free(ghost_info *info)
	{
		assert(info->array_index >= _ghost_zero_update_index && info->array_index < _ghost_free_index);
		assert(_ghost_array[info->array_index] == info);
		_ghost_free_index--;
		if(info->array_index != _ghost_free_index)
		{
			_ghost_array[_ghost_free_index]->array_index = info->array_index;
			_ghost_array[info->array_index] = _ghost_array[_ghost_free_index];
			_ghost_array[_ghost_free_index] = info;
			info->array_index = _ghost_free_index;
		}
		//assert(validate_ghost_array(), "Invalid ghost array!");
	}
	
	/// Moves the specified ghost_info from the free area into the range of the ghost array for zero updateMasks.
	inline void ghost_push_free_to_zero(ghost_info *info)
	{
		assert(info->array_index >= _ghost_free_index);
		assert(_ghost_array[info->array_index] == info);
		if(info->array_index != _ghost_free_index)
		{
			_ghost_array[_ghost_free_index]->array_index = info->array_index;
			_ghost_array[info->array_index] = _ghost_array[_ghost_free_index];
			_ghost_array[_ghost_free_index] = info;
			info->array_index = _ghost_free_index;
		}
		_ghost_free_index++;
		//assert(validate_ghost_array(), "Invalid ghost array!");
	}
	
	/// Returns the client-side ghostIndex of the specified server object, or -1 if the object is not available on the client.
	int32 get_ghost_index(net_object *object)
	{
		if(!object)
			return -1;
		if(!does_ghost_from())
			return object->_remote_index;
		int32 index = object->get_hash_id() & ghost_lookup_table_mask;
		
		for(ghost_info *gptr = _ghost_lookup_table[index]; gptr; gptr = gptr->next_lookup_info)
		{
			if(gptr->obj == object && (gptr->flags & (ghost_info::killing_ghost | ghost_info::ghosting | ghost_info::not_yet_ghosted | ghost_info::kill_ghost)) == 0)
				return gptr->index;
		}
		return -1;
	}
	
	
	/// Returns true if the object is available on the client.
	bool is_ghost_available(net_object *object) { return get_ghost_index(object) != -1; }
	
	/// Stops ghosting objects from this ghost_connection to the remote host, which causes all ghosts to be destroyed on the client.
	void reset_ghosting()
	{
		if(!does_ghost_from())
			return;
		// stop all ghosting activity
		// send a message to the other side notifying of this
		
		_ghosting = false;
		_scoping = false;
		rpc(&ghost_connection::rpc_end_ghosting);
		_ghosting_sequence++;
		clear_ghost_info();
		//assert(validate_ghost_array(), "Invalid ghost array!");
	}
	
	/// Begins ghosting objects from this ghost_connection to the remote host, starting with the GhostAlways objects.
	void activate_ghosting()
	{
		if(!does_ghost_from())
			return;
		
		_ghosting_sequence++;
		TorqueLogMessageFormatted(LogGhostConnection, ("ghosting activated - %d", _ghosting_sequence));
		
		assert((_ghost_free_index == 0) && (_ghost_zero_update_index == 0));
		
		// iterate through the ghost always objects and in_scope them...
		// also post em all to the other side.
		
		int32 j;
		for(j = 0; j < max_ghost_count; j++)
		{
			_ghost_array[j] = _ghost_refs + j;
			_ghost_array[j]->array_index = j;
		}
		_scoping = true; // so that object_in_scope will work
		
		rpc(&ghost_connection::rpc_start_ghosting, _ghosting_sequence);
		//assert(validate_ghost_array(), "Invalid ghost array!");
	}
	
	bool is_ghosting() { return _ghosting; } ///< Returns true if this connection is currently ghosting objects to the remote host.
	
	/// Notifies the ghost_connection that the specified ghost_info should no longer be scoped to the client.
	void detach_object(ghost_info *info)
	{
		// mark it for ghost killin'
		info->flags |= ghost_info::kill_ghost;
		
		// if the mask is in the zero range, we've got to move it up...
		if(!info->update_mask)
		{
			info->update_mask = 0xFFFFFFFF;
			ghost_push_non_zero(info);
		}
		if(info->obj)
		{
			if(info->prev_object_ref)
				info->prev_object_ref->next_object_ref = info->next_object_ref;
			else
				info->obj->_first_object_ref = info->next_object_ref;
			if(info->next_object_ref)
				info->next_object_ref->prev_object_ref = info->prev_object_ref;
			// remove it from the lookup table
			
			uint32 id = info->obj->get_hash_id();
			for(ghost_info **walk = &_ghost_lookup_table[id & ghost_lookup_table_mask]; *walk; walk = &((*walk)->next_lookup_info))
			{
				ghost_info *temp = *walk;
				if(temp == info)
				{
					*walk = temp->next_lookup_info;
					break;
				}
			}
			info->prev_object_ref = info->next_object_ref = NULL;
			info->obj = NULL;
		}
	}
	
	/// RPC from server to client before the GhostAlwaysObjects are transmitted
	void rpc_start_ghosting(uint32 sequence)
	{
		TorqueLogMessageFormatted(LogGhostConnection, ("Got GhostingStarting %d", sequence));
		
		if(!does_ghost_to())
			throw tnl_exception_illegal_rpc;
		
		on_start_ghosting();
		rpc(&ghost_connection::rpc_ready_for_normal_ghosts, sequence);
	}
	
	/// RPC from client to server sent when the client receives the rpcGhostAlwaysActivated
	void rpc_ready_for_normal_ghosts(uint32 sequence)
	{
		TorqueLogMessageFormatted(LogGhostConnection, ("Got ready for normal ghosts %d %d", sequence, _ghosting_sequence));
		if(!does_ghost_from())
			throw tnl_exception_illegal_rpc;
		if(sequence != _ghosting_sequence)
			return;
		_ghosting = true;
	}
	
	
	/// RPC from server to client sent to notify that ghosting should stop	
	void rpc_end_ghosting()
	{
		if(!does_ghost_to())
			throw tnl_exception_illegal_rpc;
		delete_local_ghosts();
		on_end_ghosting();
	}
	void register_rpc_methods()
	{
		register_rpc(&ghost_connection::rpc_end_ghosting, rpc_guaranteed_ordered, rpc_bidirectional);
		register_rpc(&ghost_connection::rpc_ready_for_normal_ghosts, rpc_guaranteed_ordered, rpc_bidirectional);
		register_rpc(&ghost_connection::rpc_start_ghosting, rpc_guaranteed_ordered, rpc_bidirectional);
	}

	/*/// Internal method called by net_object RPC events when they are packed.
	void postRPCEvent(NetObjectRPCEvent *theEvent)
	{
		RefPtr<NetObjectRPCEvent> event = theEvent;
		
		Assert((!is_ghost() && theEvent->mRPCDirection == RPCToGhost) ||
			   (is_ghost() && theEvent->mRPCDirection == RPCToGhostParent),
			   "Invalid RPC call - going in the wrong direction!");
		
		// ok, see what kind of an ref_object this is:
		if(is_ghost())
			_owning_connection->postNetEvent(theEvent);
		else if(net_object::getRPCDestConnection())
		{
			net_object::getRPCDestConnection()->postNetEvent(theEvent);
		}
		else
		{
			for(ghost_info *walk = _first_object_ref; walk; walk = walk->next_object_ref)
			{
				if(!(walk->flags & ghost_info::NotAvailable))
					walk->connection->postNetEvent(theEvent);
			}
		}
	}*/
};

//----------------------------------------------------------------------------

/// Each ghost_info structure tracks the state of a single net_object's ghost for a single ghost_connection.
struct ghost_info
{
	// NOTE:
	// if the size of this structure changes, the NetConnection::get_ghost_index function MUST be changed to reflect.

	net_object *obj; ///< The real object on the server.
	uint32 update_mask; ///< The current out-of-date state mask for the object for this connection.
	ghost_connection::ghost_ref *last_update_chain; ///< The ghost_ref for this object in the last packet it was updated in,
	///   or NULL if that last packet has been notified yet.
	ghost_info *next_object_ref;  ///< Next ghost_info for this object in the doubly linked list of GhostInfos across
	///  all connections that scope this object
	ghost_info *prev_object_ref;  ///< Previous ghost_info for this object in the doubly linked list of GhostInfos across
	///  all connections that scope this object

	ghost_connection *connection; ///< The connection that owns this ghost_info
	ghost_info *next_lookup_info;   ///< Next ghost_info in the hash table for net_object*->ghost_info*
	uint32 update_skip_count;         ///< How many times this object has NOT been updated in write_packet

	uint32 flags; ///< Current flag status of this object for this connection.
	float32 priority; ///< Priority for the update of this object, computed after the scoping process has run.
	uint16 index; ///< Fixed index of the object in the _ghost_refs array for the connection, and the ghostId of the object on the client.
	int16 array_index; ///< Position of the object in the _ghost_array for the connection, which changes as the object is pushed to zero, non-zero and free.
	type_database::type_rep *type_rep; ///< type descriptor for this object

	enum Flags
	{
		in_scope = bit(0),             ///< This ghost_info's net_object is currently in scope for this connection.
		scope_local_always = bit(1),    ///< This ghost_info's net_object is always in scope for this connection.
		not_yet_ghosted = bit(2),       ///< This ghost_info's net_object has not been sent to or constructed on the remote host.
		ghosting = bit(3),            ///< This ghost_info's net_object has been sent to the client, but the packet it was sent in hasn't been acked yet.
		kill_ghost = bit(4),           ///< The ghost of this ghost_info's net_object should be destroyed ASAP.
		killing_ghost = bit(5),        ///< The ghost of this ghost_info's net_object is in the process of being destroyed.
		
		/// Flag mask - if any of these are set, the object is not yet available for ghost ID lookup.
		not_available = (not_yet_ghosted | ghosting | kill_ghost | killing_ghost),
	};
};
