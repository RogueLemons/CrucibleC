// Every 'off' must be turned back on in the same file, in headers and
// source files alike. This check is always enabled.

#include "headers/suppression_unterminated.h"

enum after_include { AFTER_INCLUDE }; // bad: the header's missing 'on' does not reach into this file

// WorkshopC off
enum balanced_region { BALANCED }; // good: suppressed
// WorkshopC on

// WorkshopC on // bad: turned on without a preceding off

// WorkshopC off
// WorkshopC off // bad: already off, suppressions can not be nested
enum nested_region { NESTED }; // good: suppressed
// WorkshopC on

enum after_nested { AFTER_NESTED }; // bad: the single 'on' above ended the suppression

// WorkshopC off // bad: never turned back on in this file
enum until_end_of_file { END_OF_FILE }; // good: still suppressed until the end of the file
