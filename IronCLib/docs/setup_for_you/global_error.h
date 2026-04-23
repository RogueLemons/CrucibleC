#ifndef SETUP_FOR_YOU_GLOBAL_ERROR_H
#define SETUP_FOR_YOU_GLOBAL_ERROR_H

/*
USAGE:
    - Type name: Error
    - All Error types are accessed as Error_<Variant>, e.g. Error_Runtime, Error_FileNotFound, etc.
    - Access error code as integer with Error_get(error) and error message string with Error_to_string(error)
    - Check equality with Error_eq(err1, err2)
*/

#include "ic_typenum.h"
#include "ic_inline.h"

// Define a global error type for the application
#define GLOBAL_ERROR_LIST(X, Type) \
    /* Core / success */ \
    X(Type, NoError, 0, "Everything is fine") \
    \
    /* General / fallback */ \
    X(Type, Unknown, 1, "An unknown error occurred") \
    X(Type, Runtime, 2, "General runtime failure") \
    \
    /* Usage / API errors */ \
    X(Type, Argument, 3, "Illegal argument was provided") \
    X(Type, NullRef, 4, "Illegal null pointer was provided") \
    X(Type, OutOfBounds, 5, "Index or range out of bounds") \
    \
    /* System / OS permissions & resources */ \
    X(Type, Permission, 6, "Failure due to lacking permissions") \
    X(Type, AccessDenied, 11, "Access to resource denied by OS") \
    \
    /* Memory */ \
    X(Type, BadAlloc, 7, "Failed to allocate memory") \
    \
    /* I/O and filesystem */ \
    X(Type, IoError, 8, "Input/output operation failed") \
    X(Type, FileNotFound, 9, "Requested file does not exist") \
    X(Type, FileExists, 10, "File already exists") \
    X(Type, DiskFull, 12, "No space left on device") \
    \
    /* Execution / runtime state */ \
    X(Type, Interrupted, 13, "Operation was interrupted") \
    X(Type, Timeout, 20, "Operation timed out") \
    \
    /* State / lifecycle errors */ \
    X(Type, InvalidState, 14, "Operation not valid in current state") \
    X(Type, NotInitialized, 15, "Component not initialized") \
    X(Type, AlreadyInitialized, 16, "Component already initialized") \
    X(Type, NotFound, 17, "Requested resource was not found") \
    X(Type, AlreadyExists, 18, "Resource already exists") \
    X(Type, Unsupported, 19, "Operation is not supported") \
    \
    /* Data / parsing / integrity */ \
    X(Type, ParseError, 21, "Failed to parse input") \
    X(Type, FormatError, 22, "Invalid data format") \
    X(Type, ChecksumMismatch, 23, "Data integrity check failed") \
    X(Type, CorruptData, 24, "Data is corrupted") \
    \
    /* Networking / external systems */ \
    X(Type, NetworkError, 25, "Network operation failed") \
    X(Type, ConnectionRefused, 26, "Connection was refused") \
    X(Type, HostUnreachable, 27, "Host is unreachable") \
    X(Type, ProtocolError, 28, "Protocol violation or mismatch") \
    \
    /* Concurrency / threading */ \
    X(Type, Deadlock, 29, "Deadlock detected") \
    X(Type, RaceCondition, 30, "Race condition detected") \
    \
    /* Internal fallback */ \
    X(Type, InternalError, 31, "Internal application error")

// Generate a global error type with the provided list
IC_TYPENUM_FULL(Error, int, GLOBAL_ERROR_LIST)

#endif // SETUP_FOR_YOU_GLOBAL_ERROR_H