static PoolAllocator<ConnectionStringTable::PacketEntry> packetEntryFreeList;

/// ConnectionStringTable is a helper class to EventConnection for reducing duplicated string data sends
class ConnectionStringTable : public StringTableEntryCompressor
{
public:
   enum StringTableConstants{
      EntryCount = 1024,
      EntryBitSize = 10,
   };

   struct Entry; 

   struct PacketEntry {
      PacketEntry *nextInPacket; ///< The next string table entry updated in the packet this is linked in.
      Entry *stringTableEntry; ///< The ConnectionStringTable::Entry this refers to
      StringTableEntry string; ///< The StringTableEntry that was set in that string
   };

public:
   struct PacketList {
      PacketEntry *stringHead;  ///< The head of the linked list of strings sent in this packet.
      PacketEntry *stringTail; ///< The tail of the linked list of strings sent in this packet.

      PacketList() { stringHead = stringTail = NULL; }
   };

   /// An entry in the EventConnection's string table
   struct Entry {
      StringTableEntry string; ///< Global string table entry of this string
                           ///< will be 0 if this string is unused.
      U32 index;           ///< Index of this entry.
      Entry *nextHash;     ///< The next hash entry for this id.
      Entry *nextLink;     ///< The next entry in the LRU list.
      Entry *prevLink;     ///< The prev entry in the LRU list.

      /// Does the other side now have this string?
      bool receiveConfirmed;
   };

private:
   Entry mEntryTable[EntryCount];
   Entry *mHashTable[EntryCount];
   StringTableEntry mRemoteStringTable[EntryCount];
   Entry mLRUHead, mLRUTail;

   NetConnection *mParent;

   /// Pushes an entry to the back of the LRU list.
   inline void pushBack(Entry *entry)
   {
      entry->prevLink->nextLink = entry->nextLink;
      entry->nextLink->prevLink = entry->prevLink;
      entry->nextLink = &mLRUTail;
      entry->prevLink = mLRUTail.prevLink;
      entry->nextLink->prevLink = entry;
      entry->prevLink->nextLink = entry;
   }
public:
   ConnectionStringTable(NetConnection *parent)
	{
	   mParent = parent;
	   for(U32 i = 0; i < EntryCount; i++)
	   {
		  mEntryTable[i].nextHash = NULL;
		  mEntryTable[i].nextLink = &mEntryTable[i+1];
		  mEntryTable[i].prevLink = &mEntryTable[i-1];
		  mEntryTable[i].index = i;
		  mHashTable[i] = NULL;
	   }
	   mLRUHead.nextLink = &mEntryTable[0];
	   mEntryTable[0].prevLink = &mLRUHead;
	   mLRUTail.prevLink = &mEntryTable[EntryCount-1];
	   mEntryTable[EntryCount-1].nextLink = &mLRUTail;
	}

   void write(BitStream &theStream, const StringTableEntry &theEntry)
	{
	   // see if the entry is in the hash table right now
	   U32 hashIndex = string.getIndex() % EntryCount;
	   Entry *sendEntry = NULL;
	   for(Entry *walk = mHashTable[hashIndex]; walk; walk = walk->nextHash)
	   {
		  if(walk->string == string)
		  {
			 // it's in the table
			 // first, push it to the back of the LRU list.
			 pushBack(walk);
			 sendEntry = walk;
			 break;
		  }
	   }
	   if(!sendEntry)
	   {
		  // not in the hash table, means we have to add it
		  // pull the new entry from the LRU list.
		  sendEntry = mLRUHead.nextLink;

		  // push it to the end of the LRU list
		  pushBack(sendEntry);

		  // remove the string from the hash table
		  Entry **hashWalk;
		  for (hashWalk = &mHashTable[sendEntry->string.getIndex() % EntryCount]; *hashWalk; hashWalk = &((*hashWalk)->nextHash))
		  {
			 if(*hashWalk == sendEntry)
			 {
				*hashWalk = sendEntry->nextHash;
				break;
			 }
		  }
		  
		  sendEntry->string = string;
		  sendEntry->receiveConfirmed = false;
		  sendEntry->nextHash = mHashTable[hashIndex];
		  mHashTable[hashIndex] = sendEntry;
	   }
	   stream.writeInt(sendEntry->index, EntryBitSize);
	   if(!stream.writeFlag(sendEntry->receiveConfirmed))
	   {
	#pragma message("Compile error -- fix this")
	//      stream.writeString(sendEntry->string.getString());
		  PacketEntry *entry = packetEntryFreeList.allocate();

		  entry->stringTableEntry = sendEntry;
		  entry->string = sendEntry->string;
		  entry->nextInPacket = NULL;

		  PacketList *note = &mParent->getCurrentWritePacketNotify()->stringList;

		  if(!note->stringHead)
			 note->stringHead = entry;
		  else
			 note->stringTail->nextInPacket = entry;
		  note->stringTail = entry;
	   }
	}

   void read(BitStream &theStream, StringTableEntry *theEntry)
	{
	   U32 index = stream.readInt(EntryBitSize);

	   char buf[256];
	   if(!stream.readFlag())
	   {
		  stream.readString(buf);
		  mRemoteStringTable[index].set(buf);
	   }
	   *theEntry = mRemoteStringTable[index];
	}


   void packet_received(PacketList *note)
	{
	   PacketEntry *walk = note->stringHead;
	   while(walk)
	   {
		  PacketEntry *next = walk->nextInPacket;
		  if(walk->stringTableEntry->string == walk->string)
			 walk->stringTableEntry->receiveConfirmed = true;
		  packetEntryFreeList.deallocate(walk);
		  walk = next;
	   }
	}
	
   void packet_dropped(PacketList *note)
	{
	   PacketEntry *walk = note->stringHead;
	   while(walk)
	   {
		  PacketEntry *next = walk->nextInPacket;
		  packetEntryFreeList.deallocate(walk);
		  walk = next;
	   }
	}
};
