/// The ClientPuzzleManager class issues, solves and validates client
/// puzzles for connection authentication.
class ClientPuzzleManager
{
public:
private:
   /// NonceTable manages the list of client nonces for which clients
   /// have constructed valid puzzle solutions for the current server
   /// nonce.  There are 2 nonce tables in the ClientPuzzleManager -
   /// one for the current nonce and one for the previous nonce.

   class NonceTable {
    private:
      struct Entry {
         Nonce mNonce;
         Entry *mHashNext;
      };
      enum {
         MinHashTableSize = 127,
         MaxHashTableSize = 387,
      };

      Entry **mHashTable;
      U32 mHashTableSize;
      PageAllocator<> mChunker;

    public:
      /// NonceTable constructor
      NonceTable() { reset(); }

      /// Resets and clears the nonce table
      void reset()
		{
		   mChunker.releasePages();
		   mHashTableSize = Random::readI(MinHashTableSize, MaxHashTableSize) * 2 + 1;
		   mHashTable = (Entry **) mChunker.allocate(sizeof(Entry *) * mHashTableSize);
		   for(U32 i = 0; i < mHashTableSize; i++)
			  mHashTable[i] = NULL;
		}

      /// checks if the given nonce is already in the table and adds it
      /// if it is not.  Returns true if the nonce was not in the table
      /// when the function was called.
      bool checkAdd(Nonce &theNonce)
		{
		   U32 nonce1 = readU32FromBuffer(theNonce.data);
		   U32 nonce2 = readU32FromBuffer(theNonce.data + 4);

		   U64 fullNonce = (U64(nonce1) << 32) | nonce2;

		   U32 hashIndex = U32(fullNonce % mHashTableSize);
		   for(Entry *walk = mHashTable[hashIndex]; walk; walk = walk->mHashNext)
			  if(walk->mNonce == theNonce)
				 return false;
		   Entry *newEntry = (Entry *) mChunker.allocate(sizeof(Entry));
		   newEntry->mNonce = theNonce;
		   newEntry->mHashNext = mHashTable[hashIndex];
		   mHashTable[hashIndex] = newEntry;
		   return true;
		}

   };

   U32 mCurrentDifficulty;
   Time mLastUpdateTime;
   Time mLastTickTime;

   Nonce mCurrentNonce;
   Nonce mLastNonce;

   NonceTable *mCurrentNonceTable;
   NonceTable *mLastNonceTable;
   static bool checkOneSolution(U32 solution, Nonce &clientNonce, Nonce &serverNonce, U32 puzzleDifficulty, U32 clientIdentity)
	{
	   U8 buffer[8];
	   writeU32ToBuffer(solution, buffer);
	   writeU32ToBuffer(clientIdentity, buffer + 4);

	   hash_state hashState;
	   U8 hash[32];

	   sha256_init(&hashState);
	   sha256_process(&hashState, buffer, sizeof(buffer));
	   sha256_process(&hashState, clientNonce.data, Nonce::NonceSize);
	   sha256_process(&hashState, serverNonce.data, Nonce::NonceSize);
	   sha256_done(&hashState, hash);

	   U32 index = 0;
	   while(puzzleDifficulty > 8)
	   {
		  if(hash[index])
			 return false;
		  index++;
		  puzzleDifficulty -= 8;
	   }
	   U8 mask = 0xFF << (8 - puzzleDifficulty);
	   return (mask & hash[index]) == 0;
	}


public:
   ClientPuzzleManager()
	{
	   mCurrentDifficulty = InitialPuzzleDifficulty;
	   Random::read(mCurrentNonce.data, Nonce::NonceSize);
	   Random::read(mLastNonce.data, Nonce::NonceSize);

	   mCurrentNonceTable = new NonceTable;
	   mLastNonceTable = new NonceTable;
	}

   ~ClientPuzzleManager()
	{
	   delete mCurrentNonceTable;
	   delete mLastNonceTable;
	}

   /// Checks to see if a new nonce needs to be created, and if so
   /// generates one and tosses out the current list of accepted nonces
   void tick(Time currentTime)
	{
	   if(!mLastTickTime)
		  mLastTickTime = currentTime;

	   // use delta of last tick time and current time to manage puzzle
	   // difficulty.

	   // not yet implemented.


	   // see if it's time to refresh the current puzzle:
	   Time timeDelta = currentTime - mLastUpdateTime;
	   if(timeDelta > millisecondsToTime(PuzzleRefreshTime))
	   {
		  mLastUpdateTime = currentTime;
		  mLastNonce = mCurrentNonce;
		  NonceTable *tempTable = mLastNonceTable;
		  mLastNonceTable = mCurrentNonceTable;
		  mCurrentNonceTable = tempTable;

		  mLastNonce = mCurrentNonce;
		  mCurrentNonceTable->reset();
		  Random::read(mCurrentNonce.data, Nonce::NonceSize);
	   }
	}


   /// Error codes that can be returned by checkSolution
   enum ErrorCode {
      Success,
      InvalidSolution,
      InvalidServerNonce,
      InvalidClientNonce,
      InvalidPuzzleDifficulty,
      ErrorCodeCount,
   };

   /// Difficulty levels of puzzles
   enum {
      PuzzleRefreshTime          = 30000, ///< Refresh the server puzzle every 30 seconds
      InitialPuzzleDifficulty    = 17, ///< Initial puzzle difficulty is set so clients do approx 2-3x the shared secret
                                       ///  generation of the server
      MaxPuzzleDifficulty        = 26, ///< Maximum puzzle difficulty is approx 1 minute to solve on ~2004 hardware.
      MaxSolutionComputeFragment = 30, ///< Number of milliseconds spent computing solution per call to solvePuzzle.
      SolutionFragmentIterations = 50000, ///< Number of attempts to spend on the client puzzle per call to solvePuzzle.
   };

   /// Checks a puzzle solution submitted by a client to see if it is a valid solution for the current or previous puzzle nonces
   ErrorCode checkSolution(U32 solution, Nonce &clientNonce, Nonce &serverNonce, U32 puzzleDifficulty, U32 clientIdentity)
	{
	   if(puzzleDifficulty != mCurrentDifficulty)
		  return InvalidPuzzleDifficulty;
	   NonceTable *theTable = NULL;
	   if(serverNonce == mCurrentNonce)
		  theTable = mCurrentNonceTable;
	   else if(serverNonce == mLastNonce)
		  theTable = mLastNonceTable;
	   if(!theTable)
		  return InvalidServerNonce;
	   if(!checkOneSolution(solution, clientNonce, serverNonce, puzzleDifficulty, clientIdentity))
		  return InvalidSolution;
	   if(!theTable->checkAdd(clientNonce))
		  return InvalidClientNonce;
	   return Success;
	}
   /// Computes a puzzle solution value for the given puzzle difficulty and server nonce.  If the execution time of this function
   /// exceeds MaxSolutionComputeFragment milliseconds, it will return the current trail solution in the solution variable and a
   /// return value of false.
   ///
   /// @note Although the behavior of this function can be tweaked using MaxSolutionComputeFragment and
   ///       SolutionFragmentIterations, it's important to bias these settings in favor of rapid puzzle
   ///       completion. A client puzzle is only valid for two times PuzzleRefreshTime, so for about a
   ///       minute, maximum. Most of the time the puzzle can be solved in only a few hundred
   ///       milliseconds. It's better to solve the puzzle fast than to let it drag out, (ie, it's better to
   ///       let your application hitch for a moment whilst calculating than to make the user endure many
   ///       seconds of lag) so reducing the timeout or iterations should be done only if you know what
   ///       you're doing.
   ///
   static bool solvePuzzle(U32 *solution, Nonce &clientNonce, Nonce &serverNonce, U32 puzzleDifficulty, U32 clientIdentity)
	{
	   Time startTime = GetTime();
	   U32 startValue = *solution;

	   // Until we're done...
	   for(;;)
	   {
		  U32 nextValue = startValue + SolutionFragmentIterations;
		  for(;startValue < nextValue; startValue++)
		  {
			 if(checkOneSolution(startValue, clientNonce, serverNonce, puzzleDifficulty, clientIdentity))
			 {
				*solution = startValue;
				return true;
			 }
		  }

		  // Then we check to see if we're out of time...
		  if(GetTime() - startTime > millisecondsToTime(MaxSolutionComputeFragment))
		  {
			 *solution = startValue;
			 return false;
		  }
	   }
	}


   /// Returns the current server nonce
   Nonce getCurrentNonce() { return mCurrentNonce; }

   /// Returns the current client puzzle difficulty
   U32 getCurrentDifficulty() { return mCurrentDifficulty; }
};
