// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DTRSTATUS_H__
#define __ARC_DTRSTATUS_H__

#include <string>

namespace DataStaging {
      
  /**
   * A class to be used for tracking and manipulating DTR status.
   */
  class DTRStatus {

  public:
    
    enum DTRStatusType {
    	
      /* Set by the generator */
      /// Just created
      NEW,

      /* Set by the scheduler */
      /// Check the cache for the file may be already there 
      CHECK_CACHE,
      
      /// Resolve a meta-protocol
      RESOLVE,
      
      /// Query a replica
      QUERY_REPLICA,

      /// The destination should be deleted
      PRE_CLEAN,

      /// Prepare or stage the source and/or destination
      STAGE_PREPARE,

      /// Hold the ready transfer
      TRANSFER_WAIT,
      
      /// Transfer ready and can be started
      TRANSFER,
      
      /// Transfer finished, release requests on the storage
      RELEASE_REQUEST,
      
      /// Register a new replica of the destination
      REGISTER_REPLICA,
      
      /// Destination is cacheable, process cache
      PROCESS_CACHE,
      
      /// Everything completed successfully
      DONE,
      
      /// Cancellation request fulfilled successfully
      CANCELLED,
      
      /// Cancellation request fulfilled but DTR also completed transfer successfully
      CANCELLED_FINISHED,

      /// Error occured
      ERROR,
      
      /* Set by the pre-processor */
      /// Checking the cache
      CHECKING_CACHE,

      /// Cache file is locked, waiting for its release
      CACHE_WAIT,
      
      /// Cache check completed
      CACHE_CHECKED,

      /// Resolving replicas
      RESOLVING,

      /// Replica resolution completed
      RESOLVED,

      /// Replica is being queried
      QUERYING_REPLICA,

      /// Replica was queried
      REPLICA_QUERIED,

      /// Deleting the destination
      PRE_CLEANING,

      /// The destination file has been deleted
      PRE_CLEANED,

      /// Making a staging or preparing request
      STAGING_PREPARING,

      /// Wait for the status of the staging/preparing request
      STAGING_PREPARING_WAIT,

      /// Staging/preparing request completed
      STAGED_PREPARED,

      /* Set by the delivery */
      /// Transfer is going
      TRANSFERRING,
      
      /// Transfer is on-going but scheduled for cancellation
      TRANSFERRING_CANCEL,

      /// Transfer completed
      TRANSFERRED,
      
      /* Set by the post-processor */
      /// Releasing staging/preparing request
      RELEASING_REQUEST,

      /// Release of staging/preparing request completed
      REQUEST_RELEASED,
    
      /// Registering a replica in an index service
      REGISTERING_REPLICA,
      
      /// Replica registration completed
      REPLICA_REGISTERED,
 
      /// Releasing locks and copying/linking cache files to the session dir
      PROCESSING_CACHE,
      
      /// Cache processing completed
      CACHE_PROCESSED,

      /// "Stateless" DTR
      NULL_STATE
      
    };
    
    DTRStatus(const DTRStatusType& status, std::string desc="")
      : status(status), desc(desc) {}
    DTRStatus() 
      : status(NEW), desc ("") {}
    ~DTRStatus() {}

    bool operator==(const DTRStatusType& s) const {
      return status == s;
    }
    bool operator==(const DTRStatus& s) const {
      return status == s.status;
    }
  
    bool operator!=(const DTRStatusType& s) const {
      return status != s;
    }
    bool operator!=(const DTRStatus& s) const {
      return status != s.status;
    }
  
    DTRStatus operator=(const DTRStatusType& s) {
      status = s;
      return *this;
    }

    std::string str() const;

    void SetDesc(const std::string& d) {
      desc = d;
    }
    
    std::string GetDesc() const {
      return desc;
    }
    
     DTRStatusType GetStatus() const {
      return status;
    }

  private:
  
    /// status code
    DTRStatusType status;
    /// description set by the owner process
    std::string desc;

  }; // DTRStatus

  /**
   * A class to represent error states reported by various components.
   */
  class DTRErrorStatus {

   public:

    /// A list of error types
    enum DTRErrorStatusType {

      /// No error
      NONE_ERROR,

      /// Internal error in Data Staging logic
      INTERNAL_ERROR,

      /// Attempt to replicate a file to itself
      SELF_REPLICATION_ERROR,

      /// Permanent error with cache
      CACHE_ERROR,

      /// Temporary error with remote service
      TEMPORARY_REMOTE_ERROR,

      /// Permanent error with remote service
      PERMANENT_REMOTE_ERROR,

      /// Transfer rate was too slow
      TRANSFER_SPEED_ERROR,
      
      /// Waited for too long to become staging
      STAGING_TIMEOUT_ERROR
    };

    /// Describes where the error occurred
    enum DTRErrorLocation {

      /// No error
      NO_ERROR_LOCATION,

      /// Error with source
      ERROR_SOURCE,

      /// Error with destination
      ERROR_DESTINATION,

      /// Error during transfer not directly related to source or destination
      ERROR_TRANSFER,

      /// Error occurred in an unknown location
      ERROR_UNKNOWN
    };

    DTRErrorStatus(DTRErrorStatusType status,
                   DTRStatus::DTRStatusType error_state,
                   DTRErrorLocation location,
                   const std::string& desc = ""):
      error_status(status),
      last_error_state(error_state),
      error_location(location),
      desc(desc) {};

    DTRErrorStatus() :
      error_status(NONE_ERROR),
      last_error_state(DTRStatus::NULL_STATE),
      error_location(NO_ERROR_LOCATION),
      desc("") {};

    /** returns the error type */
    DTRErrorStatusType GetErrorStatus() const {
     return error_status;
    }

    /** returns the state in which the error occurred */
    DTRStatus::DTRStatusType GetLastErrorState() const {
      return last_error_state.GetStatus();
    }

    /** returns the location at which the error occurred */
    DTRErrorLocation GetErrorLocation() const {
      return error_location;
    }

    /** returns the error description */
    std::string GetDesc() const {
      return desc;
    }

    bool operator==(const DTRErrorStatusType& s) const {
      return error_status == s;
    }
    bool operator==(const DTRErrorStatus& s) const {
      return error_status == s.error_status;
    }

    bool operator!=(const DTRErrorStatusType& s) const {
      return error_status != s;
    }
    bool operator!=(const DTRErrorStatus& s) const {
      return error_status != s.error_status;
    }

    DTRErrorStatus operator=(const DTRErrorStatusType& s) {
      error_status = s;
      return *this;
    }

   private:
    /// error state
    DTRErrorStatusType error_status;
    /// state that error occurred in
    DTRStatus last_error_state;
    /// place where the error occurred
    DTRErrorLocation error_location;
    /// description of error
    std::string desc;

  };

} // namespace DataStaging

#endif /*__ARC_DTRSTATUS_H_*/
