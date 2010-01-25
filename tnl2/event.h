/// An event to be sent over the network.
///
/// @note TNL implements two methods of network data passing; this is one of them.  See GhostConnection for details of the other, which is referred to as ghosting.
///
/// TNL lets you pass net_event objects across event_connection instances. There are three types of events:
///      - <b>unguaranteed events</b> are events which are sent once. If they don't make it through the link, they are not resent. This is good for quick, frequent status updates which are of transient interest, like voice communication fragments.
///      - <b>guaranteed events</b> are events which are guaranteed to be delivered. If they don't make it through the link, they are sent as needed. This is good for important, one-time information, like which team a user wants to play on, or the current weather.  guaranteed events are processed when they are received, so they may be processed in a different order than they were sent in.
///      - <b>guaranteed_ordered events</b> are events which are guaranteed to be delivered, and whose process methods will be executed in the order the events were sent.  This is good for information which is not only important, but also order-critical, like chat messages or file transfers.
///
/// There are 3 methods that you need to implement if you want to make a basic net_event subclass, and 2 macros you need to call.
///
/// @code
/// // A simple net_event to transmit a string over the network.
/// class SimpleMessageEvent : public net_event
/// {
///    typedef net_event Parent;
///    char *msg;
/// public:
///    SimpleMessageEvent(const char *message = NULL);
///    ~SimpleMessageEvent();
///
///    virtual void pack   (event_connection *conn, bit_stream *bstream)
///    virtual void unpack (event_connection *conn, bit_stream *bstream);
///    virtual void process(event_connection *conn);
///
///    TORQUE_DECLARE_CLASS(SimpleMessageEvent);
/// };
///
/// TNL_IMPLEMENT_NETEVENT(SimpleMessageEvent, NetClassGroupGameMask,0);
/// @endcode
///
/// The first macro called, TORQUE_DECLARE_CLASS() registers the static class functions and ClassRep object that will assign this class a network ID and allow instances to be constructed by ID.
///
/// The second, TNL_IMPLEMENT_NETEVENT(), instantiates the ClassRep and tells it that the instances are net_event objects in the Game group.  The final parameter to the TNL_IMPLEMENT_NETEVENT macro is the version number of the event class.  Versioning allows a server to offer new event services without forcing older clients to be updated.
///
/// In the constructor for the event the guarantee type of the event and the direction it will be allowed to travel over the connection, must be specified by way of the constructor for the base net_event class.  The guarantee type can be one of:
///      - <b>net_event::guaranteed_ordered</b>, for guaranteed, ordered events
///      - <b>net_event::guaranteed</b>, for guaranteed events
///      - <b>net_event::unguaranteed</b>, for unguaranteed events
///
/// It is also a good idea to clearly specify which direction the event is allowed to travel.  If the program has a certain set of message events that are only sent from server to client, then the network system can enforce that error checking automatically, in order to prevent hacks that may otherwise crash or compromise the program.  The valid event directions are:
///      - <b>net_event::DirAny</b>, this event can be sent from server to client or from client to server
///      - <b>net_event::DirServerToClient</b>, this event can only be sent from server to client.  If the server receives an event of this type, it will signal an error on the connection.
///      - <b>net_event::DirClientToServer</b>, this event can only be sent from client to server.  If the client receives an event of this type, it will signal an error on the connection.
///
/// @note TNL allows you to call NetConnection::setLastError() on the event_connection passed to the net_event. This will cause the connection to abort if invalid data is received, specifying a reason to the user.
///
/// Of the 5 methods declared above; the constructor and destructor need only do whatever book-keeping is needed for the specific implementation, in addition to calling the net_event constructor with the direction and type information that the networking system needs to function. In this case, the SimpleMessageEvent simply allocates/deallocates the space for the string, and specifies the event as guaranteed ordered and bidirectional.
///
/// @code
///    SimpleMessageEvent::SimpleMessageEvent(const char *message = NULL)
///           : net_event(net_event::guaranteed_ordered, net_event::DirAny)
///    {
///       // we marked this event as guaranteed_ordered, and it can be sent in any direction
///       if(message)
///          msg = strDuplicate(message);
///       else
///          msg = NULL;
///    }
///
///    SimpleMessageEvent::~SimpleMessageEvent()
///    {
///      strDelete(msg);
///    }
/// @endcode
///
/// The 3 other functions that must be overridden for evern net_event are pack(), unpack() and process().
///
/// <b>pack()</b> is responsible for packing the event over the wire:
///
/// @code
/// void SimpleMessageEvent::pack(event_connection* conn, bit_stream *bstream)
/// {
///   bstream->writeString(msg);
/// }
/// @endcode
///
/// <b>unpack()</b> is responsible for unpacking the event on the other end:
///
/// @code
/// // The networking layer takes care of instantiating a new
/// // SimpleMessageEvent, which saves us a bit of effort.
/// void SimpleMessageEvent::unpack(event_connection *conn, bit_stream *bstream)
/// {
///   char buf[256];
///   bstream->readString(buf);
///   msg = strDuplicate(buf);
/// }
/// @endcode
///
/// <b>process()</b> is called when the network layer is finished with things.  A typical case is that a guaranteed_ordered event is unpacked and stored, but not processed until the events preceding it in the sequence have been process()'d.
///
/// @code
/// // This just prints the event in the log. You might
/// // want to do something more clever here.
/// void SimpleMessageEvent::process(event_connection *conn)
/// {
///   logprintf("Received a SimpleMessageEvent: %s", msg);
///
///   // An example of something more clever - kick people who say bad words.
///   // if(isBadWord(msg)) conn->setLastError("No swearing, naughtypants!");
/// }
/// @endcode
///
/// Posting an event to the remote host on a connection is simple:
///
/// @code
/// event_connection *conn; // We assume you have filled this in.
///
/// conn->postNetEvent(new SimpleMessageEvent("This is a test!"));
/// @endcode
///
/// Finally, for more advanced applications, notify_posted() is called when the event is posted into the send queue, notify_sent() is called whenever the event is sent over the wire, in event_connection::eventWritePacket(). notify_delivered() is called when the packet is finally received or (in the case of unguaranteed packets) dropped.
///
/// @note the TNL_IMPLEMENT_NETEVENT groupMask specifies which "group" of EventConnections the event can be sent over.  See TNL::Object for a further discussion of this.
class net_event : public ref_object
{
   friend class event_connection;
public:
   enum event_direction {
      bidirectional, ///< This event can be sent from either the initiator or the host of the connection.
      host_to_initiator, ///< This event can only be sent from the host to the initiator
      initiator_to_host, ///< This event can only be sent from the initiator to the host
   } _event_direction; ///< Direction this event is allowed to travel in the network

   enum guarantee_type {
      guaranteed_ordered = 0, ///< Event delivery is guaranteed and will be processed in the order it was sent relative to other ordered events.
      guaranteed = 1, ///< Event delivery is guaranteed and will be processed in the order it was received.
      unguaranteed = 2 ///< Event delivery is not guaranteed - however, the event will remain ordered relative to other unguaranteed events.
   } _guarantee_type; ///< Type of data guarantee this event supports

   /// Constructor - should always be called by subclasses.
   ///
   /// Subclasses MUST pass in an event direction and guarantee type, or else the network system will error on the event. Events are by default guaranteed_ordered, however, the default direction is unset which will result in asserts.
   net_event(guarantee_type the_type = guaranteed_ordered, event_direction the_direction = bidirectional)
   {
      _guarantee_type = the_type;
      _event_direction = the_direction;
   }

   /// Pack is called on the origin side of the connection to write an event's data into a packet.
   virtual void pack(event_connection *ps, bit_stream *bstream) = 0;

   /// Unpack is called on the destination side of the connection to read an event's data out of a packet.
   virtual void unpack(event_connection *ps, bit_stream *bstream) = 0;

   /// Process is called to process the event data when it has been unpacked.
   ///
   /// For a guaranteed, ordered event, process is called only once all prior events have been received and processed.  For unguaranteed events, process is called immediately after unpack.
   virtual void process(event_connection *ps) = 0;

   /// notify_posted is called on an event when it is posted to a particular event_connection, before it is added to the send queue.    This allows events to post additional events to the connection that will be send _before_ this event
   virtual void notify_posted(event_connection *ps) {}

   /// notify_sent is called on each event after all of the events for a packet have been written into the packet stream.
   virtual void notify_sent(event_connection *ps) {}

   /// notify_delivered is called on the source event after it has been received and processed by the other side of the connection.
   ///
   /// If the packet delivery fails on an unguaranteed event, madeIt will be false, otherwise it will be true.
   virtual void notify_delivered(event_connection *ps, bool made_it) {}

   /// get_event_direction returns the direction this event is allowed to travel in on a connection
	event_direction get_event_direction()
	{
		return _event_direction;
	}

   /// get_debug_name is used to construct event names for packet logging in debug mode.
	virtual const char *get_debug_name()
	{
		//return get_class_name();
	}
};


/// The IMPLEMENT_NETEVENT macro is used for implementing events that can be sent from server to client or from client to server
#define TNL_IMPLEMENT_NETEVENT(class_name,group_mask,class_version) \
   TORQUE_IMPLEMENT_NETWORK_CLASS(class_name, Torque::NetClassTypeEvent, group_mask, class_version)


