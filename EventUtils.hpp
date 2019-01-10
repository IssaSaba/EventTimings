#pragma once

#include "Event.hpp"
#include <chrono>
#include <map>
#include <vector>
#include <string>
#include <mpi.h>


/// Class that aggregates durations for a specific event.
class EventData
{
public:
  explicit EventData(std::string _name);

  EventData(std::string _name, int _rank, long _count, long _total,
            long _max, long _min, std::vector<int> _data, Event::StateChanges stateChanges);

  /// Adds an Events data.
  void put(Event* event);

  std::string getName() const;

  /// Get the average duration of all events so far.
  long getAvg() const;

  /// Get the maximum duration of all events so far
  long getMax() const;

  /// Get the minimum duration of all events so far
  long getMin() const;

  /// Get the total duration of all events so far
  long getTotal() const;

  /// Get the number of all events so far
  long getCount() const;

  /// Get the time percentage that the total time of this event took in relation to the global duration.
  int getTimePercentage() const;

  const std::vector<int> & getData() const;

  /// Write one line of CSV data
  void writeCSV(std::ostream &out);

  /// Writes all state changes (ones per line)
  void writeEventLog(std::ostream &out);

  Event::Clock::duration max = Event::Clock::duration::min();
  Event::Clock::duration min = Event::Clock::duration::max();
  Event::Clock::duration total = Event::Clock::duration::zero();

  int rank;
  Event::StateChanges stateChanges;

private:
  std::string name;
  long count = 0;
  std::vector<int> data;
};

/// All EventData of one particular rank
class RankData
{
public:
  RankData();

  /// Records the initialized timestamp
  void initialize();

  /// Records the finalized timestamp
  void finalize();

  /// Adds a new event
  void put(Event* event);

  void addEventData(EventData ed);

  /// Normalizes all Events to zero time of t0
  void normalizeTo(std::chrono::system_clock::time_point t0);

  /// Map of EventName -> EventData, should be private later
  std::map<std::string, EventData> evData;

  std::chrono::steady_clock::duration getDuration();

  std::chrono::system_clock::time_point initializedAt;
  std::chrono::system_clock::time_point finalizedAt;
  
private:
  std::chrono::steady_clock::time_point initializedAtTicks;
  std::chrono::steady_clock::time_point finalizedAtTicks;

  bool isFinalized = false;
  int rank = 0;

};

/// Holds data aggregated from all MPI ranks for one event
struct GlobalEventStats
{
  // std::string name;
  int maxRank, minRank;
  Event::Clock::duration max   = Event::Clock::duration::min();
  Event::Clock::duration min   = Event::Clock::duration::max();
};


using GlobalEvents = std::multimap<std::string, EventData>;

/// High level object that stores data of all events.
/** Call EventRegistry::intialize at the beginning of your application and
EventRegistry::finalize at the end. Event timings will be usuable without calling this
function at all, but global timings as well as percentages do not work this way.  */
class EventRegistry
{
public:
  /// Deleted copy operator for singleton pattern
  EventRegistry(EventRegistry const &) = delete;

  /// Deleted assigment operator for singleton pattern
  void operator=(EventRegistry const &) = delete;

  static EventRegistry & instance();

  /// Sets the global start time
  /**
   * @param[in] applicationName A name that is added to the logfile to distinguish different participants
   * @param[in] runName A name of the run, will be printed as a separate column with each Event.
   * @param[in] comm MPI communicator which is used for barriers and collecting information from ranks.
   */
  void initialize(std::string applicationName = "", std::string runName = "", MPI_Comm comm = MPI_COMM_WORLD);

  /// Sets the global end time
  void finalize();

  /// Clears the registry. needed for tests
  void clear();

  /// Finalizes the timings and calls print. Can be used as a crash handler to still get some timing results.
  void signal_handler(int signal);

  /// Records the event.
  void put(Event* event);

  /// Make this returning a reference or smart ptr?
  Event & getStoredEvent(std::string const & name);

  /// Returns the timestamp of the run, i.e. when the run finished
  std::chrono::system_clock::time_point getTimestamp();

  /// Returns the duration of the run in ms, either currently running, or fixed when run is stopped.
  Event::Clock::duration getDuration();

  /// Prints a verbose report to stdout and a terse one to EventTimings-AppName.log
  void printAll();

  /// Prints the result table to an arbitrary stream.
  /** terse enables a more machine readable format with one event per line, seperated by whitespace. */
  void print(std::ostream &out);

  /// Convenience function: Prints to std::cout
  void print();

  void writeCSV(std::string filename);

  void writeEventLogs(std::string filename);

  void writeTimings(std::string filename);
  
  void writeEvents(std::string filename);

  void printGlobalStats();

  MPI_Comm & getMPIComm();

  /// Currently active prefix. Changing that applies only to newly created events.
  std::string prefix;

  /// A name that is added to the logfile to identify a run
  std::string runName;

private:
  /// Private, empty constructor for singleton pattern
  EventRegistry()
    : globalEvent("_GLOBAL", true, false) // Unstarted, it's started in initialize
  {}

  RankData localRankData;

  /// Holds RankData from all ranks, only populated at rank 0
  std::vector<RankData> globalRankData;

  /// Gather EventData from all ranks on rank 0.
  void collect();

  /// Normalize times among all ranks
  void normalize();

  /// Collects first initialize and last finalize time at rank 0.
  std::pair<std::chrono::system_clock::time_point, std::chrono::system_clock::time_point> collectInitAndFinalize();

  /// Returns length of longest name
  size_t getMaxNameWidth();

  /// Finds the first initialized time and last finalized time in globalRankData
  std::pair<std::chrono::system_clock::time_point, std::chrono::system_clock::time_point> findFirstAndLastTime();

  /// Event for measuring global time, also acts as a barrier
  Event globalEvent;

  bool initialized = false;

  /// Timestamp when the run finished
  std::chrono::system_clock::time_point timestamp;

  /// Map of name -> events for this rank only
  // std::map<std::string, EventData> events;

  std::map<std::string, Event> storedEvents;

  /// A name that is added to the logfile to distinguish different participants
  std::string applicationName;

  /// MPI Communicator
  MPI_Comm comm;
};


