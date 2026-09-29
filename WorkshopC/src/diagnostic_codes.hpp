#pragma once

#include <cstddef>

/*
 * Every diagnostic WorkshopC can report has a code CCWrrcc (CrucibleC
 * WorkshopC): 'rr' is the rule and 'cc' the check within that rule.
 *
 *   00  WorkshopC itself (suppression comments)
 *   01  enum                                   12  struct resource management: cleanup
 *   02  private                                13  struct resource management: return values
 *   03  private_alternative                    14  (reserved for future rules)
 *   04  function_pointer                       15  (reserved for future rules)
 *   05  typedef_struct                         16  restricted_malloc
 *   06  assignment                             17  single_return
 *   07  prefix_namespace                       18  strict_switch
 *   08  null_check                             19  global_variable
 *   09  argument_pointer_movement              20  reference_pointer
 *   10  struct resource management: database   21  function_discard
 *   11  struct resource management: init       22  array_struct
 *                                              23  span_struct
 *
 * Codes are never renumbered or reused: a removed check keeps its code
 * unused, and a new check gets the next free number of its rule. The
 * table in README.md (Diagnostic codes) lists them all, keep it in sync.
 *
 * X(enumerator, code, name, description)
 */
#define WORKSHOPC_DIAGNOSTIC_CODES(X) \
    /* 00 WorkshopC */ \
    X(SuppressionNotTurnedBackOn, "CCW0001", "suppression-not-turned-back-on", \
      "'WorkshopC off' is never turned back on with 'WorkshopC on' in the same file") \
    X(SuppressionMissingReason, "CCW0002", "suppression-missing-reason", \
      "'WorkshopC off' is not followed on the next line by a comment starting with 'Reason: '") \
    X(SuppressionOnWithoutOff, "CCW0003", "suppression-on-without-off", \
      "'WorkshopC on' without a preceding 'WorkshopC off' in the same file") \
    X(SuppressionNested, "CCW0004", "suppression-nested", \
      "'WorkshopC off' while already turned off, suppressions can not be nested") \
    /* 01 enum */ \
    X(EnumNotAllowed, "CCW0101", "enum-not-allowed", \
      "Enums are not allowed") \
    X(EnumMissingTypedef, "CCW0102", "enum-missing-typedef", \
      "An enum must have a typedef") \
    X(EnumInitNotMember, "CCW0103", "enum-init-not-member", \
      "An enum variable must be initialized with a member of its enum or an explicit cast") \
    X(EnumAssignmentNotMember, "CCW0104", "enum-assignment-not-member", \
      "An enum value must be assigned a member of its enum or an explicit cast") \
    X(EnumArithmetic, "CCW0105", "enum-arithmetic", \
      "An enum value may not be modified with arithmetic (compound assignment, ++ or --)") \
    X(EnumArgumentNotMember, "CCW0106", "enum-argument-not-member", \
      "An enum argument must be a member of its enum or an explicit cast") \
    /* 02 private */ \
    X(PrivateAccessOutsideFunction, "CCW0201", "private-access-outside-function", \
      "A private field is accessed outside of any function") \
    X(PrivateAccess, "CCW0202", "private-access", \
      "A private field is accessed from a function that is not a static getter or setter in a .c file") \
    /* 03 private_alternative */ \
    X(PrivateAlternativeAccessOutsideFunction, "CCW0301", "private-alternative-access-outside-function", \
      "A private field is accessed outside of any function") \
    X(PrivateAlternativeAccess, "CCW0302", "private-alternative-access", \
      "A private field is accessed from a function that is not named after its struct and does not take 'self'") \
    /* 04 function_pointer */ \
    X(FunctionPointerMissingTypedef, "CCW0401", "function-pointer-missing-typedef", \
      "A function pointer variable, parameter, struct field or return type is declared without a typedef") \
    /* 05 typedef_struct */ \
    X(StructMissingTypedef, "CCW0501", "struct-missing-typedef", \
      "A struct must have a typedef") \
    /* 06 assignment */ \
    X(VariableUninitialized, "CCW0601", "variable-uninitialized", \
      "A variable must be initialized at declaration") \
    X(ArrayUninitialized, "CCW0602", "array-uninitialized", \
      "An array must be initialized at declaration") \
    X(PointerObjectZeroInitialized, "CCW0603", "pointer-object-zero-initialized", \
      "An object containing pointers may not be initialized with {0}") \
    X(NullPointerFieldInitializer, "CCW0604", "null-pointer-field-initializer", \
      "NULL is used in the initializer of a pointer field") \
    X(NullPointerArrayInitializer, "CCW0605", "null-pointer-array-initializer", \
      "NULL is used in the initializer of an array of pointers") \
    X(PointerArrayPartiallyInitialized, "CCW0606", "pointer-array-partially-initialized", \
      "An array of pointers must explicitly initialize every element") \
    X(MutableArgumentPointer, "CCW0607", "mutable-argument-pointer", \
      "The address of an argument is taken as a pointer to non-const") \
    X(PointerAssignedNull, "CCW0608", "pointer-assigned-null", \
      "A pointer is assigned or initialized with NULL") \
    X(PointerFieldAssignedNull, "CCW0609", "pointer-field-assigned-null", \
      "A pointer field is assigned NULL") \
    X(ArgumentReassigned, "CCW0610", "argument-reassigned", \
      "A function argument is reassigned, or changed with ++ or --") \
    X(ByValueArgumentModified, "CCW0611", "by-value-argument-modified", \
      "A field of a by-value argument is modified") \
    X(NullArgument, "CCW0612", "null-argument", \
      "NULL is passed as an argument") \
    /* 07 prefix_namespace */ \
    X(MissingNamespacePrefix, "CCW0701", "missing-namespace-prefix", \
      "A name does not start with the namespace prefix of its file") \
    X(MissingIncludeGuard, "CCW0702", "missing-include-guard", \
      "A header does not have the expected include guard") \
    /* 08 null_check */ \
    X(DereferenceBeforeNullCheck, "CCW0801", "dereference-before-null-check", \
      "A pointer parameter is dereferenced before it is checked for null") \
    /* 09 argument_pointer_movement */ \
    X(MovementTagMissing, "CCW0901", "movement-tag-missing", \
      "A non-const pointer parameter has no movement attribute") \
    X(MovementTagMismatch, "CCW0902", "movement-tag-mismatch", \
      "The movement attribute of a parameter differs between declaration and definition") \
    X(MovementTagNotOnParameter, "CCW0903", "movement-tag-not-on-parameter", \
      "A movement attribute is used on something other than a parameter of a function or function pointer type") \
    X(BorrowedPointerMoved, "CCW0904", "borrowed-pointer-moved", \
      "A modify or out parameter is moved to another function") \
    X(OperatorOnUntaggedParameter, "CCW0905", "operator-on-untagged-parameter", \
      "A callsite operator is used for a parameter without a movement attribute") \
    X(OperatorDisabled, "CCW0906", "operator-disabled", \
      "A callsite operator is used while that kind of callsite operator is disabled") \
    X(OperatorMissing, "CCW0907", "operator-missing", \
      "A callsite operator is missing for a parameter with a movement attribute") \
    X(UseAfterMove, "CCW0908", "use-after-move", \
      "A pointer is used after it may have been moved") \
    X(FunctionPointerMovementTagMismatch, "CCW0909", "function-pointer-movement-tag-mismatch", \
      "A function or function pointer with other movement tags is assigned or passed to a function pointer, or called with it through a conditional") \
    /* 10 struct resource management: database */ \
    X(StructInvalidConstructor, "CCW1001", "struct-invalid-constructor", \
      "A struct does not have exactly one pod, raii or free constructor function") \
    X(StructMissingDestroy, "CCW1002", "struct-missing-destroy", \
      "A raii struct is missing its destroy function") \
    X(StructMissingCopy, "CCW1003", "struct-missing-copy", \
      "A raii struct is missing its copy function") \
    X(StructMissingMove, "CCW1004", "struct-missing-move", \
      "A raii struct is missing its move function") \
    X(StructMissingReturn, "CCW1005", "struct-missing-return", \
      "A raii struct is missing its return function") \
    X(StructMissingValid, "CCW1006", "struct-missing-valid", \
      "A raii struct is missing its validation function") \
    /* 11 struct resource management: init */ \
    X(PodInit, "CCW1101", "pod-init", \
      "A pod struct variable is not initialized from a function return value or another struct variable") \
    X(RaiiInit, "CCW1102", "raii-init", \
      "A raii struct variable is not initialized from a function return value") \
    X(PodArrayInit, "CCW1103", "pod-array-init", \
      "A pod struct array does not initialize every element from a function return value or another struct variable") \
    X(RaiiArrayMultidimensional, "CCW1104", "raii-array-multidimensional", \
      "A raii struct array outside of a struct has more than one dimension") \
    X(RaiiArrayInit, "CCW1105", "raii-array-init", \
      "A raii struct array does not initialize every element from a function return value") \
    X(RaiiArrayMissingDestroyArray, "CCW1106", "raii-array-missing-destroy-array", \
      "A raii struct array is used but the struct has no array destroy function") \
    X(StructArrayNotAllowed, "CCW1107", "struct-array-not-allowed", \
      "An array of this kind of struct is only allowed inside structs") \
    X(RaiiReassigned, "CCW1108", "raii-reassigned", \
      "A raii struct is reassigned with another struct value") \
    X(PodArgument, "CCW1109", "pod-argument", \
      "A pod struct argument is not a function return value or another struct variable") \
    X(RaiiArgument, "CCW1110", "raii-argument", \
      "A raii struct argument is not a function return value") \
    X(PodFieldNotPod, "CCW1111", "pod-field-not-pod", \
      "A struct field inside a pod struct is not a pod struct") \
    X(MoveNotValueRef, "CCW1112", "move-not-value-ref", \
      "A raii move function is given something other than the address of a variable") \
    /* 12 struct resource management: cleanup */ \
    X(RaiiNotDestroyed, "CCW1201", "raii-not-destroyed", \
      "A raii struct variable or array is not destroyed before scope exit") \
    X(RaiiParameterNotDestroyed, "CCW1202", "raii-parameter-not-destroyed", \
      "A raii struct parameter is not destroyed before scope exit") \
    X(RaiiUseAfterDestroy, "CCW1203", "raii-use-after-destroy", \
      "A raii struct is used after being destroyed") \
    X(DestroyNotValueRef, "CCW1204", "destroy-not-value-ref", \
      "A raii destroy function is given something other than the address of a variable") \
    X(DestroyArrayFirstArgument, "CCW1205", "destroy-array-first-argument", \
      "The first argument of an array destroy function is not the array itself") \
    X(DestroyArraySize, "CCW1206", "destroy-array-size", \
      "The second argument of an array destroy function is not the size of the array") \
    X(RaiiDestroyNotReverseOrder, "CCW1207", "raii-destroy-not-reverse-order", \
      "A raii struct is destroyed before a later-declared raii struct") \
    /* 13 struct resource management: return values */ \
    X(RaiiReturnFunctionOutsideReturn, "CCW1301", "raii-return-function-outside-return", \
      "A raii return function is used outside of a return statement") \
    X(PodReturn, "CCW1302", "pod-return", \
      "A function returning a pod struct does not return a function return value or another struct variable") \
    X(RaiiReturn, "CCW1303", "raii-return", \
      "A function returning a raii struct does not return a function call") \
    X(RaiiReturnMemberAccess, "CCW1304", "raii-return-member-access", \
      "A member is accessed directly on a raii struct returned by a function") \
    X(RaiiReturnDiscarded, "CCW1305", "raii-return-discarded", \
      "A raii struct returned by a function is discarded") \
    /* 16 restricted_malloc */ \
    X(RestrictedMalloc, "CCW1601", "restricted-malloc", \
      "A memory function is used outside of the allowed functions") \
    /* 17 single_return */ \
    X(MultipleReturns, "CCW1701", "multiple-returns", \
      "A function has more than a single return") \
    X(MissingFinalReturn, "CCW1702", "missing-final-return", \
      "A function does not end with a return statement") \
    /* 18 strict_switch */ \
    X(SwitchFallthrough, "CCW1801", "switch-fallthrough", \
      "A switch case does not end with a break or return") \
    X(SwitchMissingDefault, "CCW1802", "switch-missing-default", \
      "A switch statement has no default case") \
    /* 19 global_variable */ \
    X(GlobalMissingPrefix, "CCW1901", "global-missing-prefix", \
      "A global variable does not start with the required prefix") \
    X(GlobalNotCapitals, "CCW1902", "global-not-capitals", \
      "A global variable is not written in capital letters") \
    X(GlobalNotStatic, "CCW1903", "global-not-static", \
      "A global variable is not static") \
    X(GlobalNotConst, "CCW1904", "global-not-const", \
      "A global variable is not const all the way through") \
    X(GlobalStaticInHeader, "CCW1905", "global-static-in-header", \
      "A static variable is defined in a header") \
    /* 20 reference_pointer */ \
    X(ReferenceInvalidArgument, "CCW2001", "reference-invalid-argument", \
      "The argument for a reference parameter is not the address of an object or another reference") \
    X(ReferenceReassigned, "CCW2002", "reference-reassigned", \
      "A reference pointer is reassigned") \
    X(ReferenceTagOnNonPointer, "CCW2003", "reference-tag-on-non-pointer", \
      "A reference tag is used on a parameter that is not a pointer") \
    X(ReferenceTagNotOnParameter, "CCW2004", "reference-tag-not-on-parameter", \
      "A reference tag is used on something other than a parameter of a function or function pointer type") \
    X(FunctionPointerReferenceTagMismatch, "CCW2005", "function-pointer-reference-tag-mismatch", \
      "A function or function pointer with other reference tags is assigned or passed to a function pointer, or called with it through a conditional") \
    /* 21 function_discard */ \
    X(FunctionReturnDiscarded, "CCW2101", "function-return-discarded", \
      "A non-void function return value is discarded") \
    /* 22 array_struct */ \
    X(ArrayOutsideStruct, "CCW2201", "array-outside-struct", \
      "An array may only be declared as a field inside a struct") \
    X(ArrayPassedToNonLibraryFunction, "CCW2202", "array-passed-to-non-library-function", \
      "An array field may only be passed directly to a standard-library or third-party function") \
    /* 23 span_struct */ \
    X(SpanInvalidDefinition, "CCW2301", "span-invalid-definition", \
      "A span struct must contain only a pointer field named data and then a size_t field named size") \
    X(SpanUninitialized, "CCW2302", "span-uninitialized", \
      "A span struct must be initialized at declaration") \
    X(SpanArrayCount, "CCW2303", "span-array-count", \
      "A span struct array initializer must use the array element count") \
    X(SpanArrayPassedToNonLibraryFunction, "CCW2304", "span-array-passed-to-non-library-function", \
      "An array may only be passed to a standard-library, third-party, span, or pod function") \
    X(SpanConstMismatch, "CCW2305", "span-const-mismatch", \
      "A span struct holds a pointer to const data, or a const span struct a pointer to non-const data") \
    X(SpanDataPassedToNonLibraryFunction, "CCW2306", "span-data-passed-to-non-library-function", \
      "The data of a span, or a pointer returned by a function named after it, is passed to a function that is not a standard-library or third-party function") \
    X(SpanMissingAfterArray, "CCW2307", "span-missing-after-array", \
      "An array outside of a struct is not followed right away by a span variable holding the whole array") \
    X(SpanDataOutsideWrapper, "CCW2308", "span-data-outside-wrapper", \
      "The data of a span is passed on outside of a static function with a single statement") \
    X(ArrayUsedAfterSpan, "CCW2309", "array-used-after-span", \
      "An array that has a span is used directly instead of through its span")

enum class DiagCode {
#define WORKSHOPC_CODE_ENUM(enumerator, code, name, description) enumerator,
    WORKSHOPC_DIAGNOSTIC_CODES(WORKSHOPC_CODE_ENUM)
#undef WORKSHOPC_CODE_ENUM
};

struct DiagnosticCodeInfo {
    const char *id;
    const char *name;
    const char *description;
};

inline constexpr DiagnosticCodeInfo kDiagnosticCodes[] = {
#define WORKSHOPC_CODE_INFO(enumerator, code, name, description) {code, name, description},
    WORKSHOPC_DIAGNOSTIC_CODES(WORKSHOPC_CODE_INFO)
#undef WORKSHOPC_CODE_INFO
};

inline constexpr std::size_t kDiagnosticCodeCount =
    sizeof(kDiagnosticCodes) / sizeof(kDiagnosticCodes[0]);

inline const DiagnosticCodeInfo &codeInfo(DiagCode code) {
    return kDiagnosticCodes[static_cast<std::size_t>(code)];
}
