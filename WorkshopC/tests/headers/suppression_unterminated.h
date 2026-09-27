#ifndef TESTS_HEADERS_SUPPRESSION_UNTERMINATED_H
#define TESTS_HEADERS_SUPPRESSION_UNTERMINATED_H

// WorkshopC off // bad: never turned back on in this header
enum header_enum { HEADER_VALUE }; // good: still suppressed until the end of the header

#endif // TESTS_HEADERS_SUPPRESSION_UNTERMINATED_H
