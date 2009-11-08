/// NetClassTypes are used to define the ranges of individual
/// class identifiers, in order to reduce the number of bits
/// necessary to identify the class of an object across the
/// network.
///
/// For example, if there are only 21 classes declared
/// of NetClassTypeObject, the class identifier only needs to
/// be sent using 5 bits.
enum NetClassType {
   NetClassTypeNone = -1, ///< Not an id'able network class
   NetClassTypeObject = 0, ///< Game object classes
   NetClassTypeEvent, ///< Event classes
   NetClassTypeFirstUnused,///< Applications can define their own net class types
};

/// NetClassGroups are used to define different service types
/// for an application.
///
/// Each network-related class can be marked as valid across one or
/// more NetClassGroups.  Each network connection belongs to a
/// particular group, and can only transmit objects that are valid
/// in that group.
enum NetClassGroup
{
   NetClassGroupGame, ///< Group for game related network classes
   NetClassGroupCommunity, ///< Group for community server/authentication classes
   NetClassGroupMaster, ///< Group for simple master server.
   NetClassGroupFirstUnused,///< First unused network class group
   NetClassGroupInvalid = 32, ///< Invalid net class group
};

/// Mask values used to indicate which NetClassGroup(s) a NetObject or NetEvent
/// can be transmitted through.
enum NetClassGroupMask
{
   NetClassGroupGameMask      = 1 << NetClassGroupGame,
   NetClassGroupCommunityMask = 1 << NetClassGroupCommunity,
   NetClassGroupMasterMask    = 1 << NetClassGroupMaster,
};

class BitStream;

/// @section ClassRep_netstuff NetClasses and Class IDs
///
/// TNL supports a notion of group, type, and direction for objects passed over
/// the network. Class IDs are assigned sequentially per-group, per-type, so that, for instance,
/// the IDs assigned to NetObjects are separate from the IDs assigned to NetEvents.
/// This can translate into significant bandwidth savings, since the size of the fields
/// for transmitting these bits are determined at run-time based on the number of IDs given out.
///

namespace NetClassRegistry
{
	static bool Initialized = false;

	struct ClassTracker
	{
	   ClassRep *classRep;
	   U32 classType; ///< Which class type is this?
	   U32 classGroupMask; ///< Mask for which class groups this class belongs to.
	   U32 classVersion; ///< The version number for this class.
	   Array<U32> classId; ///< The id for this class in each class group.

	   U32 initialUpdateBitsUsed; ///< Number of bits used on initial updates of objects of this class.
	   U32 partialUpdateBitsUsed; ///< Number of bits used on partial updates of objects of this class.
	   U32 initialUpdateCount; ///< Number of objects of this class constructed over a connection.
	   U32 partialUpdateCount; ///< Number of objects of this class updated over a connection.
	};

	struct ClassGroupTypeInfo
	{
	   Array<ClassTracker *> classTable;
	   U32 classBitSize;
	};

	// lookup table for class ID to tracker pointer
	static Array<ClassTracker *> ClassIDToTrackerTable;

	// two dimensional array of class data.  First index
	// is by class group, second is by class type.
	static Array<Array<ClassGroupTypeInfo> > ClassTable;

	static Array<ClassTracker> &GetClassTrackerArray()
	{
	   static Array<ClassTracker> theArray;
	   return theArray;
	}
	static inline ClassGroupTypeInfo &LookupClassGroupTypeInfo(U32 groupId, U32 typeId)
	{
	   Assert(groupId < ClassTable.size(), "Out of range group id.");
	   Assert(typeId < ClassTable[groupId].size(), "Out of range type.");
	   return ClassTable[groupId][typeId];
	}

	static inline ClassTracker *LookupClassTracker(U32 groupId, U32 typeId, U32 classIndex)
	{
	   ClassGroupTypeInfo &ti = LookupClassGroupTypeInfo(groupId, typeId);
	   Assert(classIndex < ti.classTable.size(), "Out of range class index.");
	   return ti.classTable[classIndex];
	}

	static inline ClassTracker *LookupClassTrackerByObject(Object *theObject)
	{
	   ClassTracker *ct = ClassIDToTrackerTable[theObject->getClassId()];
	   Assert(ct != 0, ("Class index lookup failed for class (%s) not registered with NetClassRegistry", theObject->getClassName().c_str()));
	   return ct;
	}

   enum {
      InvalidClassIndex = 0xFFFFFFFF,
   };
   /// Adds a class to the NetClassRegistry, with the specified classType, group mask and version number.
   /// This function can be called manually, or it will be called automatically
   /// by the several networking-related TORQUE_IMPLEMENT class macros.
   void AddClass(ClassRep *theClassRep, U32 classType, U32 classGroupMask, U32 networkVersion)
	{
	   Assert(Initialized == 0, "Error, cannot add more classes after NetClassRegistry is initialized.");
	   Array<ClassTracker> &array = GetClassTrackerArray();
	   ClassTracker *t = array.pushBack();
	   t->classRep = theClassRep;
	   t->classType = classType;
	   t->classGroupMask = classGroupMask;
	   t->classVersion = networkVersion;
	   t->initialUpdateBitsUsed = 0;
	   t->partialUpdateBitsUsed = 0;
	   t->initialUpdateCount = 0;
	   t->partialUpdateCount = 0;
	}

   /// Constructs lookup tables for all of the networkable classes.
   /// Should be called after all calls to AddClass.
   void Initialize()
	{
	   if(Initialized)
		  return;
	   Initialized = true;

	   U32 totalClassCount = ClassRep::getClassRepList().size();
	   Array<ClassTracker> &netClassArray = GetClassTrackerArray();
	   
	   ClassIDToTrackerTable.resize(totalClassCount);
	   for(U32 i = 0; i < totalClassCount; i++)
		  ClassIDToTrackerTable[i] = 0;
	   U32 totalGroupCount = 0;
	   U32 totalTypeCount = 0;
	   for(Array<ClassTracker>::Iterator i = netClassArray.begin(); i != netClassArray.end(); i++)
	   {
		  ClassIDToTrackerTable[i->classRep->getClassId()] = i;
		  U32 highestGroup = getBinaryLog2(i->classGroupMask) + 1;
		  if(highestGroup > totalGroupCount)
			 totalGroupCount = highestGroup;
		  if(i->classType + 1 > totalTypeCount)
			 totalTypeCount = i->classType + 1;
	   }
	   for(Array<ClassTracker>::Iterator i = netClassArray.begin(); i != netClassArray.end(); i++)
		  i->classId.resize(totalGroupCount);
	   ClassTable.resize(totalGroupCount);
	   for(U32 i = 0; i < totalGroupCount; i++)
		  ClassTable[i].resize(totalTypeCount);
	   
	   Array<ClassTracker *> dynamicTable;
	   
	   for (U32 group = 0; group < totalGroupCount; group++)
	   {
		  U32 groupMask = 1 << group;
		  for(U32 type = 0; type < totalTypeCount; type++)
		  {
			 for(Array<ClassTracker>::Iterator i = netClassArray.begin(); i != netClassArray.end(); i++)
				if(i->classType == type && i->classGroupMask & groupMask)
				   dynamicTable.pushBack(i);

			 if(!dynamicTable.size())
				continue;

			 quickSort(dynamicTable.begin(), dynamicTable.end(), classTrackerLess);

			 TorqueLogBlock(LogNetBase,
				logprintf("Class Group: %d  Class Type: %d  count: %d",
					  group, type, dynamicTable.size());
				for(U32 i = 0; i < dynamicTable.size(); i++)
				   logprintf("%s", dynamicTable[i]->classRep->getClassName());
			 )
			 ClassTable[group][type].classTable = dynamicTable;

			 for(U32 i = 0; i < ClassTable[group][type].classTable.size();i++)
				ClassTable[group][type].classTable[i]->classId[group] = i;

			 ClassTable[group][type].classBitSize = 
				getBinaryLog2(getNextPowerOfTwo(ClassTable[group][type].classTable.size() + 1));
			 dynamicTable.clear();
		  }
	   }
	}
   
   /// Creates an object of the specified class, type and group, and
   /// returns the instance, or NULL if the values passed in were
   /// out of bounds.
   Object *Create(U32 groupId, U32 typeId, U32 classIndex)
	{
	   return LookupClassTracker(groupId, typeId, classIndex)->classRep->create();
	}
   
   /// Returns the class index of the specified object within the
   /// specified group.  Classes may have different class indexes
   /// within different groups.
   U32 GetClassIndexOfObject(Object *theObject, U32 groupId)
	{
	   return LookupClassTrackerByObject(theObject)->classId[groupId];
	}

   /// Returns the network class type of the specified object.
   U32 GetClassTypeOfObject(Object *theObject)
	{
	   return LookupClassTrackerByObject(theObject)->classType;
	}

   /// Returns the number of classes of the specified type within the specified group.
   U32 GetClassCount(U32 classGroup, U32 classType)
	{
	   return LookupClassGroupTypeInfo(groupId, typeId).classTable.size();   
	}

   /// Returns the number of bits necessary to transmit a class index
   /// of the specified type within the specified group.
   U32 GetClassBitSize(U32 classGroup, U32 classType)
	{
	   return LookupClassGroupTypeInfo(groupId, typeId).classBitSize;   
	}

   /// Returns the class version of the specified class.
   U32 GetClassVersion(U32 classGroup, U32 classType, U32 index)
	{
	   return LookupClassTracker(groupId, typeId, classIndex)->classVersion;
	}

   /// Returns true if the given class count is on a version boundary
   bool IsVersionBorderCount(U32 classGroup, U32 classType, U32 count)
	{
	   ClassGroupTypeInfo &ti = LookupClassGroupTypeInfo(classGroup, classType);
	   
	   return count == ti.classTable.size() ||
		  (count > 0 && ti.classTable[count]->classVersion !=
		  ti.classTable[count - 1]->classVersion);
	}

   /// Returns the ClassRep for the class specified by class index,
   /// type and group.
   ClassRep *GetClassRep(U32 classGroup, U32 classType, U32 index)
	{
	   return LookupClassTracker(groupId, typeId, classIndex)->classRep;
	}

   /// Accumulates the specified bitCount for the class of theObject
   /// in its initial update count.
   void AddInitialUpdate(Object *theObject, U32 bitCount)
	{
	   ClassTracker *t = LookupClassTrackerByObject(theObject);
	   t->initialUpdateCount++;
	   t->initialUpdateBitsUsed += bitCount;
	}

   /// Accumulates the specified bitCount for the class of theObject
   /// in its partial update count.
   void AddPartialUpdate(Object *theObject, U32 bitCount)
	{
	   ClassTracker *t = LookupClassTrackerByObject(theObject);
	   t->partialUpdateCount++;
	   t->partialUpdateBitsUsed += bitCount;
	}

   /// Logs a dump of the current class-wise bit accumulations for
   /// initial and partial object updates.
   void LogBitUsage()
	{
	   Array<ClassTracker> &array = GetClassTrackerArray();
	   logprintf("Net Class Bit Usage:");
	   for(Array<ClassTracker>::Iterator i = array.begin(); i != array.end(); i++)
	   {
		  if(i->initialUpdateBitsUsed)
			 logprintf("%s (Initial) - Count: %d   Avg Size: %g", 
				   i->classRep->getClassName().c_str(), i->initialUpdateCount,
				   i->initialUpdateBitsUsed / F32(i->initialUpdateCount));
		  if(i->partialUpdateBitsUsed)
			 logprintf("%s (Partial) - Count: %d   Avg Size: %g", 
				   i->classRep->getClassName().c_str(), i->partialUpdateCount,
				   i->partialUpdateBitsUsed / F32(i->partialUpdateCount));
	   }   
	}
   
   /// Writes an object's class index, given its class type and class group.
   void WriteClassIndex(BitStream *theStream, U32 classId, U32 classType, U32 classGroup)
	{
	   ClassGroupTypeInfo &ti = LookupClassGroupTypeInfo(classGroup, classType);
	   Assert(classId < ti.classTable.size(), "Out of range class id.");
	   theStream->writeInt(classId, ti.classBitSize);
	}

   /// Reads a class ID for an object, given a class type and class group.  Returns -1 if the class type is out of range
   U32 ReadClassIndex(BitStream *theStream, U32 classType, U32 classGroup)
	{
	   ClassGroupTypeInfo &ti = LookupClassGroupTypeInfo(classGroup, classType);
	   U32 ret = theStream->readInt(ti.classBitSize);
	   if(ret >= ti.classTable.size())
		  return InvalidClassIndex;
	   return ret;
	}

   /// Poorly thought out CRC functionality -- deprecated.
   U32 GetClassGroupCRC(U32 classGroup)
	{
	   return 0xFFFFFFFF;
	}
};

#define TORQUE_IMPLEMENT_NETWORK_CLASS(className, classType, classGroupMask, networkVersion) \
   TORQUE_IMPLEMENT_CLASS(className); \
   TORQUE_STARTUPEXECUTE(className##NetReg) { Torque::NetClassRegistry::AddClass(className::getClassRepStatic(), classType, classGroupMask, networkVersion); }



