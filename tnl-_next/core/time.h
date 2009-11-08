// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#define ONE_DAY         int64(86400000)
#define ONE_HOUR        int64( 3600000)
#define ONE_MINUTE      int64(   60000)
#define ONE_SECOND      int64(    1000)
#define ONE_MILLISECOND int64(       1)

#define UNIX_TIME_TO_INT64(t) ((int64(t) + int64(62135683200)) * ONE_SECOND)

static int8  _DaysInMonth[13]    = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static int8  _DaysInMonthLeap[13]= {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static int32 _DayNumber[13]      = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
static int32 _DayNumberLeap[13]  = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

/// Time, broken out into time/date
struct date_and_time {
   int32 year;         ///< current year
   int32 month;        ///< Month (0-11; 0=january)
   int32 day;          ///< Day of year (0-365)
   int32 hour;         ///< Hours after midnight (0-23)
   int32 minute;       ///< Minutes after hour (0-59)
   int32 second;       ///< seconds after minute (0-59)
   int32 millisecond;  ///< Milliseconds after second (0-999)
};

/// time values are defined as a 64-bit number of milliseconds since 12:00 AM, January 1, 0000
class time
{
	int64 _time;

	bool _is_leap_year(int32 year) const
	{
		return ((year & 3) == 0) && ( ((year % 100) != 0) || ((year % 400) == 0) );
	}

	int32 _days_in_month(int32 month, int32 year) const
	{
		if (_is_leap_year(year))
			return _DaysInMonthLeap[month];
		else
			return _DaysInMonth[month];
	}
public:
	/// Empty constructor.
	time() {}

	/// Constructs the time from a 64-bit millisecond value.
	time(int64 time)
	{
		_time = time;
	}

	/// Constructs the time from specified values.
	time(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 second, int32 millisecond)
	{
		set(year, month, day, hour, minute, second, millisecond);
	}

	/// Constructs the time from the specified DateTime.
	time(const date_and_time &date_time)
	{
		set(date_time);
	}

	/// Sets the time from the specified date_and_time.
	inline void set(const date_and_time &date_time)
	{
		set(date_time.year, date_time.month, date_time.day, date_time.hour, date_time.minute, date_time.second, date_time.millisecond);
	}

	/// Sets the time from specified values.
	void set(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 second, int32 millisecond)
	{
		second += millisecond / 1000;
		millisecond %= 1000;
		minute += second / 60;
		second %= 60;
		hour += minute / 60;
		minute %= 60;
		int32 carry_days = hour / 24;
		hour %= 24;

		bool leap_year = _is_leap_year(year);

		year -= 1;     // all the next operations need (year-1) so do it ahead of time
		int32 gregorian = 365 * year             // number of days since the epoch
						+ (year/4)             // add julian leap year days
						- (year/100)           // subtract century leap years
						+ (year/400)           // add gregorian 400 year leap adjustment
						+ ((367*month-362)/12) // days in prior months
						+ day                  // add days
						+ carry_days;          // add days from time overflow/underflow

		// make days in this year adjustment if leap year
		if (leap_year) {
			if (month > 2)
				gregorian -= 1;
		}
		else
			if (month > 2)
				gregorian -= 2;

		_time  = int64(gregorian) * ONE_DAY;
		_time += int64((hour * ONE_HOUR) +
			(minute * ONE_MINUTE) +
			(second * ONE_SECOND) +
			millisecond);
	}


	/// Gets the date and time values from the time.
	inline void get(date_and_time &dt) const
	{
		get_date(dt.year, dt.month, dt.day);
		get_time(dt.hour, dt.minute, dt.second, dt.millisecond);
	}

	/// Gets the date.
	void get_date(int32 &year, int32 &month, int32 &day) const
	{
		int32 gregorian = (int32)(_time / ONE_DAY);

		int32 prior = gregorian - 1;           // prior days
		int32 years400 = prior / 146097L;      // number of 400 year cycles
		int32 days400 = prior % 146097L;       // days NOT in years400
		int32 years100 = days400 / 36524L;     // number 100 year cycles not checked
		int32 days100 =  days400 % 36524L;     // days NOT already included
		int32 years4 = days100 / 1461L;        // number 4 year cycles not checked
		int32 days4 = days100 % 1461L;         // days NOT already included
		int32 year1 = days4 / 365L;            // number years not already checked
		int32 day1  = days4 % 365L;            // days NOT already included

		year = (400 * years400) + (100 * years100) + (4 * years4) + year1;

		// December 31 of leap year
		if (years100 == 4 || year1 == 4)
			day = 366;
		else
		{
			year += 1;
			day = day1 + 1;
		}

		int32 *day_number;
		if (_is_leap_year(year))
			day_number = _DayNumberLeap;
		else
			day_number = _DayNumber;

		// find month and day in month given computed year and day number,
		month = 0;
		while(day >= day_number[month+1])
			month++;

		day -= day_number[month];
	}

	/// Gets the time since midnight from the Time.
	void get_time(int32 &hour, int32 &minute, int32 &second, int32 &millisecond) const
	{
		// extract time
		int32 time = int32(_time % ONE_DAY);
		hour = time / int32(ONE_HOUR);
		time -= hour * int32(ONE_HOUR);

		minute = time / int32(ONE_MINUTE);
		time -= minute * int32(ONE_MINUTE);

		second = time / int32(ONE_SECOND);
		time -= second * int32(ONE_SECOND);

		millisecond = time;
	}

	/// Gets the total millisecond count.
	int64 get_milliseconds()
	{
		return _time;
	}

	inline const time& operator=(const int64 the_time)
	{
		_time = the_time;
		return *this;
	}

	inline time operator+(const time &the_time) const
	{
		return time(_time + the_time._time);
	}

	inline time operator-(const time &the_time) const
	{
		return time(_time - the_time._time);
	}

	inline const time& operator+=(const time the_time)
	{
		_time += the_time._time;
		return *this;
	}

	inline const time& operator-=(const time the_time)
	{
		_time -= the_time._time;
		return *this;
	}

	inline bool operator==(const time &the_time) const
	{
		return (_time == the_time._time);
	}

	inline bool operator!=(const time &the_time) const
	{
		return (_time != the_time._time);
	}
	inline bool operator<(const time &the_time) const
	{
		return (_time < the_time._time);
	}
	inline bool operator>(const time &the_time) const
	{
		return (_time > the_time._time);
	}
	inline bool operator<=(const time &the_time) const
	{
		return (_time <= the_time._time);
	}
	inline bool operator>=(const time &the_time) const
	{
		return (_time >= the_time._time);
	}

#ifdef PLATFORM_WIN32
	static int64 win32_file_time_to_milliseconds(FILETIME &ft)
	{
	   ULARGE_INTEGER lt;
	   lt.LowPart = ft.dwLowDateTime;
	   lt.HighPart = ft.dwHighDateTime;

	   // FILETIME is a number of 100-nanosecond intervals since January 1, 1601 (UTC).
	   // re-base it to January 1, 0000
	   return lt.QuadPart / 10000 + int64(50491209600) * ONE_SECOND;
	}

	class win32_multimedia_timer
	{
		uint32 resolution;
		uint32 last_timer_time;
		int64 system_time;
		public:
		win32_multimedia_timer()
		{
			TIMECAPS tc;
			timeGetDevCaps(&tc, sizeof(tc));
			resolution = max(uint32(tc.wPeriodMin), uint32(1));
			timeBeginPeriod(resolution);
			last_timer_time = timeGetTime();
			FILETIME ft;
			GetSystemTimeAsFileTime(&ft);
			system_time = win32_file_time_to_milliseconds(ft);
		}

		~win32_multimedia_timer()
		{
			timeEndPeriod(resolution);
		}

		int64 get_current()
		{
			uint32 current = timeGetTime();
			system_time += current - last_timer_time;
			last_timer_time = current;
			return system_time;
		}
	};


	/// Returns the current time.
	static time get_current()
	{
		static win32_multimedia_timer the_timer;
		time current_time = the_timer.get_current();

		return current_time;
	}
#endif
};

